#include "market.hpp"
#include <iostream>
#include <thread>

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

void Market::addOrder(Order *order) {
    // Check if limit already exists
    Limit *limit = findLimit(order->limitPrice, order->buyOrder);

    orderMap.push_back(order);

    // Create the limit if it doesn't exist
    if (limit == nullptr) limit = createLimit(order->limitPrice, order->buyOrder);
    
    limit->addOrder(*order);
}

// Find the limit in the map, return nullptr if it doesn't exist
Limit *Market::findLimit(const std::uint32_t limitPrice, const bool buyOrSell) {
    const auto& map = buyOrSell ? buyLimitMap : sellLimitMap;
    const auto it = map.find(limitPrice);
    return it != map.end() ? it->second : nullptr;
}

// Add the limit to the tree, update the root and best bid/ask if necessary
void Market::addLimit(Limit *limit, const bool buyOrSell) {
    if (Limit *&root = buyOrSell ? buyTree : sellTree; root == nullptr) {
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
}

// Create a new limit node and add it to the tree
Limit *Market::createLimit(const std::uint32_t limitPrice, const bool buyOrSell) {
    if (buyOrSell) {
        auto *limit = new Limit();
        limit->limitPrice = limitPrice;
        buyLimitMap[limitPrice] = limit;
        addLimit(limit, buyOrSell);
        return limit;
    }
    else {
        auto *limit = new Limit();
        limit->limitPrice = limitPrice;
        sellLimitMap[limitPrice] = limit;
        addLimit(limit, buyOrSell);
        return limit;
    }
}

std::uint32_t Market::getBestBid() const {
    return highestBuy->limitPrice;
}

std::uint32_t Market::getBestAsk() const{
    return lowestSell->limitPrice;
}

// Add the order to the current limit node, update the size and total volume
void Limit::addOrder(Order& order) {
    if (headOrder == nullptr) {
        headOrder = &order;
        tailOrder = &order;
    }
    else {
        tailOrder->nextOrder = &order;
        order.prevOrder = tailOrder;
        tailOrder = &order;
    }
    order.parentLimit = this;
    size++;
    totalVolume += order.shares;
}