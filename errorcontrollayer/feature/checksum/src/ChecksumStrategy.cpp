//
// Created by julio-martins on 4/16/25.
//

#include "../include/ChecksumStrategy.h"

uint8_t ChecksumStrategy :: byteSummation(const std::vector<uint8_t>& data) {
    uint8_t byteSum = 0;
    for (const auto& byte : data) {
        byteSum ^= byte; // use XOR operation for byte 1st complement sum
    }
    return byteSum;
}

uint8_t ChecksumStrategy::generate(const std::vector<uint8_t>& data) const {
    uint8_t checksum = byteSummation(data);
    return !checksum; // 2nd complement sum (inverted the checksum)
}

bool ChecksumStrategy::assert(const std::vector<uint8_t>& data) const {
    if (data.empty()) return false;

    const std::vector raw_data(data.begin(), data.end());

    uint8_t sum = byteSummation(raw_data); // calculate checksum from the data

    return (sum ^ 0xFF) == 0x00; // check if the checksum is valid
}