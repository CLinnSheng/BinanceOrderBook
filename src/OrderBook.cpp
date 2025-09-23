#include "OrderBook.h"

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        g_running.store(false);
        //std::cout << "\nShutting down gracefully..." << std::endl;
    }
}

std::atomic<bool> g_running(true);

OrderBook::OrderBook(const std::string &tradingSymbol)
    : symbol(tradingSymbol), ws(avgPrice, orderBookManager, tradingSymbol), ui(avgPrice, orderBookManager, tradingSymbol)
{
}

void OrderBook::run()
{
    // Status messages disabled to prevent FTXUI interference

    ws.start();

    // Give websocket some time to connect
    std::this_thread::sleep_for(std::chrono::seconds(2));

    ui.start();

    // Immediately stop WebSocket when UI exits
    ws.stop();
}
