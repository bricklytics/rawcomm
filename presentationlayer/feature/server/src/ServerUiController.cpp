//
// Created by julio-martins on 5/20/25.
//

#include <memory>
#include "../include/ServerUiController.h"
#include "../include/GridUtils.h"

ServerUiController::ServerUiController(const std::string &interface) {
    this->transmitter = new DataTransferRawSocket(interface);
    if (!this->transmitter->openSocket()) {
        throw std::runtime_error("Socket creation failed");
    }
    this->controller = new StopAndWaitController(transmitter);
    this->protocol = new KermitProtocol(controller);
}

ServerUiController::~ServerUiController() {
    delete this->protocol;
    delete this->controller;
    delete this->transmitter;
}

FileUtils::FileType ServerUiController::getFileType(const std::string &filePath) {
    std::string command = "file --mime-type -b \"" + filePath + "\"";
    std::array<char, 128> buffer;
    std::string fileType;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) return FileUtils::FileType::UNKNOWN;

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        fileType += buffer.data();
    }

    size_t pos = fileType.find('/');
    if (pos != std::string::npos) fileType = fileType.substr(0, pos);
    else return FileUtils::FileType::UNKNOWN;

    if (fileType == "text") {
        return FileUtils::FileType::TEXT;
    }
    if (fileType == "video") {
        return FileUtils::FileType::VIDEO;
    }
    if (fileType == "image") {
        return FileUtils::FileType::IMAGE;
    }
    return FileUtils::FileType::UNKNOWN;
}

bool ServerUiController::sendFile(const std::string &filePath) const {
    auto fileType = getFileType(filePath);
    std::string fullName = filePath;
    switch (fileType) {
        case FileUtils::FileType::TEXT: fullName.append(".txt");
            break;
        case FileUtils::FileType::IMAGE: fullName.append(".jpg");
            break;
        case FileUtils::FileType::VIDEO: fullName.append(".mp4");
            break;
        default: return false;
    }
    return this->protocol->sendFile(fileType, filePath);
}

void ServerUiController::listen() {
    auto msg = protocol->receiveMsg();
    if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::MOVE_DATA)) {
        moveObserver.post(GridUtils::toInt(msg[0])); // Convert to ncurses key from byte
    }
}
