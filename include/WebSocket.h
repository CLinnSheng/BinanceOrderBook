#pragma once
#include "AveragePrice.h"
#include <atomic>
#include <string>
#include <thread>
#include <websocketpp/client.hpp>
#include <websocketpp/close.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_client.hpp>

class OrderBookManager;
class OrderBookSynchronizer;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

class WebSocket
{
  private:
    client ws_client;
    websocketpp::connection_hdl hdl;
    std::string baseUri{"wss://stream.binance.com:9443/ws/"};
    std::string symbol;
    std::thread ws_thread;
    std::atomic<bool> running;
    AveragePrice &avgPrice;
    OrderBookManager &orderBookManager;
    OrderBookSynchronizer &synchronizer;

    void on_message(client::message_ptr msg);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close();
    void on_fail();
    context_ptr on_tls_init(websocketpp::connection_hdl hdl);

    // Helper methods
    void updateMidPrice();

  public:
    WebSocket(AveragePrice &avgPrice, OrderBookManager &orderBookManager, OrderBookSynchronizer &synchronizer,
              const std::string &tradingSymbol);
    ~WebSocket();

    void start();
    void stop();
};
