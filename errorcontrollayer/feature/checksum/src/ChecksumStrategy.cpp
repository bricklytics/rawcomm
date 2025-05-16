//
// Created by julio-martins on 4/16/25.
//

#include "../include/ChecksumStrategy.h"

#include <numeric>

uint8_t ChecksumStrategy::generate(const std::vector<uint8_t>& data) const {
    uint8_t checksum = std::accumulate(data.begin(), data.end(), 0);
    return -checksum; // 1st complement sum (inverted the checksum)
}

bool ChecksumStrategy::assert(const std::vector<uint8_t>& data) const {
    if (data.empty()) return false;

    uint8_t sum = std::accumulate(data.begin(), data.end(), 0);
    return (sum & 0xFF) == 0;
}