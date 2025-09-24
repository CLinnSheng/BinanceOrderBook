#include "OrderBookSynchronizer.h"
#include <algorithm>
#include <iostream>
#include <json/json.h>

OrderBookSynchronizer::OrderBookSynchronizer(const std::string &tradingSymbol) : symbol(tradingSymbol)
{
}

OrderBookSynchronizer::~OrderBookSynchronizer()
{
    stop();
}

void OrderBookSynchronizer::start()
{
    if (running.load())
    {
        return;
    }

    running.store(true);
    state.store(SyncState::INITIALIZING);

    // Start background processing thread
    processingThread = std::thread(&OrderBookSynchronizer::backgroundProcessor, this);

    // Request initial snapshot
    requestSnapshot();
}

void OrderBookSynchronizer::stop()
{
    if (!running.load())
        return;

    running.store(false);

    if (processingThread.joinable())
    {
        processingThread.join();
    }
}

void OrderBookSynchronizer::reset()
{
    std::lock_guard<std::mutex> orderLock(orderBookMutex);
    std::lock_guard<std::mutex> bufferLock(bufferMutex);

    // Reset all state
    state.store(SyncState::INITIALIZING);
    orderBook.clear();
    localUpdateId.store(0);
    firstBufferedEventU.store(0);
    clearBuffer();
    snapshotRequested.store(false);

    // Request new snapshot
    requestSnapshot();
}

