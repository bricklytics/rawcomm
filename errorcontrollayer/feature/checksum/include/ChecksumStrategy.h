//
// Created by julio-martins on 4/16/25.
//

#ifndef CHECKSUMSTRATEGY_H
#define CHECKSUMSTRATEGY_H

#include <vector>
#include <cstdint>
#include "../../base/include/IErrorControlStrategy.h"

class ChecksumStrategy final : public IErrorControlStrategy {

    /**
     * Function to compute the byte summation
     * @param data to be summed
     * @return byte sum of the data
     */
    static uint8_t byteSummation(const std::vector<uint8_t>& data);
public:

    /**
     * Function to generate the checksum of the data
     * @param data byte stream without the init marker
     * @return checksum byte
     */
    uint8_t generate(const std::vector<uint8_t>& data) const override;

    /**
     * Function to assess the correctness of the data
     * @param data byte stream without the init marker
     * @return true if data is the expected one, false otherwise
     */
    bool assert(const std::vector<uint8_t>& data) const override;
};
#endif //CHECKSUMSTRATEGY_H
