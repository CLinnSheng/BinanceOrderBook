#include "WebSocket.h"
#include "OrderBook.h"
#include "OrderBookManager.h"
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

WebSocket::WebSocket(AveragePrice &avgPrice, OrderBookManager &orderBookManager, const std::string &tradingSymbol)
    : running(false), avgPrice(avgPrice), orderBookManager(orderBookManager), symbol(tradingSymbol)
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
    // Check if shutdown was requested
    if (!g_running.load()) {
        // Don't call stop() from within the callback to avoid deadlock
        // Just return and let the main thread handle shutdown
        return;
    }

    std::string payload = msg->get_payload();
    // Debugging disabled for UI display:
    // std::cout << "Received: " << payload.substr(0, 200) << "..." << std::endl;

    try
    {
        Json::Value root;
        Json::Reader reader;

        if (reader.parse(payload, root))
        {
            // Check if this is a stream message
            if (root.isMember("stream") && root.isMember("data"))
            {
                std::string stream = root["stream"].asString();
                Json::Value data = root["data"];

                // Debug: print stream names (disabled)
                // static int msgCount = 0;
                // if (++msgCount % 10 == 0) { // Print every 10th message to avoid spam
                //     std::cerr << "Stream: " << stream << std::endl;
                // }

                // Handle bookTicker stream (contains best bid "b" and best ask "a")
                if (stream.find("@bookTicker") != std::string::npos && data.isMember("b") && data.isMember("a"))
                {
                    double bestBid = std::stod(data["b"].asString());
                    double bestAsk = std::stod(data["a"].asString());
                    double midPrice = (bestBid + bestAsk) / 2.0;
                    avgPrice.updatePrice(midPrice);
                }
                // Handle depth stream
                else if (stream.find("@depth") != std::string::npos)
                {
                    orderBookManager.processDepthUpdate(payload);
                }
            }
            // Fallback for direct bookTicker stream (old format)
            else if (root.isMember("b") && root.isMember("a"))
            {
                double bestBid = std::stod(root["b"].asString());
                double bestAsk = std::stod(root["a"].asString());
                double midPrice = (bestBid + bestAsk) / 2.0;
                avgPrice.updatePrice(midPrice);
            }
        }
    }
    catch (const std::exception &e)
    {
        // Silently handle errors to avoid UI glitching
    }
}

void WebSocket::on_open(websocketpp::connection_hdl hdl)
{
    this->hdl = hdl;
}

void WebSocket::on_close()
{
}

void WebSocket::on_fail()
{
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
        // Silently handle TLS errors to avoid UI glitching
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

    // Stop the client to exit the event loop
    ws_client.stop();

    if (ws_thread.joinable())
    {
        // Use detach instead of join to avoid potential deadlock
        ws_thread.detach();
    }
}

void WebSocket::start()
{
    if (running.load())
        return;

    running.store(true);

    // Subscribe to both bookTicker and depth streams using the combined stream
    // Use @depth for full incremental updates and @bookTicker for best bid/ask
    std::string combined_uri = uri + symbol + "@bookTicker/" + symbol + "@depth";
    // std::cerr << "Connecting to: " << combined_uri << std::endl;

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(combined_uri, ec);

    if (ec)
    {
        return;
    }

    ws_client.connect(con);

    ws_thread = std::thread([this]() { ws_client.run(); });
}
