//
// Created by julio-martins on 3/20/25.
//

#include "RawSocket.h"

RawSocket::RawSocket(const std::string& interface) : interface_name(interface) {
    // Create a raw socket
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Raw socket created on interface: " << interface << std::endl;
}

RawSocket::~RawSocket() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

bool RawSocket::bindSocket() {
    ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);

    // Get the interface index
    auto ioctlResult = ioctl(sockfd, SIOCGIFINDEX, &ifr);
    if (ioctlResult == -1) {
        perror("Failed to get interface index");
        return false;
    }

    socket_address.sll_ifindex = ifr.ifr_ifindex;
    socket_address.sll_protocol = htons(ETH_P_ALL);

    auto wasBound = bind(sockfd, reinterpret_cast<sockaddr *>(&socket_address), sizeof(socket_address));
    if (wasBound == -1) {
        perror("Binding socket failed");
        return false;
    }

    std::cout << "Socket bound to interface: " << interface_name << std::endl;
    return true;
}

bool RawSocket::getMacAddress() {
    ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);

    // Get the MAC address
    auto ioctlResult = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
    if (ioctlResult == -1) {
        perror("Failed to get MAC address");
        return false;
    }

    memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
    return true;
}

void RawSocket::printMacAddress() const {
    printf("MAC Address of %s: %02X:%02X:%02X:%02X:%02X:%02X\n", interface_name.c_str(),
           mac_address[0], mac_address[1], mac_address[2],
           mac_address[3], mac_address[4], mac_address[5]);
}