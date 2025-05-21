//
// Created by julio-martins on 5/18/25.
//

#ifndef IBASEPROTOCOL_H
#define IBASEPROTOCOL_H

#include <cstdint>
#include <vector>
#include <string>
#include "../../../../flowcontrollayer/feature/base/include/PacketType.h"
#include "FileType.h"

class IBaseProtocol {
public:
    virtual ~IBaseProtocol() = default;
    virtual bool sendMsg(PacketUtils::PacketType type, const std::vector<uint8_t>& data) = 0;
    virtual bool sendFile(FileUtils::FileType type, const std::string& filePath) = 0;
    virtual std::vector<uint8_t>receiveMsg() = 0;
    virtual bool receiveFile(std::vector<uint8_t> fileName) = 0;
};

#endif //IBASEPROTOCOL_H
