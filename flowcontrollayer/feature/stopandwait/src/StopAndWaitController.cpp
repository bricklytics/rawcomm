//
// Created by julio-martins on 5/1/25.
//

#include "../include/StopAndWaitController.h"

#include <cstring>
#include <iostream>

#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"

StopAndWaitController::StopAndWaitController() {
    this->errorControlStrategy = new ChecksumStrategy();
    this->transmitter = new DataTransferRawSocket("lo"); // Use loopback interface as default
    this->transmitter->setTimeout(TIMEOUT_SECONDS, 0); // Set timeout for receiving
    this->packet_type = 0x00;
    this->last_seq_num = 0xFF;
}

StopAndWaitController::StopAndWaitController(IBaseSocket *transmitter) {
    this->errorControlStrategy = new ChecksumStrategy();
    this->transmitter = transmitter("lo"); // Use loopback interface as default
    this->transmitter->setTimeout(TIMEOUT_SECONDS, 0); // Set timeout for receiving
    this->packet_type = 0x00;
    this->last_seq_num = 0xFF;
}

StopAndWaitController::StopAndWaitController(IBaseSocket* baseSocket, IErrorControlStrategy* errorControl) {
    this->transmitter = baseSocket;
    this->errorControlStrategy = errorControl;
    this->transmitter->setTimeout(TIMEOUT_SECONDS, 0); // Set timeout for receiving
    this->packet_type = 0x00;
    this->last_seq_num = 0xFF;
}

StopAndWaitController::~StopAndWaitController() {
    delete errorControlStrategy;
    delete transmitter;
}

bool StopAndWaitController::dispatch(const std::vector<uint8_t>& data) {
    std::unique_lock lock(mtx);

    if (data.size() > DATA_SIZE_MAX) {
        std::cerr << "Data size too large" << std::endl;
        return false;
    }

    //Prepare header
    PacketHeader header{};
    memset(&header, 1, sizeof(header));

    header.size = data.size();
    last_seq_num = nextSeqNum(last_seq_num);
    header.seq_num = last_seq_num;
    header.type = packet_type;

    std::vector<uint8_t> checksumData;
    checksumData.push_back(header.size);
    checksumData.push_back(header.seq_num);
    checksumData.push_back(header.type);
    checksumData.insert(checksumData.end(), data.begin(), data.end());
    header.checksum = errorControlStrategy->generate(checksumData);

    // Send the packet
    std::vector<uint8_t> packet;
    packet.push_back(START_MARK);
    memcpy(packet.data() + 1, &header, sizeof(header));
    packet.insert(packet.end(), data.begin(), data.end());

    // Send the packet
    bool hasSucceeded = false;
    do {
        transmitter->sendData(packet);
        hasSucceeded = waitForAck(header.seq_num);  // Block until ACK/NACK is received
    } while (!hasSucceeded);

    return true;
}

std::vector<uint8_t> StopAndWaitController::receive() {
    std::unique_lock lock(mtx);

    auto pckt = transmitter->receiveData();
    if (pckt.empty()) return {};

    //Prepare header
    PacketHeader header{};
    memcpy(&header, pckt.data() + 1, sizeof(PacketHeader));

    // Recover data for checksum
    std::vector<uint8_t> checksumData, data;
    checksumData.push_back(header.size);
    checksumData.push_back(header.seq_num);
    checksumData.push_back(header.type);
    data.insert(data.end(), pckt.begin() + sizeof(PacketHeader) + 1 , pckt.end());
    checksumData.insert(checksumData.end(), data.begin(), data.end());

    if (!errorControlStrategy->assert(checksumData)) {
        sendNack(header.seq_num);
        return {};
    }

    sendAck(header.seq_num);

    return data;
}

uint8_t StopAndWaitController::nextSeqNum(uint8_t current) {
    uint8_t mask = (1 << 5) - 1;                // 5 bits for sequence number
    uint8_t next = (current + 1) & mask;        // Increment and wrap around
    return next;
}

void StopAndWaitController::setPacketType(PacketType type) {
    uint8_t mask = (1 << 4) - 1;
    this->packet_type = static_cast<uint8_t>(type) & mask;        // Ensure type is within 4 bits
}

bool StopAndWaitController::waitForAck(uint8_t seq_num) {
    std::unique_lock lock(mtx);
    auto pckt = transmitter->receiveData();

    PacketHeader header{};
    memcpy(&header, pckt.data() + 1, sizeof(header));

    std::vector<uint8_t> checksumData;
    checksumData.push_back(header.size);
    checksumData.push_back(header.seq_num);
    checksumData.push_back(header.type);

    if ( errorControlStrategy->assert(checksumData) &&
        header.seq_num == seq_num &&
        header.type == static_cast<uint8_t>(PacketType::ACK)
    ) return true;

    return false; // Timeout - NACK
}

void StopAndWaitController::sendAck(uint8_t seq_num) const {
    PacketHeader ackHeader{};
    ackHeader.size = 0;
    ackHeader.seq_num = seq_num;
    ackHeader.type = static_cast<uint8_t>(PacketType::ACK);

    std::vector<uint8_t> ackPacket;
    memcpy(ackPacket.data(), &ackHeader, sizeof(ackHeader));
    ackHeader.checksum = errorControlStrategy->generate(ackPacket);
    ackPacket.clear();

    ackPacket.push_back(START_MARK);
    memcpy(ackPacket.data() + 1, &ackHeader, sizeof(ackHeader));

    transmitter->sendData(ackPacket);
}

void StopAndWaitController::sendNack(uint8_t seq_num) const {
    PacketHeader ackHeader{};
    ackHeader.size = 0;
    ackHeader.seq_num = seq_num;
    ackHeader.type = static_cast<uint8_t>(PacketType::NACK);

    std::vector<uint8_t> ackPacket;
    memcpy(ackPacket.data(), &ackHeader, sizeof(ackHeader));
    ackHeader.checksum = errorControlStrategy->generate(ackPacket);
    ackPacket.clear();

    ackPacket.push_back(START_MARK);
    memcpy(ackPacket.data() + 1, &ackHeader, sizeof(ackHeader));

    transmitter->sendData(ackPacket);
}
