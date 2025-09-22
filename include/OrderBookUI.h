#pragma once
#include "AveragePrice.h"
#include <ftxui/component/screen_interactive.hpp>
#include <string>

class PriceTracker;

class OrderBookUI
{
  private:
    AveragePrice &avgPrice;
    std::string symbol;
    ftxui::ScreenInteractive screen;

  public:
    OrderBookUI(AveragePrice &avgPrice, const std::string &ticker);
    ~OrderBookUI() = default;

    void start();
    void stop();
};
