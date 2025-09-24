#include "OrderBook.h"
#include "WebSocket.h"
#include <algorithm>
#include <cctype>
#include <json/json.h>
#include <websocketpp/client.hpp>
#include <websocketpp/close.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_client.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

WebSocket::WebSocket(AveragePrice &avgPrice, OrderBookManager &orderBookManager, OrderBookSynchronizer &synchronizer,
                     const std::string &tradingSymbol)
    : running(false), avgPrice(avgPrice), orderBookManager(orderBookManager), synchronizer(synchronizer),
      symbol(tradingSymbol)
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

void WebSocket::on_message(client::message_ptr msg)
{
    // Check if shutdown was requested
    if (!g_running.load())
    {
        // Don't call stop() from within the callback to avoid deadlock
        // Just return and let the main thread handle shutdown
        return;
    }

    std::string payload = msg->get_payload();

    try
    {
        Json::Value root;
        Json::Reader reader;

        if (reader.parse(payload, root))
        {
            // Check if this is a stream message (combined streams format)
            if (root.isMember("stream") && root.isMember("data"))
            {
                std::string stream = root["stream"].asString();

                // Handle depth stream only - process through synchronizer
                if (stream.find("@depth") != std::string::npos)
                {
                    synchronizer.processDepthEvent(payload);
                    updateMidPrice();
                }
            }
            // Handle direct depth stream format (single stream)
            else if (root.isMember("U") && root.isMember("u") && root.isMember("b") && root.isMember("a"))
            {
                // This is a direct depth update, not wrapped in stream format
                synchronizer.processDepthEvent(payload);
                updateMidPrice();
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
    }

    return ctx;
}

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

void WebSocket::updateMidPrice()
{
    if (synchronizer.isSynchronized())
    {
        auto [bids, asks] = synchronizer.getTopLevels(1);
        if (!bids.empty() && !asks.empty())
        {
            double bestBid = bids[0].getPrice();
            double bestAsk = asks[0].getPrice();
            double midPrice = (bestBid + bestAsk) / 2.0;
            avgPrice.updatePrice(midPrice);
        }
    }
}

void WebSocket::start()
{
    if (running.load())
        return;

    running.store(true);

    std::string lowerSymbol = symbol;
    std::transform(lowerSymbol.begin(), lowerSymbol.end(), lowerSymbol.begin(), ::tolower);
    std::string depth_uri = baseUri + lowerSymbol + "@depth";

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(depth_uri, ec);

    if (ec)
    {
        return;
    }

    ws_client.connect(con);

    ws_thread = std::thread([this]() { ws_client.run(); });
}
