//
// Created by julio-martins on 5/18/25.
//

#ifndef KERMITPROTOCOL_H
#define KERMITPROTOCOL_H

#include <vector>
#include <string>

#include "../../../../flowcontrollayer/feature/stopandwait/include/StopAndWaitController.h"
#include "../../../../flowcontrollayer/feature/base/include/PacketType.h"
#include "../../base/include/IBaseProtocol.h"

class KermitProtocol final : public IBaseProtocol {
    IFlowController *controller;

    long fileSize = 0L;
    long bytesDownloaded = 0L;

    bool sendFileInfo(FileUtils::FileType type, const std::string &filePath) const;

public:
    KermitProtocol(IFlowController *controller);
    ~KermitProtocol() override = default;

    /**
     * Send a array of bytes over Kermit protocol.
     * @param type The type of the packet.
     * @param data The data chunk to be sent.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool sendMsg(PacketUtils::PacketType type, const std::vector<uint8_t> &data) override;

    /**
     * Send a file over Kermit protocol.
     * @param filePath The path to the file to be sent.
     * @return  True if the file was sent successfully, false otherwise.
     */
    bool sendFile(FileUtils::FileType type, const std::string &filePath) override;

    /**
     * Receive a array of bytes over Kermit protocol.
     * @return The received data chunk.
     */
    std::vector<uint8_t> receiveMsg() override;

    /**
     * Receive a file over Kermit protocol.
     * If path not provided, the file will be saved in the current directory. Always override.
     * @return The received data chunk.
     */
    bool receiveFile(std::vector<uint8_t> fileName) override;
};
#endif //KERMITPROTOCOL_H
