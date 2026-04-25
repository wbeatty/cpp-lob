#pragma once
#include <cstdint>

struct Order; // forward declaration — Limit only holds pointers

struct Limit {
    std::uint32_t limitPrice;
    std::uint32_t size;
    std::uint32_t totalVolume;
    Limit *parent;
    Limit *leftChild;
    Limit *rightChild;
    Order *headOrder;
    Order *tailOrder;
};
