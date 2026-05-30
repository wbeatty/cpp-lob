#pragma once
#include <cstdint>

struct Trade {
    std::uint32_t id;
    std::uint32_t price;
    std::uint32_t shares;
    std::uint64_t executionTime;
    std::uint64_t takerEntryTime;
    std::uint32_t buyerID;
    std::uint32_t sellerID;
};