void OrderBookSynchronizer::processDepthEvent(const std::string &jsonData)
{
    if (!running.load())
        return;

    try
    {
        DepthEvent event = parseDepthEvent(jsonData);
        if (event.finalUpdateId == 0)
        {
            return; // Invalid event
        }

        auto currentState = state.load();

        switch (currentState)
        {
        case SyncState::INITIALIZING:
        case SyncState::BUFFERING:
            // Buffer events and note the U of the first event (Step 2)
            bufferEvent(event);

            // If this is the first event, note its U
            if (firstBufferedEventU.load() == 0)
            {
                firstBufferedEventU.store(event.firstUpdateId);
            }

            state.store(SyncState::BUFFERING);
            break;

        case SyncState::SNAPSHOT_RECEIVED:
            // Continue buffering during snapshot processing
            bufferEvent(event);
            break;

        case SyncState::SYNCHRONIZED:
            // Real-time processing
            if (validateEventSequence(event))
            {
                applyDepthEvent(event);
            }
            else
            {
                std::cout << "Event sequence validation failed. Resetting..." << std::endl;
                reset();
            }
            break;

        case SyncState::ERROR_STATE:
            std::cout << "Ignoring event - in error state" << std::endl;
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error processing depth event: " << e.what() << std::endl;
        state.store(SyncState::ERROR_STATE);
    }
}

void OrderBookSynchronizer::backgroundProcessor()
{

    while (running.load())
    {
        try
        {
            auto currentState = state.load();

            switch (currentState)
            {
            case SyncState::BUFFERING:
                // Check if snapshot is ready
                if (snapshotFuture.valid() &&
                    snapshotFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {

                    DepthSnapshot snapshot = snapshotFuture.get();
                    handleSnapshotReceived(snapshot);
                }
                break;

            case SyncState::SNAPSHOT_RECEIVED:
                processEventBuffer();
                break;

            case SyncState::ERROR_STATE:
                std::cout << "In error state, attempting reset..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                reset();
                break;

            default:
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in background processor: " << e.what() << std::endl;
            state.store(SyncState::ERROR_STATE);
        }
    }
}

void OrderBookSynchronizer::requestSnapshot()
{
    if (snapshotRequested.load())
    {
        return;
    }

    snapshotRequested.store(true);
    snapshotFuture = BinanceAPI::getDepthSnapshotAsync(symbol, 5000);
}

void OrderBookSynchronizer::handleSnapshotReceived(const DepthSnapshot &snapshot)
{
    if (!snapshot.isValid)
    {
        snapshotRequested.store(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(SNAPSHOT_RETRY_DELAY_MS));
        requestSnapshot();
        return;
    }

    // Step 4: If lastUpdateId < U from first buffered event, retry snapshot
    long long firstU = firstBufferedEventU.load();
    if (firstU > 0 && snapshot.lastUpdateId < firstU)
    {

        snapshotRequested.store(false);
        requestSnapshot();
        return;
    }

    // Step 6: Set local order book to snapshot
    {
        std::lock_guard<std::mutex> lock(orderBookMutex);
        orderBook.getBids() = snapshot.bids;
        orderBook.getAsks() = snapshot.asks;
        orderBook.setLastUpdateId(snapshot.lastUpdateId);
        localUpdateId.store(snapshot.lastUpdateId);
    }

    state.store(SyncState::SNAPSHOT_RECEIVED);
}

void OrderBookSynchronizer::processEventBuffer()
{
    std::lock_guard<std::mutex> lock(bufferMutex);

    long long currentUpdateId = localUpdateId.load();

    size_t processed = 0;
    size_t discarded = 0;

    // Step 5: Discard events where u <= lastUpdateId of snapshot
    while (!eventBuffer.empty())
    {
        DepthEvent event = eventBuffer.front();
        eventBuffer.pop();

        // Discard old events
        if (event.finalUpdateId <= currentUpdateId)
        {
            discarded++;
            continue;
        }

        // Step 5 continued: First buffered event should have lastUpdateId within [U;u] range
        if (processed == 0)
        {
            if (!(event.firstUpdateId <= currentUpdateId + 1 && event.finalUpdateId >= currentUpdateId + 1))
            {
                state.store(SyncState::ERROR_STATE);
                return;
            }
        }

        // Apply the event
        applyDepthEvent(event);
        processed++;
    }

    // Step 7: Now synchronized - apply subsequent events in real-time
    state.store(SyncState::SYNCHRONIZED);

    if (updateCallback)
    {
        updateCallback();
    }
}

bool OrderBookSynchronizer::validateEventSequence(const DepthEvent &event) const
{
    long long currentUpdateId = localUpdateId.load();

    // Step 1 of update procedure: If u < local update ID, ignore
    if (event.finalUpdateId <= currentUpdateId)
    {
        return false; // Ignore but don't error
    }

    // Step 2 of update procedure: If U > local update ID + 1, something went wrong
    if (event.firstUpdateId > currentUpdateId + 1)
    {
        std::cout << "Gap detected! Expected U <= " << (currentUpdateId + 1) << ", got U = " << event.firstUpdateId
                  << std::endl;
        return false; // This triggers reset
    }

    return true;
}

void OrderBookSynchronizer::applyDepthEvent(const DepthEvent &event)
{
    std::lock_guard<std::mutex> lock(orderBookMutex);

    // Step 3 of update procedure: Apply price level changes
    for (const auto &[price, quantity] : event.bids)
    {
        if (quantity == 0.0)
        {
            orderBook.getBids().erase(price);
        }
        else
        {
            orderBook.getBids()[price] = quantity;
        }
    }

    for (const auto &[price, quantity] : event.asks)
    {
        if (quantity == 0.0)
        {
            orderBook.getAsks().erase(price);
        }
        else
        {
            orderBook.getAsks()[price] = quantity;
        }
    }

    // Step 4 of update procedure: Set order book update ID to u
    orderBook.setLastUpdateId(event.finalUpdateId);
    localUpdateId.store(event.finalUpdateId);

    // Trigger UI update
    if (updateCallback)
    {
        updateCallback();
    }
}

DepthEvent OrderBookSynchronizer::parseDepthEvent(const std::string &jsonData) const
{
    DepthEvent event;

    try
    {
        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(jsonData, root))
        {
            return event;
        }

        // Handle stream format
        Json::Value data = root.isMember("data") ? root["data"] : root;

        // Extract update IDs
        if (!data.isMember("U") || !data.isMember("u"))
        {
            return event;
        }

        event.firstUpdateId = data["U"].asInt64();
        event.finalUpdateId = data["u"].asInt64();
        event.rawJson = jsonData;

        // Parse bids
        if (data.isMember("b"))
        {
            Json::Value bidsArray = data["b"];
            for (const auto &bid : bidsArray)
            {
                if (bid.isArray() && bid.size() >= 2)
                {
                    double price = std::stod(bid[0].asString());
                    double quantity = std::stod(bid[1].asString());
                    event.bids[price] = quantity;
                }
            }
        }

        // Parse asks
        if (data.isMember("a"))
        {
            Json::Value asksArray = data["a"];
            for (const auto &ask : asksArray)
            {
                if (ask.isArray() && ask.size() >= 2)
                {
                    double price = std::stod(ask[0].asString());
                    double quantity = std::stod(ask[1].asString());
                    event.asks[price] = quantity;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing depth event: " << e.what() << std::endl;
    }

    return event;
}

void OrderBookSynchronizer::bufferEvent(const DepthEvent &event)
{
    std::lock_guard<std::mutex> lock(bufferMutex);

    if (eventBuffer.size() >= MAX_BUFFER_SIZE)
    {
        eventBuffer.pop();
    }

    eventBuffer.push(event);
}

void OrderBookSynchronizer::clearBuffer()
{
    while (!eventBuffer.empty())
    {
        eventBuffer.pop();
    }
}

size_t OrderBookSynchronizer::getBufferSize() const
{
    std::lock_guard<std::mutex> lock(bufferMutex);
    return eventBuffer.size();
}

// Data access methods
OrderBookData OrderBookSynchronizer::getOrderBookSnapshot() const
{
    std::lock_guard<std::mutex> lock(orderBookMutex);
    return orderBook;
}

std::pair<std::vector<OrderBookLevel>, std::vector<OrderBookLevel>> OrderBookSynchronizer::getTopLevels(
    int levels) const
{
    std::lock_guard<std::mutex> lock(orderBookMutex);
    return std::make_pair(orderBook.getTopBids(levels), orderBook.getTopAsks(levels));
}

// Status methods
bool OrderBookSynchronizer::isInitialized() const
{
    return state.load() != SyncState::INITIALIZING;
}

bool OrderBookSynchronizer::isSynchronized() const
{
    return state.load() == SyncState::SYNCHRONIZED;
}

SyncState OrderBookSynchronizer::getState() const
{
    return state.load();
}

std::string OrderBookSynchronizer::getStateString() const
{
    switch (state.load())
    {
    case SyncState::INITIALIZING:
        return "INITIALIZING";
    case SyncState::BUFFERING:
        return "BUFFERING";
    case SyncState::SNAPSHOT_RECEIVED:
        return "SNAPSHOT_RECEIVED";
    case SyncState::SYNCHRONIZED:
        return "SYNCHRONIZED";
    case SyncState::ERROR_STATE:
        return "ERROR_STATE";
    default:
        return "UNKNOWN";
    }
}

void OrderBookSynchronizer::setUpdateCallback(const std::function<void()> &callback)
{
    updateCallback = callback;
}
