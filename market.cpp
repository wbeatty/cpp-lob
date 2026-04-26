#include "market.hpp"
#include <chrono>
#include <charconv>
#include <thread>

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

void Market::addOrder(Order *order) {
    // Check if limit already exists
    Limit *limit = findLimit(order->limitPrice, order->buyOrSell);

    orderMap[order->idNumber] = order;

    // Create the limit if it doesn't exist
    if (limit == nullptr) limit = createLimit(order->limitPrice, order->buyOrSell);
    
    limit->addOrder(*order);
}

// Main matching engine loop
void Market::processOrders() {
    Order *order;
    while (true) {
        if (orderQueue.pop(order)) {
            addOrder(order);
        } else if (inputDone.load(std::memory_order_acquire)) {
            // read in any orders pushed after the flag was set
            while (orderQueue.pop(order)) addOrder(order);
            break;
        } else {
            std::this_thread::yield();
        }
    }
}

// Find the limit in the map, return nullptr if it doesn't exist
Limit *Market::findLimit(std::uint32_t limitPrice, bool buyOrSell) {
    if (buyOrSell) {
        if (buyLimitMap.find(limitPrice) == buyLimitMap.end()) {
            return nullptr;
        }
        return buyLimitMap[limitPrice];
    }
    else {
        if (sellLimitMap.find(limitPrice) == sellLimitMap.end()) {
            return nullptr;
        }
        return sellLimitMap[limitPrice];
    }
}

// Add the limit to the tree, update the root and best bid/ask if necessary
void Market::addLimit(Limit *limit, bool buyOrSell) {
    Limit *&root = buyOrSell ? buyTree : sellTree;
    if (root == nullptr) {
        root = limit;
    }
    else {
        Limit * current = root;
        while (true) {
            if (limit->limitPrice < current->limitPrice) {
                if (current->leftChild == nullptr) {
                    current->leftChild = limit;
                    limit->parent = current;
                    break;
                }
                current = current->leftChild;
            }
            else {
                if (current->rightChild == nullptr) {
                    current->rightChild = limit;
                    limit->parent = current;
                    break;
                }
                current = current->rightChild;
            }
        }
    }
    if (buyOrSell) {
        if (highestBuy == nullptr || limit->limitPrice > highestBuy->limitPrice) {
            highestBuy = limit;
        }
    }
    else {
        if (lowestSell == nullptr || limit->limitPrice < lowestSell->limitPrice) {
            lowestSell = limit;
        }
    }
    return;
}

Limit *Market::createLimit(std::uint32_t limitPrice, bool buyOrSell) {
    if (buyOrSell) {
        Limit *limit = new Limit();
        limit->limitPrice = limitPrice;
        buyLimitMap[limitPrice] = limit;
        addLimit(limit, buyOrSell);
        return limit;
    }
    else {
        Limit *limit = new Limit();
        limit->limitPrice = limitPrice;
        sellLimitMap[limitPrice] = limit;
        addLimit(limit, buyOrSell);
        return limit;
    }
}