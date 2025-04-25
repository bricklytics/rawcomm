//
// Created by julio-martins on 3/20/25.
//

#include "../include/RawSocket.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <utility>

RawSocket::RawSocket(std::string interface) : interface_name(std::move(interface)) {
    source_macadd = std::vector<uint8_t>(6);
    dest_macadd = std::vector<uint8_t>(6);
    memset(&socket_address, 0, sizeof(sockaddr_ll));

    // Create a raw socket
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
}

RawSocket::~RawSocket() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

int RawSocket::getSocket() const {
    return sockfd;
}

sockaddr_ll RawSocket::getSockaddr() const {
    return socket_address;
}

std::vector<uint8_t> RawSocket::getSourceMac() const {
    return source_macadd;
}

std::vector<uint8_t> RawSocket::getDestMac() const {
    return dest_macadd;
}

bool RawSocket :: bindSocket() {
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

    packet_mreq packet_mr{};
    packet_mr.mr_ifindex = ifr.ifr_ifindex;
    packet_mr.mr_type = PACKET_MR_PROMISC;

    auto granttedPromisc= setsockopt(
        sockfd,
        SOL_PACKET,
        PACKET_ADD_MEMBERSHIP,
        &packet_mr,
        sizeof(packet_mr)
    );
    if ( granttedPromisc < 0 ) {
        perror("Failed to set socket packet promiscuous option");
        return false;
    }

    std::cout << "Socket bound to interface: " << interface_name << std::endl;
    return true;
}

void RawSocket::setTimeout(int seconds, int microseconds) const {
    timeval timeout{};
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set socket timeout");
    } else {
        std::cout << "Socket timeout set to " << seconds << "s " << microseconds << "μs" << std::endl;
    }
}

bool RawSocket :: getSourceMacAddress() {
    ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);

    // Get the MAC address
    auto ioctlResult = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
    if (ioctlResult < 0) {
        perror("Failed to get MAC address");
        return false;
    }

    memcpy(source_macadd.data(), ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    printf("Source MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           source_macadd[0], source_macadd[1], source_macadd[2],
           source_macadd[3], source_macadd[4], source_macadd[5]);

    return true;
}

void RawSocket :: sendMacaddRequest(const std::vector<uint8_t> &frame, sockaddr_ll socket_addr) const {
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

bool RawSocket :: setupBroadcast(ethhdr &eth_header, sockaddr_ll &socket_addr) const {
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
    eth_header = {};
    memcpy(eth_header.h_dest, dest_mac.data(), ETH_ALEN);
    memcpy(eth_header.h_source, source_macadd.data(), ETH_ALEN);
    eth_header.h_proto = htons(CUSTOM_ETHERTYPE);  // Custom EtherType

    socket_addr = {};
    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sll_ifindex = ifr.ifr_ifindex;
    socket_addr.sll_halen = ETH_ALEN;
    memcpy(socket_addr.sll_addr, dest_mac.data(), dest_mac.size());

    return true;
}

bool RawSocket :: getTargetMacAddress() {
    ethhdr eth_header{};
    sockaddr_ll socket_addr{};

    bool isSuccess = setupBroadcast(eth_header, socket_addr);
    if (!isSuccess) return false;

    // Prepare payload (can be empty or include a probe message)
    std::string payload = "MAC_REQUEST";
    std::vector<uint8_t>frame(sizeof(eth_header) + payload.size());

    memcpy(frame.data(), &eth_header, sizeof(eth_header));
    frame.insert(frame.end(), payload.begin(), payload.end());

    sendMacaddRequest(frame, socket_addr);
    std::cout << "Probe sent, waiting for response..." << std::endl;

    // Listen for response
    uint8_t recv_buf[1024];
    auto retries = 0;
    while (true) {
        ssize_t recv_len = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
        if (recv_len <= 0) {
            perror("Failed to receive response! Retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            retries++;
            if (retries > RETRIES) {
                std::cerr << "Failed to establish connection after " << RETRIES << " retries." << std::endl;
                return false;
            }
            continue;
        }
        sendMacaddRequest(frame, socket_addr);

        // Check if this is our custom EtherType response
        auto *recv_eth = reinterpret_cast<ethhdr *>(recv_buf);
        if (ntohs(recv_eth->h_proto) == CUSTOM_ETHERTYPE) {
            memcpy(dest_macadd.data(), recv_eth->h_source, ETH_ALEN);
            printf("Received response from MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      dest_macadd[0], dest_macadd[1], dest_macadd[2],
                      dest_macadd[3], dest_macadd[4], dest_macadd[5]
            );
            break;
        }
    }
    return true;
}
