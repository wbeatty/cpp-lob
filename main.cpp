#include <iostream>
#include <time.h>
#include "market.hpp"
#include <thread>
using namespace std;

int main() {
    std::ios_base::sync_with_stdio(false);
    
    clock_t start = clock();

    Market market;
    std::thread readOrdersThread([&market]() { market.readOrders(std::cin); });
    std::thread processOrdersThread([&market]() { market.processOrders(); });

    readOrdersThread.join();
    processOrdersThread.join();

    clock_t end = clock();
    cout << "Time taken: " << static_cast<double>(end - start) / CLOCKS_PER_SEC << " seconds\n";
    cout << "Best bid: " << market.getBestBid() << endl;
    cout << "Best ask: " << market.getBestAsk() << endl;
    return 0;
}
