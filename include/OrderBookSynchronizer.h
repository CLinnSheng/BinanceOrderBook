#pragma once

#include "BinanceAPI.h"
#include "OrderBookData.h"
#include "utils.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

struct DepthEvent
{
    long long firstUpdateId = 0; // U
    long long finalUpdateId = 0; // u
    BidsMap bids;
    AsksMap asks;
    std::string rawJson;
    std::chrono::steady_clock::time_point timestamp;

    DepthEvent() : timestamp(std::chrono::steady_clock::now())
    {
    }
};

class OrderBookSynchronizer
{
  private:
    std::string symbol;
    std::atomic<SyncState> state{SyncState::INITIALIZING};

    // Event buffering
    std::queue<DepthEvent> eventBuffer;
    mutable std::mutex bufferMutex;
    std::atomic<long long> firstBufferedEventU{0};

    // Order book state
    OrderBookData orderBook;
    mutable std::mutex orderBookMutex;
    std::atomic<long long> localUpdateId{0};

    // Snapshot handling
    std::future<DepthSnapshot> snapshotFuture;
    std::atomic<bool> snapshotRequested{false};

    // Callbacks
    std::function<void()> updateCallback;

    // Background processing
    std::thread processingThread;
    std::atomic<bool> running{false};

    // Configuration
    static constexpr size_t MAX_BUFFER_SIZE = 1000;
    static constexpr int SNAPSHOT_RETRY_DELAY_MS = 1000;

  public:
    explicit OrderBookSynchronizer(const std::string &tradingSymbol);
    ~OrderBookSynchronizer();

    // Main interface
    void start();
    void stop();
    void reset();

    // Event processing
    void processDepthEvent(const std::string &jsonData);

    // Data access
    OrderBookData getOrderBookSnapshot() const;
    std::pair<std::vector<OrderBookLevel>, std::vector<OrderBookLevel>> getTopLevels(int levels = 5) const;

    // Status
    bool isInitialized() const;
    bool isSynchronized() const;
    SyncState getState() const;
    std::string getStateString() const;

    // Configuration
    void setUpdateCallback(const std::function<void()> &callback);

  private:
    // Binance protocol implementation
    void processEventBuffer();
    void requestSnapshot();
    void handleSnapshotReceived(const DepthSnapshot &snapshot);
    void applyDepthEvent(const DepthEvent &event);
    bool validateEventSequence(const DepthEvent &event) const;
    void backgroundProcessor();

    // Event parsing
    DepthEvent parseDepthEvent(const std::string &jsonData) const;

    // Buffer management
    void bufferEvent(const DepthEvent &event);
    void clearBuffer();
    size_t getBufferSize() const;
};
