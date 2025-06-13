//
// Created by julio-martins on 4/30/25.
//

#ifndef STOPANDWAITCONTROLLER_H
#define STOPANDWAITCONTROLLER_H

#include <vector>

#include "../../base/include/IFlowController.h"
#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"
#include "../../../../errorcontrollayer/feature/checksum/include/ChecksumStrategy.h"
#include "../../base/include/PacketType.h"

/**
 *  This class implements the Stop-and-Wait flow control protocol.
 *  It is responsible for managing the data packets should be delivered
 *  and acknowledged in a reliable manner.
 *  Also it handles the retransmission of packets in case of loss or corruption.
 *  It applies the ChecksumStrategy to ensure data integrity.
 */
class StopAndWaitController final : public IFlowController {

    IBaseSocket *transmitter;
    IErrorControlStrategy *errorControlStrategy;

    uint8_t nextSeqNum() const;
    bool waitForAck(uint8_t seq_num);

    void sendAck(uint8_t seq_num) const;
    void sendNack(uint8_t seq_num) const;

    static std::vector<uint8_t> serializeHeader(const PacketUtils::PacketHeader& header);
    static PacketUtils::PacketHeader deserializeHeader(const std::vector<uint8_t>& buffer);

public:
    StopAndWaitController(IBaseSocket *transmitter);

    ~StopAndWaitController() override;

    /**
     * Send data reliably (with retransmission and acknowledgment)
     * @param packet the data to be sent
     * @return  true if the data was sent successfully, false otherwise
     */
    bool dispatch(const PacketUtils::Packet &packet) override;

    /**
     * Receive a packet and handle it sending ACK/NACK
     * @return  a packet of bytes containing the received packet
     */
    PacketUtils::Packet receive() override;

    /**
     * Notify the controller of delivery status (e.g., ACK or NACK)
     * This method is called whenever an ACK or NACK is required.
     */
    void notify() override;

    void sendError(int err) override;
};

#endif //STOPANDWAITCONTROLLER_H
