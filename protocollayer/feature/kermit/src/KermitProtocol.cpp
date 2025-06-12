//
// Created by julio-martins on 5/18/25.
//

#include <fstream>
#include <filesystem>
#include "../include/KermitProtocol.h"
#include "../../../../presentationlayer/feature/server/include/GridUtils.h"


KermitProtocol::KermitProtocol(IFlowController *controller) {
    this->controller = controller;
}

bool KermitProtocol::sendMsg(const PacketUtils::PacketType type, const std::vector<uint8_t> &data) {
    PacketUtils::Packet packet{};
    packet.header.size = data.size();
    packet.header.type = PacketUtils::toUint8(type);
    packet.data.insert(packet.data.end(),data.begin(), data.end());

    return controller->dispatch(packet);
}

PacketUtils::Packet KermitProtocol::receiveMsg() {
    return controller->receive();
}

bool KermitProtocol::sendFileInfo(FileUtils::FileType type, const std::string &filePath) const {
    //send packet with file name
    const std::filesystem::path inputFilePath{filePath};
    std::string filename = inputFilePath.filename().string();

    //send file name
    PacketUtils::Packet packetName{};
    packetName.header.type = FileUtils::toUint8(type);
    packetName.header.size = std::min(filename.size(), static_cast<size_t>(FILE_NAME_SIZE_MAX));
    packetName.data = std::vector<uint8_t>(filename.begin(),filename.end());

    if (!controller->dispatch(packetName)) return false;

    //send file size
    PacketUtils::Packet packetSize{};
    auto fileSize = std::filesystem::file_size(inputFilePath);
    packetSize.header.type = PacketUtils::toUint8(PacketUtils::PacketType::SIZE);

    auto value = std::min(fileSize, static_cast<uintmax_t>(FILE_SIZE_MAX));
    for (size_t i = 0; i < sizeof(size_t); ++i) {
        packetSize.data.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }
    packetSize.header.size = packetSize.data.size();
    if (!controller->dispatch(packetSize)) return false;

    return true;
}

bool KermitProtocol::sendFile(const FileUtils::FileType type, const std::string &filePath) {
    if (!sendFileInfo(type, filePath)) {
        std::cerr << "Failed to send file info packet" << std::endl;
        return false;
    }

    std::ifstream iFile(filePath, std::ios_base::binary);
    if (!iFile.is_open()) {
        std::cerr << "Failed to open file " << filePath << std::endl;
        return false;
    }

    std::streamsize bytesRead = 0;
    while (!iFile.eof()) {
        PacketUtils::Packet packet;
        packet.data = std::vector<uint8_t>(DATA_SIZE_MAX, 0);
        iFile.read(reinterpret_cast<char *>(packet.data.data()), DATA_SIZE_MAX);

        if (iFile.eof()) {
            packet.header.type = PacketUtils::toUint8(PacketUtils::PacketType::EOFP);
            controller->dispatch({});
            continue;
        }

        packet.header.size = iFile.gcount();
        if (packet.header.size < DATA_SIZE_MAX) {
            packet.data.resize(bytesRead);
        }

        packet.header.type = PacketUtils::toUint8(PacketUtils::PacketType::DATA);
        if (!controller->dispatch(packet)) {
            std::cerr << "Failed to send file " << filePath << std::endl;
            return false;
        }
    }
    iFile.close();
    return true;
}

bool KermitProtocol::receiveFile(std::vector<uint8_t> fileName) {
    std::string defaultFilePath = "./tesouros/";

    auto sizePacket = controller->receive();
    if (sizePacket.data.empty()) {
        std::cerr << "Failed to receive file size packet" << std::endl;
        return false;
    }

    if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::SIZE)) {
        long fileSize = 0L;
        size_t maxBytes = std::min(sizePacket.data.size(), sizeof(unsigned long));
        for (size_t i = 0; i < maxBytes; ++i) {
            fileSize |= static_cast<unsigned long>(sizePacket.data[i]) << (8 * i);
        }
        this->fileSize = fileSize;
    }

    defaultFilePath.append(std::string(fileName.begin(), fileName.end()));

    std::ofstream oFile(defaultFilePath, std::ios_base::binary);
    if (!oFile.is_open()) {
        std::cerr << "Failed to open file " << defaultFilePath << std::endl;
        return false;
    }

    bool isEndOfData = false, writeFailed = false;
    while (!isEndOfData) {
        auto packet = controller->receive();
        isEndOfData = packet.data.empty(); //when file ending packet is received or timeout

        if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::DATA) && !isEndOfData) {
            oFile.write(reinterpret_cast<const char *>(packet.data.data()), packet.header.size);
            writeFailed = oFile.fail();
            if (writeFailed) {
                std::cerr << "Failed to write to file " << defaultFilePath << std::endl;
                break;
            }
        }
    }

    oFile.close();
    if (writeFailed) return false;

    return true;
}
