// Cancel / best-price regression tests for the matching core.
// Drives Market directly through submitAdd/submitCancel (no pipeline threads).
//
//   make test
//
// Build uses -fsanitize=address,undefined, so a use-after-free, null deref or
// out-of-bounds read fails the run even when the asserted values look right.

#include "market.hpp"
#include <cstdio>
#include <cstdlib>

namespace {

int failures = 0;

#define CHECK_EQ(actual, expected, what)                                        \
    do {                                                                        \
        const auto a_ = (actual);                                               \
        const auto e_ = (expected);                                             \
        if (a_ != e_) {                                                         \
            std::printf("  FAIL %s: got %u, expected %u\n", what,               \
                        (unsigned)a_, (unsigned)e_);                            \
            ++failures;                                                         \
        }                                                                       \
    } while (0)

// Each test gets a fresh Market and assigns ids sequentially from 0, which is
// the contract orderVector relies on.
struct Book {
    Market m;
    std::uint32_t nextId = 0;

};
// The id is captured before submitAdd: if the order fully fills it is returned
// to the pool, so it must not be read afterwards.
std::uint32_t addOrder(Book &b, bool buy, std::uint32_t px, std::uint32_t qty) {
    const std::uint32_t id = b.nextId;
    Order *o = b.m.allocateOrder();
    o->buyOrder = buy;
    o->limitPrice = px;
    o->shares = qty;
    o->idNumber = b.nextId++;
    b.m.submitAdd(o);
    return id;
}

void cancelBestSkipsEmptyLevels() {
    std::puts("cancel best bid skips levels left empty in the tree");
    Book b;
    const auto a = addOrder(b, true, 100, 10);
    const auto c = addOrder(b, true, 99, 10);
    addOrder(b, true, 98, 10);
    b.m.submitCancel(c);          // 99 is now an empty level still in the tree
    b.m.submitCancel(a);          // best was 100 -> must skip 99, land on 98
    CHECK_EQ(b.m.getBestBid(), 98u, "best bid");
}

void cancelOnlyOrderEmptiesSide() {
    std::puts("cancel the only order on a side");
    Book b;
    const auto a = addOrder(b, true, 100, 10);
    b.m.submitCancel(a);
    CHECK_EQ(b.m.getBestBid(), 0u, "best bid");
    const auto s = addOrder(b, false, 105, 10);
    b.m.submitCancel(s);
    CHECK_EQ(b.m.getBestAsk(), 0u, "best ask");
}

void cancelAfterFullFillIsNoOp() {
    std::puts("cancel orders that already fully filled");
    Book b;
    const auto buy = addOrder(b, true, 100, 10);
    const auto sell = addOrder(b, false, 100, 10);   // crosses, both filled and freed
    b.m.submitCancel(buy);
    b.m.submitCancel(sell);
    addOrder(b, true, 101, 5);                       // book still usable
    CHECK_EQ(b.m.getBestBid(), 101u, "best bid after re-add");
    CHECK_EQ(b.m.getBestAsk(), 0u, "best ask");
}

void cancelRemainderAfterPartialFill() {
    std::puts("cancel the remainder of a partially filled order");
    Book b;
    const auto buy = addOrder(b, true, 100, 10);
    addOrder(b, false, 100, 4);                      // buy left with 6 shares
    CHECK_EQ(b.m.getBestBid(), 100u, "best bid after partial fill");
    b.m.submitCancel(buy);
    CHECK_EQ(b.m.getBestBid(), 0u, "best bid after cancel");
    addOrder(b, false, 100, 5);                      // must rest, nothing left to hit
    CHECK_EQ(b.m.getBestAsk(), 100u, "best ask");
}

void cancelNonBestLeavesBestAlone() {
    std::puts("cancel a non-best level");
    Book b;
    addOrder(b, true, 100, 10);
    const auto deep = addOrder(b, true, 95, 10);
    b.m.submitCancel(deep);
    CHECK_EQ(b.m.getBestBid(), 100u, "best bid");
}

void cancelUnknownAndTwice() {
    std::puts("cancel an unknown id, and the same id twice");
    Book b;
    const auto a = addOrder(b, true, 100, 10);
    addOrder(b, true, 99, 10);
    b.m.submitCancel(12345);                         // out of range: no-op
    b.m.submitCancel(2);                             // == size(): no-op (old off-by-one)
    b.m.submitCancel(a);
    b.m.submitCancel(a);                             // already cancelled: no-op
    CHECK_EQ(b.m.getBestBid(), 99u, "best bid");
}

void cancelledOrderDoesNotTrade() {
    std::puts("a cancelled order does not trade");
    Book b;
    const auto a = addOrder(b, true, 100, 10);
    addOrder(b, true, 99, 10);
    b.m.submitCancel(a);
    addOrder(b, false, 99, 10);                      // should hit 99, not the cancelled 100
    CHECK_EQ(b.m.getBestBid(), 0u, "best bid (99 consumed)");
    CHECK_EQ(b.m.getBestAsk(), 0u, "best ask (fully filled)");
}

} // namespace

int main() {
    cancelBestSkipsEmptyLevels();
    cancelOnlyOrderEmptiesSide();
    cancelAfterFullFillIsNoOp();
    cancelRemainderAfterPartialFill();
    cancelNonBestLeavesBestAlone();
    cancelUnknownAndTwice();
    cancelledOrderDoesNotTrade();
    if (failures == 0) {
        std::puts("all cancel tests passed");
        return EXIT_SUCCESS;
    }
    std::printf("%d check(s) failed\n", failures);
    return EXIT_FAILURE;
}
