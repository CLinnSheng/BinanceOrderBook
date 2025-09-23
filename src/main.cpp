#include "OrderBook.h"
#include <csignal>
#include <iostream>
#include <string>
#include <algorithm>

int main()
{
    std::signal(SIGINT, signalHandler);

    std::string symbol;

    std::cout << "Binance OrderBook Application" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Enter trading symbol (e.g., btcusdt, ethusdt, adausdt): ";
    std::getline(std::cin, symbol);

    // Convert to lowercase for consistency
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::tolower);

    if (symbol.empty()) {
        std::cout << "No symbol entered, using default: btcusdt" << std::endl;
        symbol = "btcusdt";
    }

    // Status messages disabled to prevent FTXUI interference

    try
    {
        OrderBook orderBook(symbol);
        orderBook.run();
    }
    catch (const std::exception &e)
    {
        // Error output disabled to prevent FTXUI interference
    }
    return 0;
}
