#pragma once
#include <atomic>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include "order.hpp"
#include "limit.hpp"
#include "queue.hpp"
#include "trade.hpp"
#include "chunkpool.hpp"

class Market {
public:
    Market();
    void getOptions(int argc, char** argv);
    void readOrders();
    void processOrders();
    void processOutput();
    std::uint32_t getBestBid() const;
    std::uint32_t getBestAsk() const;
    std::uint32_t getOrderCount() const;
    bool getDebug() const { return debug; }
    std::uint64_t getDroppedTrades() const { return droppedTrades; }
    bool setOutputs(char choice);
    void outputData();

    // Direct entry points for benchmarks and tests: run the matching core on
    // the calling thread, bypassing the ingest/output pipeline. The caller
    // must assign idNumber sequentially from 0 (it indexes orderVector).
    Order *allocateOrder() { return orderPool.allocate(); }
    void submitAdd(Order *order) { addOrder(order); }
    void submitCancel(std::uint32_t idNumber) { cancelOrder(idNumber); }
    bool isLive(std::uint32_t idNumber) const {
        return idNumber < orderVector.size() && orderVector[idNumber] != nullptr;
    }
private:
    static constexpr std::size_t ORDER_POOL_CAPACITY = 2 * 1024 * 1024;
    static constexpr std::size_t LIMIT_POOL_CAPACITY = 1 * 1024 * 1024;
    // Pre-sized so the output thread does not realloc (and memcpy ~150 MB)
    // mid-run. On its own this did not stop trade-queue overflow -- the ring
    // size below is what does -- but it removes a large consumer-side stall.
    static constexpr std::size_t TRADE_LOG_CAPACITY = 4 * 1024 * 1024;
    // The trade queue only carries traffic under -d (release never pushes), so
    // it is widened for telemetry runs: a 4095-slot ring overflows on bursts,
    // and dropping fills biases tick-to-trade toward the quiet periods.
    static constexpr std::size_t TRADE_QUEUE_DEBUG_SIZE = 64 * 1024 - 1;
    
    MemoryManager<Limit> limitPool;
    MemoryManager<Order> orderPool;

    Limit *buyTree;
    Limit *sellTree;
    Limit *lowestSell;
    Limit *highestBuy;

    std::vector<Order*> orderVector; // vector of order pointers

    // Per-order telemetry, copied out of the Order before matching. Filled and
    // cancelled orders are returned to the pool, so their timestamps cannot be
    // read back from orderVector at the end of the run.
    struct OrderTiming {
        std::uint64_t entryTime = 0;
        std::uint64_t queuedTime = 0;
        std::uint64_t dequeueTime = 0;
        std::uint64_t addCompletedTime = 0;
    };
    std::vector<OrderTiming> orderTimings; // debug (-d) runs only
    std::unordered_map<std::uint32_t, Limit*> buyLimitMap; // limit price -> limit pointer
    std::unordered_map<std::uint32_t, Limit*> sellLimitMap; // limit price -> limit pointer

    SPSCQueue<Order*> orderQueue;
    std::atomic<bool> inputDone{false};

    SPSCQueue<Trade> tradeQueue;
    std::atomic<bool> processingDone{false};

    char *inputFile;
    std::vector<Trade> tradeLog; // trades that were executed

    alignas(64) uint32_t bestBid;
    alignas(64) uint32_t bestAsk;
    alignas(64) uint32_t tradeCount = 0;
    // Matcher thread only; read by main after the threads join.
    std::uint64_t droppedTrades = 0;
    
    bool debug = false;
    enum class OutputChoice {NONE, TELEMETRY, ORDER_BOOK, TRADE_LOG, ALL};
    OutputChoice outputChoice;
    bool outputTelemetry = false;
    bool outputOrderBook = false;
    bool outputTradeLog = false;
    bool outputAll = false;

    Order *createOrder();
    Limit *createLimit();


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
    void executeOrder(Order* order, std::uint32_t shares);
    void processTrade(Trade *trade);
    void cancelOrder(std::uint32_t idNumber);
};
