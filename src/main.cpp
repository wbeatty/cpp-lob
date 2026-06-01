#include <iostream>
#include <chrono>
#include "market.hpp"
#include <thread>

using namespace std;

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    Market market;
    market.getOptions(argc, argv);

    if (market.getDebug()) {
        cout << "Debug Mode Enabled" << endl;
        cout << "Which data would you like to output? (t)elementry, (o)rder book, (l)og, (a)ll, (q)uit" << endl;
        char choice;
        cin >> choice;
        if (!market.setOutputs(choice)) {
            return 0;
        }
    }

    const auto start = std::chrono::steady_clock::now();

    std::thread readOrdersThread([&market]() { market.readOrders(); });
    std::thread processOrdersThread([&market]() { market.processOrders(); });
    std::thread processTradesThread([&market] {market.processOutput(); });

    readOrdersThread.join();
    processOrdersThread.join();
    processTradesThread.join();

    const auto end = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(end - start).count();
    cout << "Time taken: " << elapsed << " seconds (wall clock)\n";
    cout << "Throughput: " << market.getOrderCount() / elapsed << " orders/second\n";

    market.outputData();

    return 0;
}
