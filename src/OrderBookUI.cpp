#include "AveragePrice.h"
#include "OrderBook.h"
#include "OrderBookUI.h"
#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace ftxui;

OrderBookUI::OrderBookUI(AveragePrice &avgPrice, const std::string &symbol)
    : avgPrice(avgPrice), symbol(symbol), screen(ScreenInteractive::Fullscreen())
{
}

void OrderBookUI::start()
{
    auto component = Renderer([this] {
        auto [price, priceChange] = avgPrice.getCurrentPrice();

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << price;
        std::string priceStr = ss.str();
        //std::cout << priceStr << std::endl;

        Color textColor = Color::White;
        switch (priceChange)
        {
        case PriceChange::INCREASE:
            textColor = Color::Green;
            break;
        case PriceChange::DECREASE:
            textColor = Color::Red;
            break;
        case PriceChange::CONSTANT:
            textColor = Color::White;
            break;
        }

        return vbox({text("Binance OrderBook") | bold | center, separator(),
                     hbox({text("Symbol: "), text(symbol) | bold}) | center, separator(),
                     hbox({text("Price: "), text(priceStr) | color(textColor) | bold}) | center, separator(),
                     text("Press Ctrl+C to quit") | dim | center}) |
               border | center;
    });

    // Set up callback to refresh UI when price updates
    avgPrice.setUpdateCallback([&]() {
        if (g_running.load())
        {
            screen.PostEvent(Event::Custom);
        }
    });

    // Monitor for shutdown signal
    std::thread shutdown_monitor([&] {
        while (g_running.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        screen.ExitLoopClosure()();
    });

    screen.Loop(component);

    if (shutdown_monitor.joinable())
    {
        shutdown_monitor.join();
    }
}

void OrderBookUI::stop()
{
    screen.ExitLoopClosure()();
}
