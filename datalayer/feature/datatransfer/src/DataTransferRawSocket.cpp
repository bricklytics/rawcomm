//
// Created by julio-martins on 3/29/25.
//

#include "../include/DataTransferRawSocket.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include <chrono>
#include <thread>
#include <utility>

DataTransferRawSocket::DataTransferRawSocket(std::string interface) : interface_name(std::move(interface)) {
    source_macadd = std::vector<uint8_t>(6);
    dest_macadd = std::vector<uint8_t>(6);
    memset(&socket_address, 0, sizeof(sockaddr_ll));
}

DataTransferRawSocket::~DataTransferRawSocket() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

bool DataTransferRawSocket::bindSocket() {
    ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);

    // Get the interface index
    auto ioctlResult = ioctl(sockfd, SIOCGIFINDEX, &ifr);
    if (ioctlResult < 0) {
        perror("Failed to get interface index");
        return false;
    }

    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_protocol = htons(ETH_P_ALL);

    auto wasBound = bind(sockfd, reinterpret_cast<sockaddr *>(&socket_address), sizeof(socket_address));
    if (wasBound < 0) {
        perror("Binding socket failed");
        return false;
    }

    std::cout << "Socket bound to interface: " << interface_name << std::endl;
    return true;
}

bool DataTransferRawSocket::getSourceMacAddress() {
    ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);

    // Get the MAC address
    auto ioctlResult = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
    if (ioctlResult < 0) {
        perror("Failed to get MAC address");
        return false;
    }

    memcpy(source_macadd.data(), ifr.ifr_hwaddr.sa_data, source_macadd.size());
    printf("Source MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           source_macadd[0], source_macadd[1], source_macadd[2],
           source_macadd[3], source_macadd[4], source_macadd[5]);

    return true;
}

void DataTransferRawSocket::sendMacaddRequest(const std::vector<uint8_t> &frame, sockaddr_ll socket_addr) const {
    auto isRequestSent = sendto(
        sockfd,
        frame.data(),
        frame.size(),
        0,
        reinterpret_cast<struct sockaddr *>(&socket_addr),
        sizeof(socket_addr)
    );
    if (isRequestSent < 0) {
        perror("Failed to send MAC address request");
    }
}

bool DataTransferRawSocket :: getTargetMacAddress() {
    ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ-1);

    auto ioctlResult = ioctl(sockfd, SIOCGIFINDEX, &ifr);
    if ( ioctlResult < 0) {
        perror("Failed to get interface index for MAC address ARP request");
        return false;
    }

    // Prepare Ethernet frame (destination MAC = Broadcast)
    std::vector<uint8_t> dest_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Broadcast
    ethhdr eth_header{};
    memcpy(eth_header.h_dest, dest_mac.data(), dest_mac.size());
    memcpy(eth_header.h_source, source_macadd.data(), source_macadd.size());
    eth_header.h_proto = htons(CUSTOM_ETHERTYPE);  // Custom EtherType

    // Prepare payload (can be empty or include a probe message)
    std::string payload = "MAC_REQUEST";
    //uint8_t frame[sizeof(eth_header) + sizeof(payload)];
    std::vector<uint8_t>frame(sizeof(eth_header) + payload.size());

    memcpy(frame.data(), &eth_header, sizeof(eth_header));
    // memcpy(frame + sizeof(eth_header), payload, sizeof(payload));
    frame.insert(frame.end(), payload.begin(), payload.end());

    // Send the Ethernet frame
    sockaddr_ll socket_addr{};
    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sll_ifindex = ifr.ifr_ifindex;
    socket_addr.sll_halen = ETH_ALEN;
    memcpy(socket_addr.sll_addr, dest_mac.data(), dest_mac.size());

    sendMacaddRequest(frame, socket_addr);
    std::cout << "Probe sent, waiting for response..." << std::endl;

    // Listen for response
    uint8_t recv_buf[1024];
    auto retries = 0;
    while (retries < RETRIES) {
        ssize_t recv_len = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
        if (recv_len <= 0) {
            perror("Failed to receive response! Retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            retries++;
            if (retries < RETRIES) {
                std::cerr << "Failed to establish connection after " << RETRIES << " retries." << std::endl;
                return false;
            }
            continue;
        }
        sendMacaddRequest(frame, socket_addr);

        // Check if this is our custom EtherType response
        auto *recv_eth = reinterpret_cast<ethhdr *>(recv_buf);
        if (ntohs(recv_eth->h_proto) == CUSTOM_ETHERTYPE) {
            memcpy(dest_macadd.data(), recv_eth->h_source, 6);
            printf("Received response from MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      dest_macadd[0], dest_macadd[1], dest_macadd[2],
                      dest_macadd[3], dest_macadd[4], dest_macadd[5]
            );
            break;
        }
    }
    return true;
}

