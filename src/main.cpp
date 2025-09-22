#include "OrderBook.h"
#include <csignal>
#include <iostream>
#include <string>

int main()
{
    std::signal(SIGINT, signalHandler);

    const std::string symbol{"btcusdt"};

    try
    {
        OrderBook orderBook(symbol);
        orderBook.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
