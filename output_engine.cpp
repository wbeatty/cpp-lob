#include <thread>
#include <iostream>
#include "market.hpp"
#include <fstream>

void Market::processOutput() {
    if (!output) {
        return;
    }
    Trade trade;
    while (true) {
        if (tradeQueue.pop(trade)) {
            processTrade(&trade);
        }
        else if (processingDone.load(std::memory_order_acquire)) {
            while (tradeQueue.pop(trade)) processTrade(&trade);
            break;
        }
        else {
            std::this_thread::yield();
        }
    }
}

void Market::processTrade(const Trade *trade) {
    std::cout << trade->executionTime << ": " << trade->shares << " shares filled at " << trade->price << "\n";
}

void Market::outputData() {
    std::ofstream out("output.txt");

    for (const auto &[idNumber, order] : orderMap) {
        uint64_t matcherTime = order->addCompletedTime - order->dequeueTime;
        out << matcherTime << "\n";
    }
    out.close();
}
