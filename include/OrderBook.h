#pragma once

#include "AveragePrice.h"
#include "OrderBookManager.h"
#include "OrderBookSynchronizer.h"
#include "OrderBookUI.h"
#include "WebSocket.h"
#include <atomic>
#include <string>

extern std::atomic<bool> g_running;

class OrderBook
{
  private:
    std::string symbol;
    AveragePrice avgPrice;
    OrderBookManager orderBookManager;
    OrderBookSynchronizer synchronizer;
    WebSocket ws;
    OrderBookUI ui;

  public:
    OrderBook(const std::string &tradingSymbol);
    ~OrderBook() = default;
    void run();
};

void signalHandler(int signal);
