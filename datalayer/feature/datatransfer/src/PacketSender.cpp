//
// Created by julio-martins on 3/20/25.
//

#include "PacketSender.h"

PacketSender::PacketSender(const std::string &interface) : RawSocket(interface) {
    if (!getMacAddress()) {
        std::cerr << "Failed to retrieve MAC address for interface: " << interface << std::endl;
        exit(EXIT_FAILURE);
    }
    printMacAddress();
}

bool PacketSender::sendPacket(const void *buffer, size_t length) {
    auto isSent = sendto(
        sockfd,
        buffer,
        length,
        0,
        reinterpret_cast<struct sockaddr *>(&socket_address),
        sizeof(socket_address)
    );

    if (isSent < 0) {
        perror("Packet sending failed");
        return false;
    }
    std::cout << "Packet sent successfully!" << std::endl;
    return true;
}

bool PacketSender::sendPacket(const uint8_t *dest_mac, const void *buffer, size_t length) {
    uint8_t packet[1500];
    auto *eth_header = reinterpret_cast<struct ether_header *>(packet);

    // Set destination MAC address
    memcpy(eth_header->ether_dhost, dest_mac, 6);
    // Set source MAC address
    memcpy(eth_header->ether_shost, mac_address, 6);
    // Set Ethernet type (custom or standard)
    eth_header->ether_type = htons(0x0800); // IPv4 type

    // Copy payload
    memcpy(packet + sizeof(ether_header), buffer, length);

    // Send the packet
    auto wasSent = sendto(
        sockfd,
        packet,
        length + sizeof(ether_header),
        0,
        reinterpret_cast<sockaddr *>(&socket_address),
        sizeof(socket_address)
    );
    if (wasSent < 0) {
        perror("Packet sending failed");
        return false;
    }
    std::cout << "Packet sent successfully!" << std::endl;
    return true;
}
