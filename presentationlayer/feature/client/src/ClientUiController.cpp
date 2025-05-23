//
// Created by julio-martins on 5/21/25.
//

#include "../include/ClientUiController.h"

ClientUiController::ClientUiController(std::string interface) {
    this->transmitter = new DataTransferRawSocket(interface);
    if (!this->transmitter->openSocket()) {
        throw std::runtime_error("Socket creation failed");
    }
    this->controller = new StopAndWaitController(this->transmitter);
    this->protocol = new KermitProtocol(controller);
}

ClientUiController::~ClientUiController() {
    delete this->protocol;
    delete this->controller;
    delete this->transmitter;
}

bool ClientUiController::saveIncomingFile(std::vector<uint8_t> fileName) const {
    return this->protocol->receiveFile(fileName);
}

bool ClientUiController::sendMovement(uint8_t move) const {
    return this->protocol->sendMsg(PacketUtils::PacketType::MOVE_DATA, {move});
}

bool ClientUiController::listen() {
    auto packet = protocol->receiveMsg();

    if (packet.header.type == PacketUtils::toUint8(PacketUtils::PacketType::TEXT_ACK_NOME) ||
        packet.header.type == PacketUtils::toUint8(PacketUtils::PacketType::MEDIA_ACK_NOME) ||
        packet.header.type == PacketUtils::toUint8(PacketUtils::PacketType::IMAGE_ACK_NOME)) {
        fileObserver.post(packet.data);
        return true;
    }

    return false;
}
