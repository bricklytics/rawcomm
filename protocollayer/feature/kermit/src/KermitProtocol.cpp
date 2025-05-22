//
// Created by julio-martins on 5/18/25.
//

#include <fstream>
#include <filesystem>
#include "../include/KermitProtocol.h"
#include "../../../../presentationlayer/feature/server/include/GridUtils.h"

KermitProtocol::KermitProtocol() {
    this->controller = new StopAndWaitController();
}

KermitProtocol::KermitProtocol(IFlowController *controller) {
    this->controller = controller;
}

KermitProtocol::~KermitProtocol() {
    delete controller;
}

bool KermitProtocol::sendMsg(PacketUtils::PacketType type, const std::vector<uint8_t> &data) {
    this->controller->packet_type = static_cast<uint8_t>(type);
    return controller->dispatch(data);
}

std::vector<uint8_t> KermitProtocol::receiveMsg() {
    return controller->receive();
}

bool KermitProtocol::sendFileInfo(FileUtils::FileType type, const std::string &filePath) const {
    //send packet with file name
    this->controller->packet_type = FileUtils::toUint8(type);
    const std::filesystem::path inputFilePath{filePath};
    std::string filename = inputFilePath.filename().string();
    if (filename.find('.') != std::string::npos) {
        filename = filename.substr(0, filename.find('.'));
    }

    std::vector<uint8_t> packet(filename.begin(),filename.end());

    if (!controller->dispatch(packet)) return false;

    //send file size
    auto fileSize = std::filesystem::file_size(inputFilePath);
    this->controller->packet_type = PacketUtils::toUint8(PacketUtils::PacketType::SIZE);

    packet.clear();
    auto value = std::min(fileSize, static_cast<uintmax_t>(FILE_SIZE_MAX));
    for (size_t i = 0; i < sizeof(size_t); ++i) {
        packet.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }

    std::cerr << "Dispatching file size packet: " << fileSize << std::endl;
    if (!controller->dispatch(packet)) return false;

    return true;
}

bool KermitProtocol::sendFile(FileUtils::FileType type, const std::string &filePath) {
    if (!sendFileInfo(type, filePath)) {
        std::cerr << "Fail to send file file info packet" << filePath << std::endl;
        return false;
    }

    std::ifstream iFile(filePath, std::ios_base::binary);
    if (!iFile.is_open()) {
        std::cerr << "Failed to open file " << filePath << std::endl;
        return false;
    }

    std::streamsize bytesRead = 0;
    while (!iFile.eof()) {
        std::vector<uint8_t> buffer(DATA_SIZE_MAX, 0);
        iFile.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

        bytesRead = iFile.gcount();
        if (bytesRead < DATA_SIZE_MAX) {
            buffer.resize(bytesRead);
            buffer.shrink_to_fit();
        }

        controller->packet_type = PacketUtils::toUint8(PacketUtils::PacketType::DATA);
        if (!controller->dispatch(buffer)) {
            std::cerr << "Failed to send file " << filePath << std::endl;
            return false;
        }
        if (iFile.eof()) {
            controller->packet_type = PacketUtils::toUint8(PacketUtils::PacketType::EOFP);
            controller->dispatch({});
        }
    }
    iFile.close();
    return true;
}

bool KermitProtocol::receiveFile(std::vector<uint8_t> fileName) {
    std::string defaultFilePath = "objetos/";

    auto sizePacket = controller->receive();
    if (sizePacket.empty()) {
        std::cerr << "Failed to receive file size packet" << std::endl;
        return false;
    }

    if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::SIZE)) {
        long fileSize = 0L;
        size_t maxBytes = std::min(sizePacket.size(), sizeof(unsigned long));
        for (size_t i = 0; i < maxBytes; ++i) {
            fileSize |= static_cast<unsigned long>(sizePacket[i]) << (8 * i);
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
        auto packet_data = controller->receive();
        isEndOfData = packet_data.empty(); //when file ending packet is received or timeout

        if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::DATA) && !isEndOfData) {
            oFile.write(reinterpret_cast<const char *>(packet_data.data()), packet_data.size());
            writeFailed = oFile.fail();
            if (writeFailed) {
                std::cerr << "Failed to write to file " << defaultFilePath << std::endl;
                break;
            }
        }
        this->bytesDownloaded += packet_data.size();
    }

    oFile.close();
    if (writeFailed) return false;

    return true;
}
