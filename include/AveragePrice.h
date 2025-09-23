#pragma once

#include "utils.h"
#include <functional>
#include <mutex>
#include <utility>
#include <chrono>

class AveragePrice
{
  private:
    double currPrice;
    double prevPrice;
    PriceChange priceChange;
    std::chrono::steady_clock::time_point lastChangeTime;

    std::mutex avgPriceMutex;
    std::function<void()> updateCallback;

    // Keep color for this duration (in milliseconds)
    static constexpr std::chrono::milliseconds COLOR_PERSIST_DURATION{2000}; // 2 seconds

  public:
    AveragePrice();
    void updatePrice(const double updatePrice);

    std::pair<double, PriceChange> getCurrentPrice();
    void setUpdateCallback(const std::function<void()> callback);
};
