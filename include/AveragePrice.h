#pragma once

#include "utils.h"
#include <functional>
#include <mutex>
#include <utility>

class AveragePrice
{
  private:
    double currPrice;
    double prevPrice;
    PriceChange priceChange;

    std::mutex avgPriceMutex;
    std::function<void()> updateCallback;

  public:
    AveragePrice();
    void updatePrice(const double updatePrice);

    std::pair<double, PriceChange> getCurrentPrice();
    void setUpdateCallback(const std::function<void()> callback);
};
