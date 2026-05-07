#include "market.hpp"
#include <algorithm>
#include <thread>
#include <iostream>

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

    updateBest(limit, order->buyOrder);
    makeTrades();
}

// Find the limit in the map, return nullptr if it doesn't exist
Limit *Market::findLimit(const std::uint32_t limitPrice, const bool isBuy) const {
    const auto& map = isBuy ? buyLimitMap : sellLimitMap;
    const auto it = map.find(limitPrice);
    return it != map.end() ? it->second : nullptr;
}


void Market::addLimit(Limit *limit, const bool isBuy) {
    if (Limit *&root = isBuy ? buyTree : sellTree; root == nullptr) {
        root = limit;
        return;
    }
    else {
        Limit * current = root;
        while (true) {
            if (limit->limitPrice < current->limitPrice) {
                if (current->leftChild == nullptr) {
                    current->leftChild = limit;
                    limit->parent = current;
                    return;
                }
                current = current->leftChild;
            }
            else {
                if (current->rightChild == nullptr) {
                    current->rightChild = limit;
                    limit->parent = current;
                    return;
                }
                current = current->rightChild;
            }
        }
    }
}

void Market::updateBest(Limit *limit, const bool isBuy) {
    if (isBuy) {
        if (highestBuy == nullptr || limit->limitPrice > highestBuy->limitPrice) {
            highestBuy = limit;
            bestBid = limit->limitPrice;
        }
    } else {
        if (lowestSell == nullptr || limit->limitPrice < lowestSell->limitPrice) {
            lowestSell = limit;
            bestAsk = limit->limitPrice;
        }
    }
}

// Create a new limit node and add it to the tree
Limit *Market::createLimit(const std::uint32_t limitPrice, const bool isBuy) {
    if (isBuy) {
        auto *limit = new Limit();
        limit->limitPrice = limitPrice;
        buyLimitMap[limitPrice] = limit;
        addLimit(limit, isBuy);
        return limit;
    }
    else {
        auto *limit = new Limit();
        limit->limitPrice = limitPrice;
        sellLimitMap[limitPrice] = limit;
        addLimit(limit, isBuy);
        return limit;
    }
}

std::uint32_t Market::getVolumeAtLimit(Limit& limit) {
    return limit.totalVolume;
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

static Limit *inOrderSuccessor(Limit *node) {
    if (node == nullptr) return nullptr;
    if (node->rightChild != nullptr) {
        Limit *cur = node->rightChild;
        while (cur->leftChild != nullptr) cur = cur->leftChild;
        return cur;
    }
    Limit *p = node->parent;
    while (p != nullptr && node == p->rightChild) {
        node = p;
        p = p->parent;
    }
    return p;
}

static Limit *inOrderPredecessor(Limit *node) {
    if (node == nullptr) return nullptr;
    if (node->leftChild != nullptr) {
        Limit *cur = node->leftChild;
        while (cur->rightChild != nullptr) cur = cur->rightChild;
        return cur;
    }
    Limit *p = node->parent;
    while (p != nullptr && node == p->leftChild) {
        node = p;
        p = p->parent;
    }
    return p;
}

Limit *Market::nextHigherLimit(Limit *limit) const {
    Limit *next = inOrderSuccessor(limit);
    while (next != nullptr && next->size == 0) next = inOrderSuccessor(next);
    return next;
}

Limit *Market::nextLowerLimit(Limit *limit) const {
    Limit *next = inOrderPredecessor(limit);
    while (next != nullptr && next->size == 0) next = inOrderPredecessor(next);
    return next;
}

void Market::makeTrades() {
    while (highestBuy != nullptr && lowestSell != nullptr
           && highestBuy->limitPrice >= lowestSell->limitPrice) {
        Limit *ask = lowestSell;
        Limit *bid = highestBuy;
        executeLimit(ask, bid);

        if (verbose) {
            std::cout << "Ask: " << ask->limitPrice << " Bid: " << bid->limitPrice;
            float spread = ((bid->limitPrice - ask->limitPrice) / static_cast<float> (ask->limitPrice)) * 100;
            std::cout << " Spread: " << spread << "%\n";
        }
        if (ask->size == 0) {
            lowestSell = nextHigherLimit(ask);
            bestAsk = (lowestSell != nullptr) ? lowestSell->limitPrice : 0;
        }
        if (bid->size == 0) {
            highestBuy = nextLowerLimit(bid);
            bestBid = (highestBuy != nullptr) ? highestBuy->limitPrice : 0;
        }
    }
}

void Market::executeLimit(Limit *ask, Limit *bid) {
    while (ask->headOrder != nullptr && bid->headOrder != nullptr) {
        Order *sellOrder = ask->headOrder;
        Order *buyOrder = bid->headOrder;

        const std::uint32_t traded = std::min(sellOrder->shares, buyOrder->shares);

        sellOrder->shares -= traded;
        buyOrder->shares -= traded;
        ask->totalVolume -= traded;
        bid->totalVolume -= traded;

        if (verbose) {
            std::cout << "Orders " << sellOrder->idNumber << " and " << buyOrder->idNumber << " executed " << traded << " shares at " << bid->limitPrice << std::endl;
        }

        if (sellOrder->shares == 0) executeOrder(sellOrder);
        if (buyOrder->shares == 0) executeOrder(buyOrder);
    }
}

void Market::executeOrder(Order *order) {
    Limit *limit = order->parentLimit;

    if (order->prevOrder != nullptr) {
        order->prevOrder->nextOrder = order->nextOrder;
    } else {
        limit->headOrder = order->nextOrder;
    }
    if (order->nextOrder != nullptr) {
        order->nextOrder->prevOrder = order->prevOrder;
    } else {
        limit->tailOrder = order->prevOrder;
    }

    order->prevOrder = nullptr;
    order->nextOrder = nullptr;
    limit->size--;
}
