#include "AveragePrice.h"
#include <mutex>
#include <utility>

AveragePrice::AveragePrice() : currPrice(0), prevPrice(0), priceChange(CONSTANT)
{
}

void AveragePrice::updatePrice(const double updatePrice)
{
    std::lock_guard<std::mutex> lock(avgPriceMutex);

    prevPrice = currPrice;
    currPrice = updatePrice;

    if (prevPrice > 0)
    {
        if (currPrice > prevPrice)
        {
            priceChange = INCREASE;
        }
        else if (currPrice < prevPrice)
        {
            priceChange = DECREASE;
        }
        else
        {
            priceChange = CONSTANT;
        }
    }

    if (updateCallback)
    {
        updateCallback();
    }
}

std::pair<double, PriceChange> AveragePrice::getCurrentPrice()
{
    std::lock_guard<std::mutex> lock(avgPriceMutex);
    return std::make_pair(currPrice, priceChange);
}

void AveragePrice::setUpdateCallback(std::function<void()> callback)
{
    updateCallback = callback;
}
