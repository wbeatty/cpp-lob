#include <iostream>
#include <sstream>
#include <string>
#include "market.hpp"
#include "random.h"
using namespace std;

namespace {

bool is_comment_or_blank(const string& line) {
    auto p = line.find_first_not_of(" \t\r\n");
    if (p == string::npos) {
        return true;
    }
    return line.compare(p, 8, "COMMENT:") == 0;
}

} // namespace

Market::Market() {
}

void Market::getOrders() {
    ostringstream whole;
    whole << cin.rdbuf();
    istringstream in(whole.str());

    uint32_t num_traders_u = 0;
    uint32_t num_stocks_u = 0;
    uint32_t seed = 0;
    uint32_t num_orders = 0;
    uint32_t arrival_rate = 0;
    bool have_traders = false;
    bool have_stocks = false;
    bool have_seed = false;
    bool have_orders = false;
    bool have_arrival = false;

    string line;
    while (getline(in, line)) {
        if (is_comment_or_blank(line)) {
            continue;
        }
        auto p = line.find_first_not_of(" \t\r\n");
        istringstream ls(line.substr(p));
        string key;
        ls >> key;
        if (key == "NUM_TRADERS:") {
            ls >> num_traders_u;
            have_traders = true;
        } else if (key == "NUM_STOCKS:") {
            ls >> num_stocks_u;
            have_stocks = true;
        } else if (key == "SEED:") {
            ls >> seed;
            have_seed = true;
        } else if (key == "NUM_ORDERS:") {
            ls >> num_orders;
            have_orders = true;
        } else if (key == "ARRIVAL_RATE:") {
            ls >> arrival_rate;
            have_arrival = true;
        } else {
            cerr << "Error: Unknown line in input: " << line << endl;
            exit(1);
        }
    }

    if (!have_traders || !have_stocks || !have_seed || !have_orders || !have_arrival) {
        cerr << "Error: Need NUM_TRADERS, NUM_STOCKS, SEED, NUM_ORDERS, and ARRIVAL_RATE\n";
        exit(1);
    }
    if (num_traders_u == 0 || num_stocks_u == 0 || num_orders == 0 || arrival_rate == 0) {
        cerr << "Error: NUM_TRADERS, NUM_STOCKS, NUM_ORDERS, and ARRIVAL_RATE must be positive\n";
        exit(1);
    }

    this->num_traders = static_cast<size_t>(num_traders_u);
    this->num_stocks = static_cast<size_t>(num_stocks_u);

    stringstream order_stream;
    Random::PR_init(order_stream, seed, num_traders_u, num_stocks_u, num_orders, arrival_rate);
    readOrders(order_stream);
}

void Market::readOrders(istream &inputStream) {
    char trash2;

    int timestamp;
    string order_type;
    int trader_id;
    int stock_id;
    int price;
    int quantity;

    stocks.reserve(this->num_stocks);
    for (size_t i = 0; i < this->num_stocks; i++) {
        stocks.emplace_back(i);
    }

    uint32_t id = 0;
    int CURRENT_TIMESTAMP = 0;
    while (inputStream >> timestamp >> order_type >> trash2 >> trader_id
        >> trash2 >> stock_id >> trash2 >> price >> trash2 >> quantity) {

        if (timestamp < 0) {
            cerr << "Error: Negative timestamp" << endl;
            exit(1);
        }
        if (timestamp < CURRENT_TIMESTAMP) {
            cerr << "Error: Decreasing timestamp" << endl;
            exit(1);
        }
        if (trader_id >= static_cast<int>(this->num_traders) || trader_id < 0) {
            cerr << "Error: Invalid trader ID" << endl;
            exit(1);
        }
        if (stock_id >= static_cast<int>(this->num_stocks) || stock_id < 0) {
            cerr << "Error: Invalid stock ID" << endl;
            exit(1);
        }
        if (price <= 0) {
            cerr << "Error: Invalid price" << endl;
            exit(1);
        }
        if (quantity <= 0) {
            cerr << "Error: Invalid quantity" << endl;
            exit(1);
        }
        CURRENT_TIMESTAMP = timestamp;

        Order order;
        bool is_buy = (order_type == "BUY") ? true : false;
        order.trader_id = static_cast<uint32_t>(trader_id);
        order.price = static_cast<uint32_t>(price);
        order.quantity = static_cast<uint32_t>(quantity);
        order.id = static_cast<uint32_t>(id);
        id++;

        makeTrades(order, static_cast<size_t>(stock_id), is_buy);
    }

    cout << "---End of Day---\n";
    cout << "Trades Completed: " << trades_completed << endl;
}

void Market::makeTrades(Order& order, size_t stock_id, bool is_buy) {
    if (is_buy) {
        if (!stocks[stock_id].hasData(false)) {
            stocks[stock_id].addOrder(order, is_buy);
            return;
        }
        else {
            while (stocks[stock_id].hasData(false) && order.price >= stocks[stock_id].getPrice(false)) {
                Order sell_order = stocks[stock_id].getOrder(false);

                if (sell_order.quantity < order.quantity) {
                    stocks[stock_id].popOrder(false);
                    order.quantity -= sell_order.quantity;
                    trades_completed++;
                }
                else if (sell_order.quantity > order.quantity) {
                    sell_order.quantity -= order.quantity;
                    stocks[stock_id].popOrder(false);
                    stocks[stock_id].addOrder(sell_order, false);
                    trades_completed++;
                    return;
                }
                else {
                    stocks[stock_id].popOrder(false);
                    trades_completed++;
                    return;
                }
            }
        }
        if (order.quantity > 0) {
            stocks[stock_id].addOrder(order, is_buy);
        }
    }
    else {
        if (!stocks[stock_id].hasData(true)) {
            stocks[stock_id].addOrder(order, is_buy);
            return;
        }
        else {
            while (stocks[stock_id].hasData(true) && order.price <= stocks[stock_id].getPrice(true)) {
                Order buy_order = stocks[stock_id].getOrder(true);

                if (buy_order.quantity < order.quantity) {
                    stocks[stock_id].popOrder(true);
                    order.quantity -= buy_order.quantity;
                    trades_completed++;
                }
                else if (buy_order.quantity > order.quantity) {
                    buy_order.quantity -= order.quantity;
                    stocks[stock_id].popOrder(true);
                    stocks[stock_id].addOrder(buy_order, true);
                    trades_completed++;
                    return;
                }
                else {
                    stocks[stock_id].popOrder(true);
                    trades_completed++;
                    return;
                }
            }
        }
        if (order.quantity > 0) {
            stocks[stock_id].addOrder(order, is_buy);
        }
    }
}
