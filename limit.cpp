#include "limit.hpp"
#include "order.hpp"

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
