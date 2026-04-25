#include <iostream>
#include <time.h>
#include "market.hpp"

using namespace std;

int main() {
    std::ios_base::sync_with_stdio(false);
    
    clock_t start = clock();

    Market market;
    cout << "Processing orders...\n";
    market.readOrders(std::cin);


    clock_t end = clock();
    cout << "Time taken: " << static_cast<double>(end - start) / CLOCKS_PER_SEC << " seconds\n";
    return 0;
}
