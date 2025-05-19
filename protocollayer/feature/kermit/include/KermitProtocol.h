//
// Created by julio-martins on 5/18/25.
//

#ifndef KERMITPROTOCOL_H
#define KERMITPROTOCOL_H

#include <cstdint>
#include <vector>
#include <string>
#include "../../../../flowcontrollayer/feature/stopandwait/include/StopAndWaitController.h"
#include "../../../../flowcontrollayer/feature/base/include/PacketType.h"
#include "../../base/include/IBaseProtocol.h"

class KermitProtocol : public IBaseProtocol {
    IFlowController *controller;

    static std::string getFileName(uint8_t type);
    bool sendFileInfo(FileUtils::FileType type, const std::string& filePath) const;
    std::pair<std::string, uint8_t> getFileInfo() const;

public:
    KermitProtocol();
    KermitProtocol(IFlowController *controller);
    ~KermitProtocol() override;

    /**
     * Send a array of bytes over Kermit protocol.
     * @param type The type of the packet.
     * @param data The data chunk to be sent.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool sendMsg(PacketUtils::PacketType type, const std::vector<uint8_t>& data) override;

    /**
     * Send a file over Kermit protocol.
     * @param filePath The path to the file to be sent.
     * @return  True if the file was sent successfully, false otherwise.
     */
    bool sendFile(FileUtils::FileType type, const std::string& filePath) override;

    /**
     * Receive a array of bytes over Kermit protocol.
     * @return The received data chunk.
     */
    std::vector<uint8_t> receiveMsg() override;

    /**
     * Receive a file over Kermit protocol.
     * If path not provided, the file will be saved in the current directory. Always override.
     * @param filePath The path to save the received file.
     * @return The received data chunk.
     */
    bool receiveFile(const std::string& filePath) override;
};
#endif //KERMITPROTOCOL_H
