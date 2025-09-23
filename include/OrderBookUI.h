#pragma once
#include "AveragePrice.h"
#include <ftxui/component/screen_interactive.hpp>
#include <string>

// Forward declaration
class OrderBookManager;

class OrderBookUI
{
  private:
    AveragePrice &avgPrice;
    OrderBookManager &orderBookManager;
    std::string symbol;
    ftxui::ScreenInteractive screen;

  public:
    OrderBookUI(AveragePrice &avgPrice, OrderBookManager &orderBookManager, const std::string &ticker);
    ~OrderBookUI() = default;

    void start();
    void stop();
};
