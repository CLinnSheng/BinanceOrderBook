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
    : symbol(tradingSymbol), ws(avgPrice, tradingSymbol), ui(avgPrice, tradingSymbol)
{
}

void OrderBook::run()
{
    std::cout << "Starting Binance OrderBook for " << symbol << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;

    ws.start();

    // Give websocket some time to connect
    std::this_thread::sleep_for(std::chrono::seconds(2));

    ui.start();

    ws.stop();
}
