//
// Created by julio-martins on 5/1/25.
//

#include "../include/StopAndWaitController.h"

#include <cstring>
#include <iostream>
#include <thread>

#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"

StopAndWaitController::StopAndWaitController() {
    this->errorControlStrategy = new ChecksumStrategy();
    this->transmitter = new DataTransferRawSocket("lo"); // Use loopback interface as default
    this->transmitter->setTimeout(TIMEOUT_SECONDS, 0); // Set timeout for receiving
    this->packet_type = static_cast<uint8_t>(PacketType::ACK);
    this->last_seq_num = 0xFF;
}

StopAndWaitController::StopAndWaitController(IBaseSocket *transmitter) {
    this->errorControlStrategy = new ChecksumStrategy();
    this->transmitter = transmitter; // Use loopback interface as default
    this->transmitter->setTimeout(TIMEOUT_SECONDS, 0); // Set timeout for receiving
    this->packet_type = static_cast<uint8_t>(PacketType::ACK);
    this->last_seq_num = 0xFF;
}

StopAndWaitController::~StopAndWaitController() {
    delete errorControlStrategy;
}

bool StopAndWaitController::dispatch(const std::vector<uint8_t> &data) {
    if (data.size() > DATA_SIZE_MAX) {
        std::cerr << "Data size too large" << std::endl;
        return false;
    }
    if (data.empty()) return false;

    //Prepare header
    PacketHeader header{};

    header.size = data.size();
    last_seq_num = nextSeqNum();
    header.seq_num = last_seq_num;
    header.type = packet_type;
    header.checksum = 0;

    std::vector<uint8_t> checksumData = serializeHeader(header);
    checksumData.insert(checksumData.end(), data.begin(), data.end());
    header.checksum = errorControlStrategy->generate(checksumData);
    checksumData[2] = header.checksum;

    // Send the packet
    std::vector<uint8_t> packet;
    packet.push_back(START_MARK);
    packet.insert(packet.end(), checksumData.begin(), checksumData.end());

    // Send the packet
    bool hasSucceeded = false;
    do {
        transmitter->sendData(packet);
        hasSucceeded = waitForAck(header.seq_num); // Block until ACK/NACK is received
        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } while (!hasSucceeded);

    return true;
}

std::vector<uint8_t> StopAndWaitController::receive() {
    bool hasSucceeded = false;
    std::vector<uint8_t> checksumData, data;
    PacketHeader header{};

    do {
        auto pckt = transmitter->receiveData();
        if (pckt.empty()) continue;
        if (pckt[0] != START_MARK) continue;

        //Prepare header
        auto header_buffer = std::vector(pckt.begin() + 1, pckt.begin() + sizeof(PacketHeader));
        header = deserializeHeader(header_buffer);

        // Recover data for checksum
        checksumData.insert(checksumData.end(), pckt.begin() + 1, pckt.end());
        data.insert(data.end(), pckt.begin() + sizeof(PacketHeader), pckt.end());

        if (!errorControlStrategy->assert(checksumData)) sendNack(header.seq_num);
        else hasSucceeded = true;
    } while (!hasSucceeded);

    sendAck(header.seq_num);

    return data;
}

void StopAndWaitController::notify() {
    // TODO: Implement this method
}

uint8_t StopAndWaitController::nextSeqNum() const {
    uint8_t mask = (1 << 5) - 1;                    // 5 bits for sequence number
    uint8_t next = (last_seq_num + 1) & mask;       // Increment and wrap around
    return next;
}

void StopAndWaitController::setPacketType(PacketType type) {
    this->packet_type = toUint8(type);
}

bool StopAndWaitController::waitForAck(uint8_t seq_num) {
    auto pckt = transmitter->receiveData();

    if (pckt[0] != START_MARK) return false;

    //Prepare header
    PacketHeader header;
    auto header_buffer = std::vector(pckt.begin() + 1, pckt.begin() + sizeof(PacketHeader));
    header = deserializeHeader(header_buffer);

    // Recover data for checksum
    std::vector<uint8_t> checksumData = serializeHeader(header);
    if (errorControlStrategy->assert(checksumData) &&
        header.seq_num == seq_num &&
        header.type == toUint8(PacketType::ACK)
    ) return true;

    return false;
}

