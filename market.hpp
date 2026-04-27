#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <memory_resource>
#include <unordered_map>
#include <vector>
#include "order.hpp"
#include "limit.hpp"
#include "queue.hpp"

class Market {
public:
    Market();
    void getOptions(int argc, char** argv);
    void readOrders();
    void processOrders();
    std::uint32_t getBestBid() const;
    std::uint32_t getBestAsk() const;
private:
    static constexpr std::size_t ORDER_POOL_BYTES = 64 * 1024 * 1024;
    std::unique_ptr<std::byte[]> orderPoolBuffer;
    std::pmr::monotonic_buffer_resource orderPool;

    Limit *buyTree;
    Limit *sellTree;
    Limit *lowestSell;
    Limit *highestBuy;
    std::vector<Order*> orderMap;
    std::unordered_map<std::uint32_t, Limit*> buyLimitMap; // limit price -> limit pointer
    std::unordered_map<std::uint32_t, Limit*> sellLimitMap; // limit price -> limit pointer
    SPSCQueue<Order*> orderQueue;
    std::atomic<bool> inputDone{false};
    char *inputFile;

    Order *createOrder();
    bool queueOrder(Order *order);
    void addOrder(Order *order);
    void cancelOrder(Order& order);
    void executeOrder(Order& order);
    std::uint32_t getVolumeAtLimit(Limit& limit);
    Limit *createLimit(std::uint32_t limitPrice, bool buyOrSell);
    Limit *findLimit(std::uint32_t limitPrice, bool buyOrSell);
    void addLimit(Limit *limit, bool buyOrSell);
};
