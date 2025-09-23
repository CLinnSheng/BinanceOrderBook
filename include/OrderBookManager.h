#pragma once

#include "utils.h"
#include <functional>
#include <mutex>
#include <atomic>
#include <string>
#include <chrono>

class OrderBookManager
{
private:
    OrderBookData orderbook;
    mutable std::mutex orderbook_mutex;
    std::function<void()> updateCallback;
    std::atomic<bool> initialized;

    // UI update throttling
    std::chrono::steady_clock::time_point lastUIUpdate;
    static constexpr std::chrono::milliseconds UI_UPDATE_INTERVAL{100}; // Update UI max every 100ms

public:
    OrderBookManager();

    // Update methods following Binance guidelines
    void updateOrderBook(const BidsMap& bids, const AsksMap& asks, long long updateId);
    void processDepthUpdate(const std::string& jsonData);

    // Data access
    OrderBookData getOrderBookSnapshot() const;
    std::pair<std::vector<OrderBookLevel>, std::vector<OrderBookLevel>> getTopLevels(int levels = 5) const;

    // Callback for UI updates
    void setUpdateCallback(const std::function<void()>& callback);

    // Status
    bool isInitialized() const;
    void reset();
};