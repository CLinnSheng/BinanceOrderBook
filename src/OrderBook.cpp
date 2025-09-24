#include "OrderBook.h"

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        g_running.store(false);
    }
}

std::atomic<bool> g_running(true);

OrderBook::OrderBook(const std::string &tradingSymbol)
    : symbol(tradingSymbol), synchronizer(tradingSymbol), ws(avgPrice, orderBookManager, synchronizer, tradingSymbol),
      ui(avgPrice, orderBookManager, tradingSymbol)
{
    orderBookManager.setSynchronizer(&synchronizer);

    // Set up update callback from synchronizer to UI
    // synchronizer.setUpdateCallback([this]() {});
}

void OrderBook::run()
{
    synchronizer.start();
    ws.start();

    // Wait for synchronization with progress updates
    for (int i = 0; i < 30; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (synchronizer.isSynchronized())
        {
            break;
        }

        if (i == 29)
        {
        }
    }

    ui.start();

    ws.stop();
    synchronizer.stop();
}
