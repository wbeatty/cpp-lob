#pragma once
#include <cstdint>
#include <istream>

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

    void addOrder(Order& order);
    void cancelOrder(Order& order);
    void executeOrder(Order& order);
    std::uint32_t getVolumeAtLimit(Limit& limit);
    std::uint32_t getBestBid();
    std::uint32_t getBestAsk();
};
