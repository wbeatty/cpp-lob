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

    alignas(64) uint32_t bestBid;
    alignas(64) uint32_t bestAsk;

    bool verbose = false;

    Order *createOrder();
    bool queueOrder(Order *order);
    void addOrder(Order *order);
    void matchOrders();
    void executeLimit(Limit* buyLimit, Limit* sellLimit);
    std::uint32_t getVolumeAtLimit(Limit& limit);
    Limit *createLimit(std::uint32_t limitPrice, bool buyOrder);
    Limit *findLimit(std::uint32_t limitPrice, bool buyOrder);
    void addLimit(Limit *limit, bool buyOrder);
    static Limit *inOrderPredecessor(Limit *limit);
    static Limit *inOrderSuccessor(Limit *limit);
    Limit *findBestBuy(Limit *limit);
    Limit *findBestSell(Limit *limit);
    void updateBest(Limit* limit, bool buyOrder);
    void executeOrder(Order* order, std::uint32_t shares, std::uint32_t tradePrice);
};
