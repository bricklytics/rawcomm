//
// Created by julio-martins on 5/18/25.
//

#ifndef FILETYPE_H
#define FILETYPE_H

#include <cstdint>

#define FILE_SIZE_MAX 1024 * 1024 * 1024 // 1GB
#define FILE_NAME_SIZE_MAX 63 // 63 bytes

class FileUtils {
public:
    enum class FileType: uint8_t {
        TEXT = 0x01,
        IMAGE = 0x02,
        VIDEO = 0x03,
        UNKNOWN = 0x00
    };

    /**
     * Convert to byte from file type
     * @param type - the file type
     * @return the byte command
     */
    static uint8_t toUint8(FileType type) {
        switch (type) {
            case FileType::TEXT: return 0x06;
            case FileType::VIDEO: return 0x07;
            case FileType::IMAGE: return 0x08;
            default: return 0x0F; //Error type
        }
    }

    /**
     * Convert to file type from byte
     * @param type - the byte command
     * @return the file type
     */
    static FileType toFileType(uint8_t type) {
        switch (PacketUtils::toPacketType(type)) {
            case PacketUtils::PacketType::TEXT_ACK_NOME: return FileType::TEXT;
            case PacketUtils::PacketType::IMAGE_ACK_NOME: return FileType::IMAGE;
            case PacketUtils::PacketType::MEDIA_ACK_NOME: return FileType::VIDEO;
            default: return FileType::UNKNOWN;
        }
    }
};

#endif //FILETYPE_H
