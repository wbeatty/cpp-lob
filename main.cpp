#include <iostream>
#include <time.h>
#include "market.hpp"
#include <thread>
using namespace std;

int main() {
    std::ios_base::sync_with_stdio(false);
    
    clock_t start = clock();

    Market market;
    cout << "Processing orders...\n";

    std::thread readOrdersThread([&market]() { market.readOrders(std::cin); });
    std::thread processOrdersThread([&market]() { market.processOrders(); });

    readOrdersThread.join();
    processOrdersThread.join();

    clock_t end = clock();
    cout << "Time taken: " << static_cast<double>(end - start) / CLOCKS_PER_SEC << " seconds\n";
    return 0;
}
