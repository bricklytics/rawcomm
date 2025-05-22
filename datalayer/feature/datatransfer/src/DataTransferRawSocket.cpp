//
// Created by julio-martins on 3/29/25.
//

#include "../include/DataTransferRawSocket.h"
#include <netinet/in.h>
#include <cstring>

DataTransferRawSocket::DataTransferRawSocket(
    std::string interface
) : interface_name(std::move(interface)) {
    rawSocket = nullptr;
    readfds = {};
    timeout = 0;
    hasTimeout = false;
    source_macadd = std::vector<uint8_t>(6);
    dest_macadd = std::vector<uint8_t>(6);
}

DataTransferRawSocket::~DataTransferRawSocket() {
    delete rawSocket;
}

bool DataTransferRawSocket::syncCommChannel() {
    std::cout << "Waiting for source MAC address..." << std::endl;
    if (!rawSocket->getSourceMacAddress()) {
        std::cerr << "Failed to get source MAC address!" << std::endl;
        return false;
    }
    std::cout << "Waiting for destination MAC address..." << std::endl;
    if (!rawSocket->getTargetMacAddress()) {
        std::cerr << "Failed to request destination MAC address!" << std::endl;
        return false;
    }
    source_macadd = rawSocket->getSourceMac();
    dest_macadd = rawSocket->getDestMac();

    return true;
}

bool DataTransferRawSocket::openSocket() {
    // Create a raw socket
    rawSocket = new RawSocket(interface_name);

    if (rawSocket->getSocket() < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (!rawSocket->bindSocket()) {
        std::cerr << "Failed to bind socket!" << std::endl;
        return false;
    }

    if (!syncCommChannel()) {
        std::cerr << "Failed to establish a route under interface: " << interface_name << std::endl;
        return false;
    }

    std::cout << "Raw socket created on interface: " << interface_name << std::endl;
    return true;
}

void DataTransferRawSocket::setTimeout(int seconds) {
    timeout = seconds;
    rawSocket->setTimeout(seconds);
}

bool DataTransferRawSocket::sendData(const std::vector<uint8_t> &payload) {
    ethhdr eth_header{};
    memcpy(eth_header.h_dest, dest_macadd.data(), ETH_ALEN);
    memcpy(eth_header.h_source, source_macadd.data(), ETH_ALEN);
    eth_header.h_proto = htons(CUSTOM_ETHERTYPE); // Custom EtherType

    std::vector<uint8_t> buffer(sizeof(eth_header));
    memcpy(buffer.data(), &eth_header, sizeof(eth_header));
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    sockaddr_ll socket_address{};
    memcpy(&socket_address, rawSocket->getSockaddr(), sizeof(socket_address));

    // Send the packet
    hasTimeout = false;
    auto isSent = sendto(
        rawSocket->getSocket(),
        buffer.data(),
        buffer.size(),
        0,
        reinterpret_cast<sockaddr *>(&socket_address),
        sizeof(socket_address)
    );

    if (isSent < 0) {
        perror("Packet sending failed");
        return false;
    }
    std::cout << "Packet sent successfully!" << std::endl;
    return true;
}

int DataTransferRawSocket::listenSocket() {
    readfds.fd = rawSocket->getSocket();
    readfds.events = POLLIN;

    return poll(&readfds, 1, timeout*1000);
}

std::vector<uint8_t> DataTransferRawSocket::receiveData() {
     auto ready = listenSocket();
    auto buffer = std::vector<uint8_t>(PACKET_SIZE);
    if (ready == 0) {
        hasTimeout = true;
        std::cerr << "Timeout!" << std::endl;

        buffer.clear();
        return buffer;
    }

    if (ready > 0 && readfds.revents & POLLIN) {
        auto bytes_received = recvfrom(
            rawSocket->getSocket(),
            buffer.data(),
            buffer.size(),
            0,
            nullptr,
            nullptr
        );

        if (bytes_received < 0) {
            std::cerr << "Packet receiving failed" << std::endl;
            buffer.clear();
            return buffer;
        }
    }

    ethhdr eth_header{};
    memcpy(&eth_header, buffer.data(), sizeof(eth_header));

    if (
        memcmp(eth_header.h_dest, source_macadd.data(), ETH_ALEN) != 0 &&
        memcmp(eth_header.h_source, dest_macadd.data(), ETH_ALEN) != 0
    ) {
        buffer.clear();
        return buffer;
    }

    std::vector payload(buffer.begin() + sizeof(eth_header), buffer.end());
    return payload;
}
