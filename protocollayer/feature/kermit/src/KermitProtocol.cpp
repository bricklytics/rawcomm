//
// Created by julio-martins on 5/18/25.
//

#include  <fstream>
#include "../include/KermitProtocol.h"

#include <filesystem>

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
    std::filesystem::path inputFilePath{filePath};
    std::vector<uint8_t> packet(inputFilePath.filename().begin(), inputFilePath.filename().end());

    if (!controller->dispatch(packet)) return false;

    //send file size
    auto fileSize = std::filesystem::file_size(inputFilePath);
    this->controller->packet_type = PacketUtils::toUint8(PacketUtils::PacketType::SIZE);
    packet.clear();
    packet.push_back(fileSize > FILE_SIZE_MAX ? FILE_SIZE_MAX : fileSize);

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

std::string KermitProtocol::getFileName(uint8_t type) {
    switch (type) {
        case PacketUtils::PacketType::TEXT_ACK_NOME:
            return "text_file.txt";
        case PacketUtils::PacketType::MEDIA_ACK_NOME:
            return "media_file.mp4";
        case PacketUtils::PacketType::IMAGE_ACK_NOME:
            return "image_file.jpg";
        default:
            return "unknown_file";
    }
}

std::pair<std::string, uint8_t> KermitProtocol::getFileInfo() const {
    std::pair<std::string, std::uint8_t> fileInfo;

    auto packet_data = controller->receive();
    if (packet_data.empty()) {
        std::cerr << "Failed to receive file info packet" << std::endl;
        return {};
    }

    if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::TEXT_ACK_NOME) ||
        this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::MEDIA_ACK_NOME) ||
        this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::IMAGE_ACK_NOME)) {
        fileInfo.first = std::string(packet_data.begin(), packet_data.end());
    }

    packet_data = controller->receive();
    if (packet_data.empty()) {
        std::cerr << "Failed to receive file size packet" << std::endl;
        return {};
    }

    if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::SIZE)) {
        fileInfo.second = packet_data[0];
    }

    return fileInfo;
}

bool KermitProtocol::receiveFile(const std::string &filePath) {
    std::ofstream oFile;
    std::string defaultFilePath = !filePath.empty() ? filePath : "./objetos/";

    //Get the name and size of incoming file
    std::pair fileInfo = getFileInfo();

    if (filePath.empty()) defaultFilePath.append(getFileName(controller->packet_type));
    else defaultFilePath.append(fileInfo.first);

    oFile.open(defaultFilePath, std::ios_base::binary);
    if (!oFile.is_open()) {
        std::cerr << "Failed to open file " << filePath << std::endl;
        return {};
    }

    bool isEndOfData = false, writeFailed = false;
    while (!isEndOfData) {
        auto packet_data = controller->receive();
        isEndOfData = packet_data.empty();

        if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::DATA)){
            oFile.write(reinterpret_cast<const char *>(packet_data.data()), packet_data.size());
            writeFailed = oFile.fail();
            if (writeFailed) {
                std::cerr << "Failed to write to file " << filePath << std::endl;
                break;
            }
        }
    }

    oFile.close();
    if (writeFailed) return false;

    return true;
}
