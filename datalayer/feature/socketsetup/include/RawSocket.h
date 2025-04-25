//
// Created by julio-martins on 3/20/25.
//

#ifndef RAW_SOCKET_H
#define RAW_SOCKET_H

#include <netinet/if_ether.h>
#include <linux/if_packet.h>
#include <string>
#include <vector>

#define PACKET_SIZE 1500
#define RETRIES 3
#define TIMEOUT_SECONDS 10
#define CUSTOM_ETHERTYPE 0x88b5

class RawSocket final {
    int sockfd;
    sockaddr_ll socket_address{};
    std::string interface_name;
    std::vector<uint8_t> source_macadd;
    std::vector<uint8_t> dest_macadd;

    void sendMacaddRequest(const std::vector<uint8_t> &frame, sockaddr_ll socket_addr) const;

    bool setupBroadcast(ethhdr &eth_header, sockaddr_ll &socket_addr) const;

public:
    explicit RawSocket(std::string interface);

    ~RawSocket();

    bool bindSocket();

    void setTimeout(int seconds, int microseconds) const;

    bool getSourceMacAddress();

    bool getTargetMacAddress();

    [[nodiscard]] int getSocket() const;

    [[nodiscard]] const sockaddr_ll* getSockaddr() const;

    [[nodiscard]] std::vector<uint8_t> getSourceMac() const;

    [[nodiscard]] std::vector<uint8_t> getDestMac() const;
};

#endif // RAW_SOCKET_H
