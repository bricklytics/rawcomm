//
// Created by julio-martins on 3/22/25.
//

#include "PacketReceiver.h"

PacketReceiver::PacketReceiver(const std::string& interface) : RawSocket(interface) {}

bool PacketReceiver::receivePacket(void* buffer, size_t length) const {
    ssize_t bytes_received = recvfrom(sockfd, buffer, length, 0, nullptr, nullptr);
    if (bytes_received < 0) {
        perror("Packet receiving failed");
        return false;
    }
    std::cout << "Packet received! Bytes: " << bytes_received << std::endl;
    return true;
}

void PacketReceiver::setTimeout(int seconds, int microseconds) const {
    timeval timeout{};
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set socket timeout");
    } else {
        std::cout << "Socket timeout set to " << seconds << "s " << microseconds << "μs" << std::endl;
    }
}
