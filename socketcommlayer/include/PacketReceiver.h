//
// Created by julio-martins on 3/22/25.
//

#ifndef PACKET_RECEIVER_H
#define PACKET_RECEIVER_H

#include "RawSocket.h"

class PacketReceiver final : public RawSocket {
public:
    explicit PacketReceiver(const std::string& interface);

    bool receivePacket(void *buffer, size_t length) const;
};

#endif // PACKET_RECEIVER_H
