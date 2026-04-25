#pragma once
#include <cstdint>

struct Limit;

struct Order {
    std::uint32_t idNumber;
    bool buyOrSell;
    std::uint32_t shares;
    std::uint32_t limit;
    std::uint32_t entryTime;
    std::uint32_t eventTime;
    Order *nextOrder;
    Order *prevOrder;
    Limit *parentLimit;
};