bool DataTransferRawSocket::syncCommChannel() {
    setTimeout(60, 0); // Set timeout for receiving

    std::cout << "Waiting for source MAC address..." << std::endl;
    if (!getSourceMacAddress()) {
        std::cerr << "Failed to get source MAC address!" << std::endl;
        return false;
    }
    std::cout << "Waiting for destination MAC address..." << std::endl;
    if (!getTargetMacAddress()) {
        std::cerr << "Failed to request destination MAC address!" << std::endl;
        return false;
    }
    return true;
}

bool DataTransferRawSocket::openSocket() {
    // Create a raw socket
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return false;
    }

    std::cout << "Raw socket created on interface: " << interface_name << std::endl;

    if (!bindSocket()) {
        std::cerr << "Failed to bind socket!" << std::endl;
        return false;
    }

    if (!syncCommChannel()) {
        std::cerr << "Failed to establish a route under interface: " << interface_name << std::endl;
        return false;
    }

    return true;
}

void DataTransferRawSocket::setTimeout(int seconds, int microseconds) {
    timeval timeout{};
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set socket timeout");
    } else {
        std::cout << "Socket timeout set to " << seconds << "s " << microseconds << "μs" << std::endl;
    }
}

bool DataTransferRawSocket::sendData(const std::vector<uint8_t>& payload) {
    // ethhdr eth_header{};
    // memcpy(eth_header.h_dest, dest_macadd.data(), dest_macadd.size());
    // memcpy(eth_header.h_source, source_macadd.data(), source_macadd.size());
    // eth_header.h_proto = htons(CUSTOM_ETHERTYPE);  // Custom EtherType
    //
    // std::vector<uint8_t> buffer(sizeof(eth_header));
    // memcpy(buffer.data(), &eth_header, sizeof(eth_header));
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    // Send the packet
    auto isSent = sendto(
        sockfd,
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

std::vector<uint8_t> DataTransferRawSocket::receiveData() {
    auto buffer = std::vector<uint8_t>(PACKET_SIZE);
    // socklen_t src_len = sizeof(socket_address);

    auto bytes_received = recvfrom(
        sockfd,
        buffer.data(),
        buffer.size(),
        0,
       nullptr,
       nullptr
    );

    if (bytes_received < 0) {
        perror("Packet receiving failed");
        buffer.clear();
        return buffer;
    }

    // ethhdr eth_header{};
    // memcpy(&eth_header, buffer.data(), sizeof(eth_header));
    // if (
    //     memcmp(eth_header.h_dest, source_macadd.data(), ETH_ALEN) != 0 &&
    //     memcmp(eth_header.h_dest, dest_macadd.data(), ETH_ALEN) != 0
    // ) {
    //     buffer.clear();
    //     return buffer;
    // }
    //
    // buffer.erase(buffer.begin(), buffer.begin() + sizeof(eth_header));
    std::vector payload(buffer.begin(), buffer.end());

    std::cout << "Packet received! Bytes: " << bytes_received << std::endl;
    return buffer;
}