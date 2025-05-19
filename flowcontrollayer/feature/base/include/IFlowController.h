//
// Created by julio-martins on 4/12/25.
//

#ifndef IFLOWCONTROLER_H
#define IFLOWCONTROLER_H

#include <vector>
#include <cstdint>
#include "PacketType.h"

class IFlowController {
public:
    uint8_t packet_type = 0x00;
    uint8_t current_seq = 0xFF;

    virtual ~IFlowController();

    virtual bool dispatch(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> receive() = 0;
    virtual void notify() = 0;
};
#endif //IFLOWCONTROLER_H
