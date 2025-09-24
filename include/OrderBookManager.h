#pragma once

#include "OrderBookData.h"
#include "utils.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>

class OrderBookSynchronizer;

class OrderBookManager
{
  private:
    OrderBookData orderbook;
    mutable std::mutex orderbook_mutex;
    std::function<void()> updateCallback;
    std::atomic<bool> initialized;

    OrderBookSynchronizer *synchronizer = nullptr;

  public:
    OrderBookManager();
    void setSynchronizer(OrderBookSynchronizer *sync);

    void updateOrderBook(const BidsMap &bids, const AsksMap &asks, long long updateId);
    void processDepthUpdate(const std::string &jsonData);

    OrderBookData getOrderBookSnapshot() const;
    std::pair<std::vector<OrderBookLevel>, std::vector<OrderBookLevel>> getTopLevels(int levels = 5) const;

    // Callback for UI updates
    void setUpdateCallback(const std::function<void()> &callback);

    // Status
    bool isInitialized() const;
    void reset();
};
