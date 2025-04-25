//
// Created by julio-martins on 3/29/25.
//

#ifndef DATATRANSFERRAWSOCKET_H
#define DATATRANSFERRAWSOCKET_H

#include "../../base/include/IBaseSocket.h"
#include "../../socketsetup/include/RawSocket.h"
#include <linux/if_packet.h>
#include <iostream>
#include <linux/if_ether.h>

#define PACKET_SIZE 1500
#define RETRIES 3
#define TIMEOUT_SECONDS 10
#define CUSTOM_ETHERTYPE 0x88b5

class DataTransferRawSocket : public IBaseSocket {
    RawSocket *rawSocket;
    std::string interface_name;
    std::vector<uint8_t> source_macadd;
    std::vector<uint8_t> dest_macadd;

    bool syncCommChannel();

public:
    explicit DataTransferRawSocket(std::string  interface);
    ~DataTransferRawSocket() override;

    /**
     * Bind the socket to the specified interface.
     * @return True if binding was successful, false otherwise.
     */
    bool openSocket() override;

    /**
     * Set the timeout for receiving data.
     * @param seconds Number of seconds for the timeout.
     * @param microseconds Number of microseconds for the timeout.
     */
    void setTimeout(int seconds, int microseconds) override;

    /**
     * Send data over the socket.
     * @param payload The data to send.
     * @return True if the data was sent successfully, false otherwise.
     */
    bool sendData(const std::vector<uint8_t>& payload) override;

    /**
     * Listen the socket for incoming data.
     * @return The received data.
     */
    std::vector<uint8_t> receiveData() override;
};

#endif //DATATRANSFERRAWSOCKET_H
