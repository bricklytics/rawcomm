//
// Created by julio-martins on 5/20/25.
//

#include "../include/ServerUiController.h"

ServerUiController::ServerUiController(const std::string &interface) {
    this->transmitter = new DataTransferRawSocket(interface);
    this->controller = new StopAndWaitController(transmitter);
    this->protocol = new KermitProtocol(controller);
}

ServerUiController::~ServerUiController() {
    delete this->protocol;
    delete this->controller;
    delete this->transmitter;
}
bool ServerUiController::saveIncomingFile(std::vector<uint8_t> fileName) const {
    return this->protocol->receiveFile(fileName);
}

void ServerUiController::listen() {
    auto msg = protocol->receiveMsg();
    if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::MOVE_DATA)) {
        auto move = GridUtils::toInt(msg[0]); // Convert to ncurses key from byte
        moveObserver.post(move);
    } else if (this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::TEXT_ACK_NOME) ||
               this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::MEDIA_ACK_NOME) ||
               this->controller->packet_type == PacketUtils::toUint8(PacketUtils::PacketType::IMAGE_ACK_NOME)) {
        fileObserver.post(msg);
    }
}
