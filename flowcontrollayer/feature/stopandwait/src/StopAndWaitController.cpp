//
// Created by julio-martins on 5/1/25.
//

#include <cstring>
#include <iostream>

#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"
#include "../include/StopAndWaitController.h"
#include "../../base/include/PacketType.h"

StopAndWaitController::StopAndWaitController(IBaseSocket *transmitter) {
    this->errorControlStrategy = new ChecksumStrategy();
    this->transmitter = transmitter; // Use loopback interface as default
    this->transmitter->setTimeout(TIMEOUT_SECONDS); // Set timeout for receiving
    this->packet_type = PacketUtils::toUint8(PacketUtils::PacketType::ACK);
    this->current_seq = 0xFF;
}

StopAndWaitController::~StopAndWaitController() {
    delete errorControlStrategy;
}

bool StopAndWaitController::dispatch(const PacketUtils::Packet &packet) {
    if (packet.header.size > DATA_SIZE_MAX) {
        std::cerr << "Data size too large" << std::endl;
        return false;
    }

    //Prepare header
    PacketUtils::Packet packet_temp{};

    packet_temp.start_mark = START_MARK;
    packet_temp.header.size = packet.header.size;
    current_seq = nextSeqNum();
    packet_temp.header.seq_num = current_seq;
    packet_temp.header.type = packet.header.type;
    packet_temp.header.checksum = 0;
    packet_temp.data = {};

    auto checksumData = serializeHeader(packet_temp.header);
    checksumData.insert(checksumData.end(), packet.data.begin(), packet.data.end());
    packet_temp.header.checksum = errorControlStrategy->generate(checksumData);

    //serialize packet
    auto buffer = serializeHeader(packet_temp.header);
    buffer.insert(buffer.begin(), packet_temp.start_mark);
    buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());

    // Send the packet
    bool hasSucceeded = false;
    int retries = 0;
    do {
        if (transmitter->sendData(buffer))
            hasSucceeded = waitForAck(packet_temp.header.seq_num);

        if (retries > RETRIES) return hasSucceeded;
        retries++;
    } while (!hasSucceeded);

    return hasSucceeded;
}

PacketUtils::Packet StopAndWaitController::receive() {
    bool hasSucceeded = false;
    PacketUtils::Packet packet{};

    while (!hasSucceeded) {
        auto pckt = transmitter->receiveData();
        if (pckt.empty())  return {};

        //Prepare header
        packet.start_mark = pckt[0];
        if (packet.start_mark != START_MARK) continue;

        pckt.erase(pckt.begin());
        if (!errorControlStrategy->assert(pckt)) sendNack(packet.header.seq_num);
        else hasSucceeded = true;

        packet.header = deserializeHeader(
            std::vector(pckt.begin(), pckt.begin() + sizeof(PacketUtils::PacketHeader) - 1)
        );
        pckt.erase(pckt.begin(), pckt.begin() + sizeof(PacketUtils::PacketHeader) - 1);
        packet.data.insert(packet.data.begin(), pckt.begin(), pckt.end());
    }

    sendAck(packet.header.seq_num);
    packet_type = packet.header.type;
    current_seq = packet.header.seq_num;

    return packet;
}

void StopAndWaitController::notify() {
    // TODO: Implement this method
}

uint8_t StopAndWaitController::nextSeqNum() const {
    uint8_t mask = (1 << 5) - 1; // 5 bits for sequence number
    uint8_t next = (current_seq + 1) & mask; // Increment and wrap around
    return next;
}

bool StopAndWaitController::waitForAck(uint8_t seq_num) const {
    bool hasSucceeded = false;
    PacketUtils::Packet packet{};

    while (!hasSucceeded) {
        auto pckt = transmitter->receiveData();

        if (pckt.empty()) {
            if (transmitter->hasTimeout) break;
            continue;
        }
        packet.start_mark = pckt[0];
        if (packet.start_mark != START_MARK) continue;

        pckt.erase(pckt.begin());
        packet.header = deserializeHeader(
            std::vector(pckt.begin(),pckt.begin() + sizeof(PacketUtils::PacketHeader))
        );

        if (errorControlStrategy->assert(pckt) &&
            packet.header.seq_num == seq_num &&
            packet.header.type == PacketUtils::toUint8(PacketUtils::PacketType::ACK)
        ) hasSucceeded = true;
        if (packet.header.type == PacketUtils::toUint8(PacketUtils::PacketType::NACK)) break;
    }

    return hasSucceeded;
}

void StopAndWaitController::sendAck(uint8_t seq_num) const {
    PacketUtils::Packet packetAck{};
    packetAck.header.size = 0;
    packetAck.header.seq_num = seq_num;
    packetAck.header.type = PacketUtils::toUint8(PacketUtils::PacketType::ACK);
    packetAck.header.checksum = 0;

    auto buffer = serializeHeader(packetAck.header);
    packetAck.header.checksum = errorControlStrategy->generate(buffer);

    std::vector<uint8_t> ackPacket = serializeHeader(packetAck.header);;
    ackPacket.insert(ackPacket.begin(), START_MARK);

    transmitter->sendData(ackPacket);
}

void StopAndWaitController::sendNack(uint8_t seq_num) const {
    PacketUtils::Packet packetAck{};
    packetAck.header.size = 0;
    packetAck.header.seq_num = seq_num;
    packetAck.header.type = PacketUtils::toUint8(PacketUtils::PacketType::NACK);
    packetAck.header.checksum = 0;

    auto buffer = serializeHeader(packetAck.header);
    packetAck.header.checksum = errorControlStrategy->generate(buffer);

    std::vector<uint8_t> ackPacket = serializeHeader(packetAck.header);;
    ackPacket.insert(ackPacket.begin(), START_MARK);

    transmitter->sendData(ackPacket);
}

std::vector<uint8_t> StopAndWaitController::serializeHeader(const PacketUtils::PacketHeader &header) {
    std::vector<uint8_t> buffer(3, 0);

    // buffer[0]: 7 bits of size + 1 MSB bit of seq_num
    buffer[0] = (header.size & 0x7F) << 1;
    buffer[0] |= (header.seq_num >> 4) & 0x01; // seq_num bit 4

    // buffer[1]: 4 LSB of seq_num + 4 bits of type
    buffer[1] = ((header.seq_num & 0x0F) << 4) | (header.type & 0x0F);

    // buffer[2]: 8 bits of checksum
    buffer[2] = header.checksum;

    return buffer;
}

PacketUtils::PacketHeader StopAndWaitController::deserializeHeader(const std::vector<uint8_t> &buffer) {
    if (buffer.size() < 3) throw std::runtime_error("Buffer too small for packet header");

    PacketUtils::PacketHeader header{};

    // Extract size (7 bits from buffer[0], bits 7..1)
    header.size = (buffer[0] >> 1) & 0x7F;

    // Extract seq_num
    uint8_t seq_msb = buffer[0] & 0x01; // bit 0 of buffer[0]
    uint8_t seq_lsb = (buffer[1] >> 4) & 0x0F; // bits 7..4 of buffer[1]
    header.seq_num = (seq_msb << 4) | seq_lsb;

    // Extract type (lower 4 bits of buffer[1])
    header.type = buffer[1] & 0x0F;

    // Extract checksum
    header.checksum = buffer[2];

    return header;
}
