#include "OrderBookManager.h"
#include <iostream>
#include <json/json.h>

OrderBookManager::OrderBookManager() : initialized(false)
{
}

void OrderBookManager::updateOrderBook(const BidsMap &bids, const AsksMap &asks, long long updateId)
{
    std::lock_guard<std::mutex> lock(orderbook_mutex);

    // Update bids
    for (const auto &bid : bids)
    {
        if (bid.second == 0.0)
        {
            // Remove the price level
            orderbook.getBids().erase(bid.first);
        }
        else
        {
            // Update the price level
            orderbook.getBids()[bid.first] = bid.second;
        }
    }

    // Update asks
    for (const auto &ask : asks)
    {
        if (ask.second == 0.0)
        {
            // Remove the price level
            orderbook.getAsks().erase(ask.first);
        }
        else
        {
            // Update the price level
            orderbook.getAsks()[ask.first] = ask.second;
        }
    }

    orderbook.setLastUpdateId(updateId);
    initialized.store(true);

    // Trigger UI update callback
    if (updateCallback)
    {
        updateCallback();
    }
}

void OrderBookManager::processDepthUpdate(const std::string &jsonData)
{
    try
    {
        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(jsonData, root))
        {
            return;
        }

        // Check if this is the correct stream
        if (!root.isMember("stream") || !root.isMember("data"))
        {
            return;
        }

        Json::Value data = root["data"];

        // Check for full depth stream format (U, u, pu fields)
        if (!data.isMember("b") || !data.isMember("a") || !data.isMember("u"))
        {
            return;
        }

        // Extract update IDs for proper validation (Binance protocol)
        long long firstUpdateId = data.isMember("U") ? data["U"].asInt64() : 0;
        long long finalUpdateId = data["u"].asInt64();
        long long prevUpdateId = data.isMember("pu") ? data["pu"].asInt64() : 0;

        // TODO: Implement proper update validation as per Binance documentation
        // For now, use finalUpdateId as the update ID
        long long updateId = finalUpdateId;

        // Process bids (use "b" field for full depth stream)
        BidsMap bids;
        Json::Value bidsArray = data["b"];
        for (const auto &bid : bidsArray)
        {
            if (bid.isArray() && bid.size() >= 2)
            {
                double price = std::stod(bid[0].asString());
                double quantity = std::stod(bid[1].asString());
                bids[price] = quantity;
            }
        }

        // Process asks (use "a" field for full depth stream)
        AsksMap asks;
        Json::Value asksArray = data["a"];
        for (const auto &ask : asksArray)
        {
            if (ask.isArray() && ask.size() >= 2)
            {
                double price = std::stod(ask[0].asString());
                double quantity = std::stod(ask[1].asString());
                asks[price] = quantity;
            }
        }

        // Debug: Print what we're processing (disabled for clean UI)
        // std::cerr << "Processing " << bids.size() << " bids, " << asks.size() << " asks (updateId: " << updateId << ")" << std::endl;

        // Update the orderbook
        updateOrderBook(bids, asks, updateId);
    }
    catch (const std::exception &e)
    {
        // Silently handle errors to avoid UI glitching
    }
}

OrderBookData OrderBookManager::getOrderBookSnapshot() const
{
    std::lock_guard<std::mutex> lock(orderbook_mutex);
    return orderbook;
}

std::pair<std::vector<OrderBookLevel>, std::vector<OrderBookLevel>> OrderBookManager::getTopLevels(int levels) const
{
    std::lock_guard<std::mutex> lock(orderbook_mutex);
    return std::make_pair(orderbook.getTopBids(levels), orderbook.getTopAsks(levels));
}

void OrderBookManager::setUpdateCallback(const std::function<void()> &callback)
{
    updateCallback = callback;
}

bool OrderBookManager::isInitialized() const
{
    return initialized.load();
}

void OrderBookManager::reset()
{
    std::lock_guard<std::mutex> lock(orderbook_mutex);
    orderbook.clear();
    initialized.store(false);
}
