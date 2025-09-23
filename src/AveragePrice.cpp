#include "AveragePrice.h"
#include <mutex>
#include <utility>

AveragePrice::AveragePrice() : currPrice(0), prevPrice(0), priceChange(CONSTANT), lastChangeTime(std::chrono::steady_clock::now())
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
            lastChangeTime = std::chrono::steady_clock::now();
        }
        else if (currPrice < prevPrice)
        {
            priceChange = DECREASE;
            lastChangeTime = std::chrono::steady_clock::now();
        }
        else
        {
            // If the price is unchanged, reset the timer to persist the color
            lastChangeTime = std::chrono::steady_clock::now();
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

    // Check if enough time has passed to reset color to CONSTANT
    auto now = std::chrono::steady_clock::now();
    if ((priceChange == INCREASE || priceChange == DECREASE) &&
        (now - lastChangeTime) >= COLOR_PERSIST_DURATION)
    {
        priceChange = CONSTANT;
    }

    return std::make_pair(currPrice, priceChange);
}

void AveragePrice::setUpdateCallback(std::function<void()> callback)
{
    updateCallback = callback;
}
