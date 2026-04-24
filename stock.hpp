#include <queue>
#include <vector>

using namespace std;

struct Order {
    uint32_t trader_id;
    uint32_t id; // unique id for the order
    uint32_t price;
    uint32_t quantity;
};

struct buyComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price == b.price) {
            return a.id > b.id;
        }
        return a.price < b.price;
    }
};

struct sellComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price == b.price) {
            return a.id > b.id;
        }
        return a.price > b.price;
    }
};

class Stock {
    private:
        priority_queue<Order, vector<Order>, buyComparator> buy_orders;
        priority_queue<Order, vector<Order>, sellComparator> sell_orders;

    public:
        Stock() = default;
        Stock(size_t stock_name);
        void addOrder(const Order& order, bool is_buy);
        bool hasData(bool is_buy);
        uint32_t getPrice(bool is_buy);
        const Order& getOrder(bool is_buy);
        void popOrder(bool is_buy);
};
