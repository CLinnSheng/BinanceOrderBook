#include "WebSocket.h"
#include <iostream>
#include <json/json.h>
#include <websocketpp/client.hpp>
#include <websocketpp/close.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_client.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

// WebSocket::WebSocket()
// {
//     ws_client.init_asio();
//
//     // Set message handler
//     ws_client.set_message_handler(
//         [this](websocketpp::connection_hdl hdl, client::message_ptr msg) { this->on_message(hdl, msg); });
//
//     // Set open handler
//     ws_client.set_open_handler([this](websocketpp::connection_hdl hdl) { this->on_open(hdl); });
//
//     // Set close handler
//     ws_client.set_close_handler([this](websocketpp::connection_hdl hdl) { this->on_close(hdl); });
//
//     // Set TLS init handler
//     ws_client.set_tls_init_handler([this](websocketpp::connection_hdl hdl) { return this->on_tls_init(hdl); });
// }

WebSocket::WebSocket(AveragePrice &avgPrice, const std::string &tradingSymbol)
    : running(false), avgPrice(avgPrice), symbol(tradingSymbol)
{
    ws_client.set_access_channels(websocketpp::log::alevel::all);
    ws_client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    ws_client.init_asio();

    ws_client.set_tls_init_handler([this](websocketpp::connection_hdl) {
        return websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
            websocketpp::lib::asio::ssl::context::sslv23);
    });

    ws_client.set_message_handler([this](websocketpp::connection_hdl, client::message_ptr msg) { on_message(msg); });

    ws_client.set_open_handler([this](websocketpp::connection_hdl hdl) { on_open(hdl); });

    ws_client.set_close_handler([this](websocketpp::connection_hdl) { on_close(); });

    ws_client.set_fail_handler([this](websocketpp::connection_hdl) { on_fail(); });
}

// void WebSocket::on_message(websocketpp::connection_hdl hdl, client::message_ptr)
// {
//     std::cout << "Received: " << msg->get_payload() << std::endl;
// }

void WebSocket::on_message(client::message_ptr msg)
{
    //std::cout << "Received: " << msg->get_payload() << std::endl;
    try
    {
        Json::Value root;
        Json::Reader reader;

        if (reader.parse(msg->get_payload(), root))
        {
            if (root.isMember("w"))
            { // "c" is the close price in ticker stream
                double price = std::stod(root["w"].asString());
                avgPrice.updatePrice(price);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Error parsing message: " << e.what() << std::endl;
    }
}

void WebSocket::on_open(websocketpp::connection_hdl hdl)
{
    std::cout << "Connection opened" << std::endl;
    this->hdl = hdl;
}

void WebSocket::on_close()
{
    std::cout << "Connection closed" << std::endl;
}

void WebSocket::on_fail()
{
    std::cout << "WebSocket connection failed" << std::endl;
}
context_ptr WebSocket::on_tls_init(websocketpp::connection_hdl hdl)
{
    context_ptr ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
        websocketpp::lib::asio::ssl::context::sslv23);

    try
    {
        ctx->set_options(
            websocketpp::lib::asio::ssl::context::default_workarounds | websocketpp::lib::asio::ssl::context::no_sslv2 |
            websocketpp::lib::asio::ssl::context::no_sslv3 | websocketpp::lib::asio::ssl::context::single_dh_use);
    }
    catch (std::exception &e)
    {
        std::cout << "TLS init error: " << e.what() << std::endl;
    }

    return ctx;
}

// void WebSocket::connect(const std::string &ticker)
// {
//     websocketpp::lib::error_code ec;
//     const std::string url = this->url + ticker;
//     client::connection_ptr con = ws_client.get_connection(url, ec);
//
//     if (ec)
//     {
//         std::cout << "Could not create connection: " << ec.message() << std::endl;
//         return;
//     }
//
//     hdl = con->get_handle();
//     ws_client.connect(con);
//
//     // Start the ASIO io_service run loop in a separate thread
//     ws_thread = std::thread([this]() { ws_client.run(); });
// }

WebSocket::~WebSocket()
{
    stop();
}
void WebSocket::stop()
{
    if (!running.load())
        return;

    running.store(false);

    websocketpp::lib::error_code ec;
    ws_client.close(hdl, websocketpp::close::status::going_away, "", ec);

    if (ws_thread.joinable())
    {
        ws_thread.join();
    }

    ws_client.stop();
}

void WebSocket::start()
{
    if (running.load())
        return;

    running.store(true);

    std::string uri = "wss://stream.binance.com:9443/ws/" + symbol + "@avgPrice";

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(uri, ec);

    if (ec)
    {
        //std::cout << "Connection error: " << ec.message() << std::endl;
        return;
    }

    ws_client.connect(con);

    ws_thread = std::thread([this]() { ws_client.run(); });
}
