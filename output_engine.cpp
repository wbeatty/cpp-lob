#include <thread>
#include <iostream>
#include "market.hpp"
#include <fstream>
#include <filesystem>


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

void Market::processTrade(Trade *trade) {
    tradeLog.push_back(*trade);
}

void Market::outputData() {
    std::filesystem::create_directories("output");

    std::ofstream queueWaitTimes("output/queue_wait_times.txt");
    if (!queueWaitTimes.is_open()) {
        std::cerr << "Failed to open queue wait times file\n";
        return;
    }
    for (const auto &[idNumber, order] : orderMap) {
        uint64_t queueWaitTime = order->dequeueTime - order->entryTime;
        queueWaitTimes << queueWaitTime << "\n";
    }
    queueWaitTimes.close();

    std::ofstream matcherProcessingTimes("output/matcher_processing_times.txt");
    if (!matcherProcessingTimes.is_open()) {
        std::cerr << "Failed to open matcher processing times file\n";
        return;
    }
    for (const auto &[idNumber, order] : orderMap) {
        uint64_t matcherProcessingTime = order->addCompletedTime - order->dequeueTime;
        matcherProcessingTimes << matcherProcessingTime << "\n";
    }
    matcherProcessingTimes.close();

    std::ofstream endToEndTimes("output/end_to_end_times.txt");
    if (!endToEndTimes.is_open()) {
        std::cerr << "Failed to open end to end times file\n";
        return;
    }
    for (const auto &[idNumber, order] : orderMap) {
        uint64_t endToEndTime = order->addCompletedTime - order->entryTime;
        endToEndTimes << endToEndTime << "\n";
    }
    endToEndTimes.close();

    std::ofstream tickToTradeTimes("output/tick_to_trade_times.txt");
    if (!tickToTradeTimes.is_open()) {
        std::cerr << "Failed to open tick to trade times file\n";
        return;
    }
    for (const auto &trade : tradeLog) {
        uint64_t tickToTradeTime = trade.executionTime - trade.takerEntryTime;
        tickToTradeTimes << tickToTradeTime << "\n";
    }
    tickToTradeTimes.close();
}
