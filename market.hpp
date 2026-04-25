#pragma once
#include <cstdint>
#include <istream>
#include <map>

struct Order;
struct Limit;

class Market {
public:
    Market();
    void readOrders(std::istream& inputStream);
private:
    Limit *buyTree;
    Limit *sellTree;
    Limit *lowestSell;
    Limit *highestBuy;
    std::map<std::uint32_t, Order*> orderMap; // order id -> order pointer
    std::map<std::uint32_t, Limit*> buyLimitMap; // limit price -> limit pointer
    std::map<std::uint32_t, Limit*> sellLimitMap; // limit price -> limit pointer

    void addOrder(Order *order);
    void cancelOrder(Order& order);
    void executeOrder(Order& order);
    std::uint32_t getVolumeAtLimit(Limit& limit);
    std::uint32_t getBestBid();
    std::uint32_t getBestAsk();
    Limit *createLimit(std::uint32_t limitPrice, bool buyOrSell);
    Limit *findLimit(std::uint32_t limitPrice, bool buyOrSell);
    void addLimit(Limit *limit, bool buyOrSell);
};
