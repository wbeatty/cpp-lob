#pragma once
#include <cstdint>

struct Order;

struct Limit {
    std::uint32_t limitPrice = 0;
    std::uint32_t size = 0;
    std::uint32_t totalVolume = 0;
    Limit *parent = nullptr;
    Limit *leftChild = nullptr;
    Limit *rightChild = nullptr;
    Order *headOrder = nullptr;
    Order *tailOrder = nullptr;

    void addOrder(Order& order);
    void removeOrder(Order* order);
};
