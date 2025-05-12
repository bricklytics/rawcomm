//
// Created by julio-martins on 4/30/25.
//

#ifndef STOPANDWAITCONTROLLER_H
#define STOPANDWAITCONTROLLER_H

#include "../../base/include/IFlowController.h"
#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"
#include "../../../../errorcontrollayer/feature/checksum/include/ChecksumStrategy.h"

#define DATA_SIZE_MAX 127               // Maximum data size in a packet in bytes
#define PAKET_SIZE_MAX 127              // Maximum packet size in bytes - including header

#define START_MARK 0x7E                 // 01111110

enum class PacketType {
    ACK = 0x00,                  // Acknowledgment
    NACK = 0x01,                 // Negative acknowledgment
    DATA = 0x02,                 // Data packet
    SIZE = 0x04,                 // Size packet
    TEXT_ACK_NOME = 0x06,        // Text file packet
    MEDIA_ACK_NOME = 0x07,       // Media file packet
    IMAGE_ACK_NOME = 0x08,       // Image file packet
    EOFP = 0x09,                 // End of file packet
    MOVE_RIGHT = 0x0A,           // Move right packet
    MOVE_UP = 0x0B,              // Move up packet
    MOVE_DOWN = 0x0C,            // Move down packet
    MOVE_LEFT = 0x0D,            // Move left packet
    ERROR = 0x0F                 // Error packet
    //Types 0x03, 0x0E are reserved for future use
};


/**
 *  This class implements the Stop-and-Wait flow control protocol.
 *  It is responsible for managing the data packets should be delivered
 *  and acknowledged in a reliable manner.
 *  Also it handles the retransmission of packets in case of loss or corruption.
 *  It applies the ChecksumStrategy to ensure data integrity.
 */
class StopAndWaitController : public IFlowController {

    IBaseSocket *transmitter;
    IErrorControlStrategy *errorControlStrategy;

    uint8_t packet_type;                    //info the type of data being sent
    uint8_t last_seq_num;
    bool listening = true;

    typedef struct kermit_header {
        uint8_t size : 7;
        uint8_t seq_num: 5;
        uint8_t type: 4;
        uint8_t checksum: 8;
    } PacketHeader;

    typedef struct kermit_packet {
        uint8_t start_mark;
        PacketHeader header;
        std::vector<uint8_t> data;
    } Packet;

    uint8_t nextSeqNum() const;
    bool waitForAck(uint8_t seq_num);

    void sendAck(uint8_t seq_num);
    void sendNack(uint8_t seq_num);

    static std::vector<uint8_t> serializeHeader(const PacketHeader& header);
    static PacketHeader deserializeHeader(const std::vector<uint8_t>& buffer);

public:

    StopAndWaitController();
    StopAndWaitController(IBaseSocket *transmitter);

    ~StopAndWaitController() override;

    /**
     * Modify the packet type for the Stop-and-Wait protocol.
     * @param type the packet type to be set
     */
    void setPacketType(PacketType type);

    /**
     * Send data reliably (with retransmission and acknowledgment)
     * @param data the data to be sent
     * @return  true if the data was sent successfully, false otherwise
     */
    bool dispatch(const std::vector<uint8_t> &data) override;

    /**
     * Receive a packet and handle it sending ACK/NACK
     * @return  a vector of bytes containing the received packet
     */
    std::vector<uint8_t> receive() override;

    /**
     * Notify the controller of delivery status (e.g., ACK or NACK)
     * This method is called whenever an ACK or NACK is required.
     */
    void notify() override;
};

#endif //STOPANDWAITCONTROLLER_H
