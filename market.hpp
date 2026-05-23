#pragma once
#include <atomic>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <unordered_map>
#include <vector>
#include "order.hpp"
#include "limit.hpp"
#include "queue.hpp"
#include "trade.hpp"

class Market {
public:
    Market();
    void getOptions(int argc, char** argv);
    void readOrders();
    void processOrders();
    void processOutput();
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

    SPSCQueue<Trade> tradeQueue;
    std::atomic<bool> processingDone{false};

    char *inputFile;

    alignas(64) uint32_t bestBid;
    alignas(64) uint32_t bestAsk;
    alignas(64) uint32_t tradeCount = 0;

    bool output = false;

    Order *createOrder();
    bool queueOrder(Order *order);
    void addOrder(Order *order);
    void matchOrders();
    void executeLimit(Limit* buyLimit, Limit* sellLimit);
    Limit *createLimit(std::uint32_t limitPrice, bool buyOrder);
    Limit *findLimit(std::uint32_t limitPrice, bool buyOrder) const;
    void addLimit(Limit *limit, bool buyOrder);
    static std::uint32_t getVolumeAtLimit(const Limit& limit);
    static Limit *inOrderPredecessor(Limit *limit);
    static Limit *inOrderSuccessor(Limit *limit);
    static Limit *findBestBuy(Limit *limit);
    static Limit *findBestSell(Limit *limit);
    void updateBest(Limit* limit, bool buyOrder);
    void executeOrder(Order* order, std::uint32_t shares) const;
    void processTrade(const Trade *trade);
};
