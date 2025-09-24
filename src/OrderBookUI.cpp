#include "AveragePrice.h"
#include "OrderBook.h"
#include "OrderBookManager.h"
#include "OrderBookUI.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace ftxui;

OrderBookUI::OrderBookUI(AveragePrice &avgPrice, OrderBookManager &orderBookManager, const std::string &symbol)
    : avgPrice(avgPrice), orderBookManager(orderBookManager), symbol(symbol), screen(ScreenInteractive::Fullscreen())
{
}

void OrderBookUI::start()
{
    auto component = Renderer([this] {
        auto [price, priceChange] = avgPrice.getCurrentPrice();
        auto [bids, asks] = orderBookManager.getTopLevels(5);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << price;
        std::string priceStr = ss.str();

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

        // Create orderbook display
        Elements bidElements;
        for (const auto &bid : bids)
        {
            std::stringstream bidSs;
            bidSs << std::fixed << std::setprecision(2) << bid.getPrice() << " | " << std::setprecision(4)
                  << bid.getQuantity();
            bidElements.push_back(text(bidSs.str()) | color(Color::GreenLight));
        }

        Elements askElements;
        for (const auto &ask : asks)
        {
            std::stringstream askSs;
            askSs << std::fixed << std::setprecision(2) << ask.getPrice() << " | " << std::setprecision(4)
                  << ask.getQuantity();
            askElements.push_back(text(askSs.str()) | color(Color::Red));
        }

        // Combine all elements
        Elements allElements;

        // Capitalize symbol for display
        std::string capitalizedSymbol = symbol;
        std::transform(capitalizedSymbol.begin(), capitalizedSymbol.end(), capitalizedSymbol.begin(), ::toupper);

        allElements.push_back(text(capitalizedSymbol) | bold | center);
        allElements.push_back(separator());

        if (orderBookManager.isInitialized())
        {
            for (const auto &elem : askElements)
            {
                allElements.push_back(elem | center);
            }

            allElements.push_back(text(""));
            allElements.push_back(hbox({text("Mid Price: "), text(priceStr) | color(textColor) | bold}) | center);
            allElements.push_back(text(""));

            for (const auto &elem : bidElements)
            {
                allElements.push_back(elem | center);
            }
        }
        else
        {
            std::stringstream statusSs;
            statusSs << "Loading orderbook... (bids: " << bids.size() << ", asks: " << asks.size() << ")";
            allElements.push_back(text(statusSs.str()) | dim | center);
        }

        allElements.push_back(separator());
        allElements.push_back(text("Press Ctrl+C to quit") | dim | center);

        return vbox(allElements) | border | center;
    });

    // Create a component that handles Ctrl+C
    auto main_component = CatchEvent(component, [&](Event event) {
        if (event == Event::CtrlC)
        {
            g_running.store(false);
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    // Set up callbacks for UI updates
    avgPrice.setUpdateCallback([&]() {
        if (g_running.load())
        {
            screen.PostEvent(Event::Custom);
        }
    });

    orderBookManager.setUpdateCallback([&]() {
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

    screen.Loop(main_component);

    if (shutdown_monitor.joinable())
    {
        shutdown_monitor.join();
    }
}

void OrderBookUI::stop()
{
    screen.ExitLoopClosure()();
}
