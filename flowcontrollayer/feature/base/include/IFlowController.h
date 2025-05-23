//
// Created by julio-martins on 4/12/25.
//

#ifndef IFLOWCONTROLER_H
#define IFLOWCONTROLER_H

#include <cstdint>

#include "PacketType.h"

class IFlowController {
public:
    uint8_t packet_type = 0x00;
    uint8_t current_seq = 0xFF;

    virtual ~IFlowController() = default;

    virtual bool dispatch(const PacketUtils::Packet& data) = 0;
    virtual PacketUtils::Packet receive() = 0;
    virtual void notify() = 0;
};
#endif //IFLOWCONTROLER_H
