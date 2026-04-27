#include <iostream>
#include <chrono>
#include "market.hpp"
#include <thread>

using namespace std;

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    const auto start = std::chrono::steady_clock::now();

    Market market;
    market.getOptions(argc, argv);

    std::thread readOrdersThread([&market]() { market.readOrders(); });
    std::thread processOrdersThread([&market]() { market.processOrders(); });

    readOrdersThread.join();
    processOrdersThread.join();

    const auto end = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(end - start).count();
    cout << "Time taken: " << elapsed << " seconds (wall clock)\n";
    cout << "Best bid: " << market.getBestBid() << endl;
    cout << "Best ask: " << market.getBestAsk() << endl;
    return 0;
}