void StopAndWaitController::sendAck(uint8_t seq_num) {
    PacketHeader ackHeader{};
    ackHeader.size = 0;
    ackHeader.seq_num = seq_num;
    ackHeader.type = toUint8(PacketType::ACK);
    ackHeader.checksum = 0;
    packet_type = ackHeader.type;

    auto buffer = serializeHeader(ackHeader);
    ackHeader.checksum = errorControlStrategy->generate(buffer);
    buffer.clear();
    buffer = serializeHeader(ackHeader);

    std::vector<uint8_t> ackPacket;
    ackPacket.push_back(START_MARK);
    ackPacket.insert(ackPacket.end(), buffer.begin(), buffer.end());

    std::cout << "Sending ACK for seq_num: " << static_cast<int>(seq_num) << std::endl;
    transmitter->sendData(ackPacket);
}

void StopAndWaitController::sendNack(uint8_t seq_num) {
    PacketHeader ackHeader{};
    ackHeader.size = 0;
    ackHeader.seq_num = seq_num;
    ackHeader.type = toUint8(PacketType::NACK);
    ackHeader.checksum = 0;
    packet_type = ackHeader.type;

    auto buffer = serializeHeader(ackHeader);
    ackHeader.checksum = errorControlStrategy->generate(buffer);
    buffer.clear();
    buffer = serializeHeader(ackHeader);

    std::vector<uint8_t> ackPacket;
    ackPacket.push_back(START_MARK);
    ackPacket.insert(ackPacket.end(), buffer.begin(), buffer.end());

    std::cout << "Sending NACK for seq_num: " << static_cast<int>(seq_num) << std::endl;
    transmitter->sendData(ackPacket);
}

std::vector<uint8_t> StopAndWaitController::serializeHeader(const PacketHeader& header) {
    std::vector<uint8_t> buffer(3, 0);

    // buffer[0]: 7 bits of size + 1 MSB bit of seq_num
    buffer[0] = (header.size & 0x7F) << 1;
    buffer[0] |= (header.seq_num >> 4) & 0x01;  // seq_num bit 4

    // buffer[1]: 4 LSB of seq_num + 4 bits of type
    buffer[1] = ((header.seq_num & 0x0F) << 4) | (header.type & 0x0F);

    // buffer[2]: 8 bits of checksum
    buffer[2] = header.checksum;

    return buffer;
}

StopAndWaitController::PacketHeader StopAndWaitController::deserializeHeader(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 3) throw std::runtime_error("Buffer too small for packet header");

    PacketHeader header{};

    // Extract size (7 bits from buffer[0], bits 7..1)
    header.size = (buffer[0] >> 1) & 0x7F;

    // Extract seq_num
    uint8_t seq_msb = buffer[0] & 0x01;                // bit 0 of buffer[0]
    uint8_t seq_lsb = (buffer[1] >> 4) & 0x0F;         // bits 7..4 of buffer[1]
    header.seq_num = (seq_msb << 4) | seq_lsb;

    // Extract type (lower 4 bits of buffer[1])
    header.type = buffer[1] & 0x0F;

    // Extract checksum
    header.checksum = buffer[2];

    return header;
}


uint8_t StopAndWaitController::toUint8(PacketType type) {
    switch (type) {
            case PacketType::ACK: return 0x00;
            case PacketType::NACK: return 0x01;
            case PacketType::DATA: return 0x02;
            case PacketType::SIZE: return 0x04;
            case PacketType::TEXT_ACK_NOME: return 0x06;
            case PacketType::MEDIA_ACK_NOME: return 0x07;
            case PacketType::IMAGE_ACK_NOME: return 0x08;
            case PacketType::EOFP: return 0x09;
            case PacketType::MOVE_RIGHT: return 0x0A;
            case PacketType::MOVE_UP: return 0x0B;
            case PacketType::MOVE_DOWN: return 0x0C;
            case PacketType::MOVE_LEFT: return 0x0D;
            default: return 0x0F; // Error packet
    }
}