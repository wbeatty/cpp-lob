#pragma once
#include <cstdint>

struct Limit;

struct Order {
    Order() = default;
    Order(std::uint64_t entryTime, std::uint64_t eventTime, Order *nextOrder, Order *prevOrder, Limit *parentLimit, std::uint32_t idNumber, std::uint32_t shares, std::uint32_t limitPrice, bool buyOrSell) : entryTime(entryTime), eventTime(eventTime), nextOrder(nextOrder), prevOrder(prevOrder), parentLimit(parentLimit), idNumber(idNumber), shares(shares), limitPrice(limitPrice), buyOrder(buyOrSell) {}

    std::uint64_t entryTime = 0;
    std::uint64_t eventTime = 0;
    Order *nextOrder = nullptr;
    Order *prevOrder = nullptr;
    Limit *parentLimit = nullptr;
    std::uint32_t idNumber = 0;
    std::uint32_t shares = 0;
    std::uint32_t limitPrice = 0;
    bool buyOrder = false; // False if Sell, True if Buy
};
