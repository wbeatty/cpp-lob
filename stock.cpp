#include "stock.hpp"
using namespace std;

Stock::Stock(size_t /* stock_id */) {
}

void Stock::addOrder(const Order& order, bool is_buy) {
    if (is_buy) {
        buy_orders.push(order);
    } else {
        sell_orders.push(order);
    }
}

bool Stock::hasData(bool is_buy) {
    if (is_buy) {
        return !buy_orders.empty();
    } else {
        return !sell_orders.empty();
    }
}

uint32_t Stock::getPrice(bool is_buy) {
    if (is_buy) {
        return buy_orders.top().price;
    } else {
        return sell_orders.top().price;
    }
}

const Order& Stock::getOrder(bool is_buy) {
    if (is_buy) {
        return buy_orders.top();
    } else {
        return sell_orders.top();
    }
}

void Stock::popOrder(bool is_buy) {
    if (is_buy) {
        buy_orders.pop();
    } else {
        sell_orders.pop();
    }
}
