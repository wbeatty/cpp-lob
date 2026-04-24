#include <istream>
#include <vector>
#include "stock.hpp"

using namespace std;

class Market {
    private:
        vector<Stock> stocks;
        uint32_t trades_completed = 0;
        size_t num_traders = 0;
        size_t num_stocks = 0;
        void readOrders(istream &inputStream);
    public:
        Market();
        void getOrders();
        void makeTrades(Order& order, size_t stock_id, bool is_buy);
};
