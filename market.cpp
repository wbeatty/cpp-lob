#include "market.hpp"
#include "order.hpp"

Market::Market() {
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
        Order order;
        order.idNumber = idNumber++;
        order.buyOrSell = line[0] == 'B';
        order.limit = std::stoi(line.substr(2));
        order.shares = std::stoi(line.substr(4));
        addOrder(order);
    }
}