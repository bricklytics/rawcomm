//
// Created by julio-martins on 4/12/25.
//

#ifndef IFLOWCONTROLER_H
#define IFLOWCONTROLER_H

#include <vector>
#include <cstdint>

class IFlowController {
public:
    virtual ~IFlowController() = default;

    // Send data reliably (with retransmission and acknowledgment)
    virtual bool dispatch(const std::vector<uint8_t>& data) = 0;

    // Receive a packet and handle it sending ACK/NACK
    virtual std::vector<uint8_t> receive() = 0;

};
#endif //IFLOWCONTROLER_H
