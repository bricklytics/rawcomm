//
// Created by julio-martins on 3/20/25.
//

#ifndef PACKET_SENDER_H
#define PACKET_SENDER_H

#include "RawSocket.h"
#include <string>
#include <sys/types.h>

class PacketSender final : public RawSocket {
public:
    explicit PacketSender(const std::string& interface);
    bool sendPacket(const void* buffer, size_t length);
    bool sendPacket(const uint8_t *dest_mac, const void *buffer, size_t length);
};

#endif // PACKET_SENDER_H
