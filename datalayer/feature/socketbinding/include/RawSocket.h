//
// Created by julio-martins on 3/20/25.
//

#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>

class RawSocket {
protected:
    int sockfd;
    sockaddr_ll socket_address{};
    std::string interface_name;
    uint8_t mac_address[6]{};

public:
    explicit RawSocket(const std::string& interface);
    virtual ~RawSocket();

    bool bindSocket();
    bool getMacAddress();
    void printMacAddress() const;
};

#endif // RAW_SOCKET_H
