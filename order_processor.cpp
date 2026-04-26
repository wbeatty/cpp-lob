#include "market.hpp"
#include <chrono>
#include <charconv>

Market::Market() : orderQueue(4095) {
    buyTree = nullptr;
    sellTree = nullptr;
    lowestSell = nullptr;
    highestBuy = nullptr;
}

void Market::readOrders(std::istream& inputStream) {
    std::string line;
    uint32_t idNumber = 0;

    while (std::getline(inputStream, line)) {
        if (line[0] == '#') continue;
        
        const char *p = line.data();
        const char *end = p + line.size();

        Order *order = new Order();
        order->buyOrSell = (p[0] == 'B');
        p += 2;

        auto [ptr1, ec1] = std::from_chars(p, end, order->limitPrice);

        if (ec1 != std::errc()) {
            throw std::runtime_error("Invalid limit price");
        }
        p = ptr1 + 1;

        auto [ptr2, ec2] = std::from_chars(p, end, order->shares);
        if (ec2 != std::errc()) {
            throw std::runtime_error("Invalid shares");
        }
        order->idNumber = idNumber++;
        order->entryTime = std::chrono::system_clock::now().time_since_epoch().count();

        queueOrder(order);
    }
    inputDone.store(true, std::memory_order_release);
}

void Market::queueOrder(Order *order) {
    if (!orderQueue.push(order)) {
        throw std::runtime_error("Order queue is full");
    }
}
