#include "OrderBook.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

// Custom signal handler for input mode
void inputSignalHandler(int signal)
{
    if (signal == SIGINT)
    {
        g_running.store(false);
        std::cout << "\nApplication interrupted by user." << std::endl;
        std::exit(0); // Exit immediately during input
    }
}

int main(int argc, char *argv[])
{
    // Set up signal handler early
    std::signal(SIGINT, signalHandler);

    std::string symbol;

    // Interactive input with special signal handling
    std::cout << "Binance OrderBook Application" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Enter trading symbol (e.g., btcusdt, ethusdt, adausdt) or press Ctrl+C to exit: ";
    std::cout.flush();

    // Switch to input-specific signal handler
    std::signal(SIGINT, inputSignalHandler);

    std::string input;

    // Read input normally
    std::getline(std::cin, input);

    // Switch back to normal signal handler
    std::signal(SIGINT, signalHandler);

    symbol = input;

    // Convert to lowercase for consistency
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::tolower);

    if (symbol.empty())
    {
        std::cout << "No symbol entered, using default: btcusdt" << std::endl;
        symbol = "btcusdt";
    }

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
