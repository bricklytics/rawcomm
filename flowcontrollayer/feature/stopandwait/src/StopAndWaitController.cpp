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
    this->packet_type = PacketUtils::toUint8(PacketUtils::PacketType::ACK);
    this->current_seq = 0xFF;
}

StopAndWaitController::StopAndWaitController(IBaseSocket *transmitter) {
    this->errorControlStrategy = new ChecksumStrategy();
    this->transmitter = transmitter; // Use loopback interface as default
    this->transmitter->setTimeout(TIMEOUT_SECONDS, 0); // Set timeout for receiving
    this->packet_type = PacketUtils::toUint8(PacketUtils::PacketType::ACK);
    this->current_seq = 0xFF;
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
    PacketUtils::PacketHeader header{};
    header.size = data.size();
    current_seq = nextSeqNum();
    header.seq_num = current_seq;
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
    int retries = 0;
    do {
        if (transmitter->sendData(packet))
            hasSucceeded = waitForAck(header.seq_num); // Block until ACK/NACK is received

        if (retries >= RETRIES) return hasSucceeded;
        retries++;
    } while (!hasSucceeded);

    return hasSucceeded;
}

std::vector<uint8_t> StopAndWaitController::receive() {
    bool hasSucceeded = false;
    std::vector<uint8_t> checksumData, data;
    PacketUtils::PacketHeader header{};

    do {
        auto pckt = transmitter->receiveData();
        if (pckt.empty()) return pckt; // Timeout or error
        if (pckt[0] != START_MARK) continue;

        //Prepare header
        auto header_buffer = std::vector(pckt.begin() + 1, pckt.begin() + sizeof(PacketUtils::PacketHeader));
        header = deserializeHeader(header_buffer);

        // Recover data for checksum
        checksumData.insert(checksumData.end(), pckt.begin() + 1, pckt.end());
        data.insert(data.end(), pckt.begin() + sizeof(PacketUtils::PacketHeader), pckt.end());

        //should control if the packet seq_num is the expected one - send error packet
        if (!errorControlStrategy->assert(checksumData)) sendNack(header.seq_num);
        else hasSucceeded = true;
    } while (!hasSucceeded);

    sendAck(header.seq_num);
    packet_type = header.type;
    current_seq = header.seq_num;

    return data;
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
    while (!hasSucceeded) {
        auto pckt = transmitter->receiveData();

        if (pckt.empty()) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; //timeout
            continue;
        }
        if (pckt[0] != START_MARK) continue;

        //Prepare header
        PacketUtils::PacketHeader header;
        auto header_buffer = std::vector(pckt.begin() + 1, pckt.begin() + sizeof(PacketUtils::PacketHeader));
        header = deserializeHeader(header_buffer);

        // Recover data for checksum
        std::vector<uint8_t> checksumData = serializeHeader(header);
        if (errorControlStrategy->assert(checksumData) &&
            header.seq_num == seq_num &&
            header.type == PacketUtils::toUint8(PacketUtils::PacketType::ACK)
        ) hasSucceeded = true;
        if (header.type == PacketUtils::toUint8(PacketUtils::PacketType::NACK)) break;
    }

    return hasSucceeded;
}

void StopAndWaitController::sendAck(uint8_t seq_num) {
    PacketUtils::PacketHeader ackHeader{};
    ackHeader.size = 0;
    ackHeader.seq_num = seq_num;
    ackHeader.type = PacketUtils::toUint8(PacketUtils::PacketType::ACK);
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
    PacketUtils::PacketHeader ackHeader{};
    ackHeader.size = 0;
    ackHeader.seq_num = seq_num;
    ackHeader.type = PacketUtils::toUint8(PacketUtils::PacketType::NACK);
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
