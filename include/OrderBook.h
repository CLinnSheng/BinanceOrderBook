#pragma once

#include "AveragePrice.h"
#include "OrderBookManager.h"
#include "OrderBookUI.h"
#include "WebSocket.h"

extern std::atomic<bool> g_running;

class OrderBook
{
  private:
    AveragePrice avgPrice;
    OrderBookManager orderBookManager;
    WebSocket ws;
    std::string symbol;
    OrderBookUI ui;

  public:
    OrderBook(const std::string &tradingSymbol);
    ~OrderBook() = default;
    void run();
};

void signalHandler(int signal);
