//
// Created by julio-martins on 4/16/25.
//

#ifndef IERRORDETECTOR_H
#define IERRORDETECTOR_H

#include <vector>
#include <cstdint>

class IErrorControlStrategy {
public:
    virtual ~IErrorControlStrategy() = default;

    // Computes the error detection code for the given data
    virtual uint8_t generate(const std::vector<uint8_t>& data) const = 0;

    // Verifies if the data + checksum is valid
    virtual bool assert(const std::vector<uint8_t>& data) const = 0;

};

#endif //IERRORDETECTOR_H
