#include "market.hpp"
#include "trade.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <pthread.h>

// Main matching engine loop
void Market::processOrders() {
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    Order *order;
    while (true) {
        if (orderQueue.pop(order)) {
            if (debug) {
                order->dequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            }
            addOrder(order);
        } else if (inputDone.load(std::memory_order_acquire)) {
            // read in any orders pushed after the flag was set
            while (orderQueue.pop(order)) {
                if (debug) {
                    order->dequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                }
                addOrder(order);
            }
            processingDone.store(true, std::memory_order_release);
            break;
        } else {
            std::this_thread::yield();
        }
    }
}

void Market::addOrder(Order *order) {
    // Check if limit already exists
    Limit *limit = findLimit(order->limitPrice, order->buyOrder);

    orderVector.push_back(order);

    // Create the limit if it doesn't exist
    if (limit == nullptr) limit = createLimit(order->limitPrice, order->buyOrder);
    
    limit->addOrder(*order);

    updateBest(limit, order->buyOrder);
    matchOrders();
    if (debug) {
        order->addCompletedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

// Find the limit in the map, return nullptr if it doesn't exist
Limit *Market::findLimit(const std::uint32_t limitPrice, const bool buyOrder) const {
    const auto& map = buyOrder ? buyLimitMap : sellLimitMap;
    const auto it = map.find(limitPrice);
    return it != map.end() ? it->second : nullptr;
}

// Add the limit to the tree, update the root and best bid/ask if necessary
void Market::addLimit(Limit *limit, const bool buyOrder) {
    if (Limit *&root = buyOrder ? buyTree : sellTree; root == nullptr) {
        root = limit;
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

void Market::updateBest(Limit* limit, const bool buyOrder) {
    if (buyOrder) {
        if (highestBuy == nullptr
            || limit->limitPrice > highestBuy->limitPrice
            || limit == highestBuy) {
            highestBuy = limit;
            bestBid = limit->limitPrice;
        }
    }
    else {
        if (lowestSell == nullptr
            || limit->limitPrice < lowestSell->limitPrice
            || limit == lowestSell) {
            lowestSell = limit;
            bestAsk = limit->limitPrice;
        }
    }
}

// Create a new limit node and add it to the tree
Limit *Market::createLimit(const std::uint32_t limitPrice, const bool buyOrder) {
    Limit *limit = limitPool.allocate();
    if (buyOrder) {
        limit->limitPrice = limitPrice;
        buyLimitMap[limitPrice] = limit;
        addLimit(limit, buyOrder);
        return limit;
    }
    else {
        limit->limitPrice = limitPrice;
        sellLimitMap[limitPrice] = limit;
        addLimit(limit, buyOrder);
        return limit;
    }
}

std::uint32_t Market::getBestBid() const {
    if (highestBuy == nullptr) return 0;
    return highestBuy->limitPrice;
}

std::uint32_t Market::getBestAsk() const{
    if (lowestSell == nullptr) return 0;
    return lowestSell->limitPrice;
}

std::uint32_t Market::getVolumeAtLimit(const Limit& limit) {
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

void Market::matchOrders() {
    while (bestBid >= bestAsk && bestBid != 0 && bestAsk != 0) {
        Limit *buyLimit = findLimit(bestBid, true);
        Limit *sellLimit = findLimit(bestAsk, false);
        if (buyLimit == nullptr || sellLimit == nullptr) {
            break;
        }
        const std::uint32_t prevBid = bestBid;
        const std::uint32_t prevAsk = bestAsk;
        executeLimit(buyLimit, sellLimit);
        if (bestBid == prevBid && bestAsk == prevAsk) {
            break;
        }
    }
}

void Market::executeLimit(Limit* buyLimit, Limit* sellLimit) {
    std::uint32_t buyVolume = getVolumeAtLimit(*buyLimit);
    std::uint32_t sellVolume = getVolumeAtLimit(*sellLimit);

    while (buyVolume > 0 && sellVolume > 0) {
        Order *currentBuy = buyLimit->headOrder;
        Order *currentSell = sellLimit->headOrder;
        if (currentBuy == nullptr || currentSell == nullptr) {
            break;
        }

        const auto tradedShares = std::min(currentBuy->shares, currentSell->shares);
        if (tradedShares == 0) {
            break;
        }

        const bool sellIsMaker = (currentSell->entryTime < currentBuy->entryTime)
            || (currentSell->entryTime == currentBuy->entryTime
                && currentSell->idNumber < currentBuy->idNumber);
        const std::uint32_t tradePrice = sellIsMaker
            ? currentSell->limitPrice
            : currentBuy->limitPrice;

        executeOrder(currentBuy, tradedShares);
        executeOrder(currentSell, tradedShares);

        if (debug) {
            const std::uint64_t executionTime = std::chrono::steady_clock::now().time_since_epoch().count();
            const std::uint64_t takerEntryTime = sellIsMaker ? currentBuy->entryTime : currentSell->entryTime;

            Trade trade{tradeCount++, tradePrice, tradedShares, executionTime, takerEntryTime, currentBuy->idNumber, currentSell->idNumber};
            if (!tradeQueue.push(trade)) {
                std::cerr << "Trade queue full. Dropping trade.\n";
            }
        } else {
            tradeCount++;
        }

        buyVolume -= tradedShares;
        sellVolume -= tradedShares;
    }

    if (buyVolume == 0) {
        Limit* nextBuy = findBestBuy(buyLimit);
        if (nextBuy == nullptr) {
            bestBid = 0;
            highestBuy = nullptr;
        }
        else {
            bestBid = nextBuy->limitPrice;
            highestBuy = nextBuy;
        }
    }
    if (sellVolume == 0) {
       Limit* nextSell = findBestSell(sellLimit);
       if (nextSell == nullptr) {
           bestAsk = 0;
           lowestSell = nullptr;
       }
       else {
            bestAsk = nextSell->limitPrice;
            lowestSell = nextSell;
       }
    }
}

void Market::executeOrder(Order *order, std::uint32_t shares) const {
    if (debug) {
        order->eventTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    order->parentLimit->totalVolume -= shares;
    order->shares -= shares;
    if (order->shares == 0) {
        order->parentLimit->removeOrder(order);
    }
}

void Limit::removeOrder(Order* order) {
    if (order->prevOrder != nullptr) {
        order->prevOrder->nextOrder = order->nextOrder;
    } else {
        headOrder = order->nextOrder;
    }
    if (order->nextOrder != nullptr) {
        order->nextOrder->prevOrder = order->prevOrder;
    } else {
        tailOrder = order->prevOrder;
    }
    order->prevOrder = nullptr;
    order->nextOrder = nullptr;
    size--;
}

Limit* bstMaximum(Limit *limit) {
    while (limit->rightChild != nullptr) {
        limit = limit->rightChild;
    }
    return limit;
}

Limit *bstMinimum(Limit *limit) {
    while (limit->leftChild != nullptr) {
        limit = limit->leftChild;
    }
    return limit;
}

Limit *Market::inOrderPredecessor(Limit *limit) {
    // in-order predecessor algorithm
    if (limit == nullptr) return nullptr;

    if (limit->leftChild != nullptr) {
        return bstMaximum(limit->leftChild);
    }

    if (limit->parent != nullptr && limit->parent->rightChild == limit) {
        return limit->parent;
    }

    Limit *p = limit->parent;
    Limit *curr = limit;

    while (p != nullptr && p->leftChild == curr) {
        curr = p;
        p = p->parent;
    }
    return p;
}

Limit *Market::inOrderSuccessor(Limit *limit) {
    // in-order successor algorithm
    if (limit == nullptr) return nullptr;

    if (limit->rightChild != nullptr) {
        return bstMinimum(limit->rightChild);
    }

    if (limit->parent != nullptr && limit->parent->leftChild == limit) {
        return limit->parent;
    }

    Limit *p = limit->parent;
    Limit *curr = limit;

    while (p != nullptr && p->rightChild == curr) {
        curr = p;
        p = p->parent;
    }
    return p;
}

Limit *Market::findBestBuy(Limit *limit) {
    // find highest limit with volume > 0
    Limit *curr = inOrderPredecessor(limit);
    while (curr != nullptr && curr->totalVolume == 0) {
        curr = inOrderPredecessor(curr);
    }
    return curr;
}

Limit *Market::findBestSell(Limit *limit) {
    // find lowest limit with volume > 0
    Limit *curr = inOrderSuccessor(limit);
    while (curr != nullptr && curr->totalVolume == 0) {
        curr = inOrderSuccessor(curr);
    }
    return curr;
}

void Market::cancelOrder(std::uint32_t idNumber) {
    Order *order = orderVector[idNumber];
    if (order == nullptr) {
        return;
    }
    order->parentLimit->removeOrder(order);
    updateBest(order->parentLimit, order->buyOrder);
    orderVector[idNumber] = nullptr;
    orderPool.deallocate(order);
}