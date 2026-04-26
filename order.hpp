#pragma once
#include <cstdint>

struct Limit;

struct Order {
    std::uint32_t idNumber = 0;
    bool buyOrSell = false;
    std::uint32_t shares = 0;
    std::uint32_t limitPrice = 0;
    std::uint64_t entryTime = 0;
    std::uint64_t eventTime = 0;
    Order *nextOrder = nullptr;
    Order *prevOrder = nullptr;
    Limit *parentLimit = nullptr;
};
