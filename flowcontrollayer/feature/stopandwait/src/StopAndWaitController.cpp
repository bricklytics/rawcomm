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

    std::vector<uint8_t> checksumData(3, 0);
    std::memmove(checksumData.data(), &header, sizeof(PacketHeader));
    checksumData[2] = 0; // Clear checksum field
    checksumData.insert(checksumData.end(), data.begin(), data.end());
    header.checksum = errorControlStrategy->generate(checksumData);
    checksumData[2] = header.checksum;

    // Send the packet
    std::vector<uint8_t> packet(1, 0);
    packet[0] = START_MARK;
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
    PacketHeader header;

    do {
        auto pckt = transmitter->receiveData();
        if (pckt.empty()) return {};
        if (pckt[0] != START_MARK) return {};

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
    this->packet_type = static_cast<uint8_t>(type);
}

bool StopAndWaitController::waitForAck(uint8_t seq_num) {
    auto pckt = transmitter->receiveData();

    if (pckt[0] != START_MARK) return false;

    //Prepare header
    PacketHeader header{};
    std::memmove(&header, pckt.data() + 1, sizeof(PacketHeader));

    // Recover data for checksum
    std::vector<uint8_t> checksumData(3);
    std::memmove(checksumData.data(), &header, sizeof(PacketHeader));

    if (errorControlStrategy->assert(checksumData) &&
        header.seq_num == seq_num &&
        header.type == static_cast<uint8_t>(PacketType::ACK)
    ) return true;

    return false;
}

void StopAndWaitController::sendAck(uint8_t seq_num) {
    PacketHeader ackHeader{};
    ackHeader.size = 0;
    ackHeader.seq_num = seq_num;
    ackHeader.type = static_cast<uint8_t>(PacketType::ACK);
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
    ackHeader.type = static_cast<uint8_t>(PacketType::NACK);
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

    buffer[0] = (header.size & 0x7F);                                           // 7 bits
    buffer[1] = ((header.seq_num & 0x1F) << 3) | ((header.type >> 1) & 0x07);   // 5 + 3 bits
    buffer[2] = ((header.type & 0x01) << 7) | (header.checksum & 0xFF);         // 1 + 7 bits

    return buffer;
}

StopAndWaitController::PacketHeader StopAndWaitController::deserializeHeader(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 3) throw std::runtime_error("Buffer too small for packet header");

    PacketHeader header{};

    header.size = buffer[0] & 0x7F;                                         // 7 bits for size
    header.seq_num = (buffer[1] >> 3) & 0x1F;                               // 5 bits for seq_num
    header.type = ((buffer[1] & 0x07) << 1) | ((buffer[2] >> 7) & 0x01);    // 4 bits for type
    header.checksum = buffer[2] & 0xFF;                                     // 8 bits for checksum

    return header;
}