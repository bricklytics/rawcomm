//
// Created by julio-martins on 5/18/25.
//

#ifndef PACKETTYPE_H
#define PACKETTYPE_H

#include <cstdint>
#include <vector>
#include <cerrno>

#define DATA_SIZE_MAX 127               // Maximum data size in a packet in bytes
#define PAKET_SIZE_MAX 127              // Maximum packet size in bytes - including header

#define START_MARK 0x7E                 // 01111110

class PacketUtils {
public:
    enum class PacketType : uint8_t {
        ACK = 0x00, // Acknowledgment
        NACK = 0x01, // Negative acknowledgment
        MOVE_DATA = 0x03, // Contain data with directional movement
        DATA = 0x02, // Data packet
        SIZE = 0x04, // Size packet
        TEXT_ACK_NOME = 0x06, // Text file packet
        MEDIA_ACK_NOME = 0x07, // Media file packet
        IMAGE_ACK_NOME = 0x08, // Image file packet
        EOFP = 0x09, // End of file packet
        MOVE_RIGHT = 0x0A, // Move right packet
        MOVE_UP = 0x0B, // Move up packet
        MOVE_DOWN = 0x0C, // Move down packet
        MOVE_LEFT = 0x0D, // Move left packet
        ERROR = 0x0F // Error packet
        //Types 0x05 and 0x0E are reserved for future use
    };

    enum class ErrorType : uint8_t {
        EACCESS = 1,
        ESTORAGE = 2,
        EGENERIC = 3
    };

    typedef struct kermit_header {
        uint8_t size: 7;
        uint8_t seq_num: 5;
        uint8_t type: 4;
        uint8_t checksum: 8;
    } PacketHeader;

    typedef struct kermit_packet {
        uint8_t start_mark;
        PacketHeader header{};
        std::vector<uint8_t> data{};
    } Packet;

    /**
     * Convert to byte from packet type
     * @param type - the packet type
     * @return the byte command
     */
    static uint8_t toUint8(PacketType type) {
        switch (type) {
            case PacketType::ACK: return 0x00;
            case PacketType::NACK: return 0x01;
            case PacketType::DATA: return 0x02;
            case PacketType::MOVE_DATA: return 0x03;
            case PacketType::SIZE: return 0x04;
            case PacketType::TEXT_ACK_NOME: return 0x06;
            case PacketType::MEDIA_ACK_NOME: return 0x07;
            case PacketType::IMAGE_ACK_NOME: return 0x08;
            case PacketType::EOFP: return 0x09;
            case PacketType::MOVE_RIGHT: return 0x0A;
            case PacketType::MOVE_UP: return 0x0B;
            case PacketType::MOVE_DOWN: return 0x0C;
            case PacketType::MOVE_LEFT: return 0x0D;
            default: return 0x0F; // Error packet
        }
    }

    /**
     * Convert to packet type from byte
     * @param type - the byte command
     * @return the packet type
     */
    static PacketType toPacketType(uint8_t type) {
        switch (type) {
            case 0x00: return PacketType::ACK;
            case 0x01: return PacketType::NACK;
            case 0x02: return PacketType::DATA;
            case 0x03: return PacketType::MOVE_DATA;
            case 0x04: return PacketType::SIZE;
            case 0x06: return PacketType::TEXT_ACK_NOME;
            case 0x07: return PacketType::MEDIA_ACK_NOME;
            case 0x08: return PacketType::IMAGE_ACK_NOME;
            case 0x09: return PacketType::EOFP;
            case 0x0A: return PacketType::MOVE_RIGHT;
            case 0x0B: return PacketType::MOVE_UP;
            case 0x0C: return PacketType::MOVE_DOWN;
            case 0x0D: return PacketType::MOVE_LEFT;
            default: return PacketType::ERROR; // Error packet
        }
    }

    static uint8_t toErrorType(int code) {
        uint8_t errCode;
        switch (code) {
            case EACCES: errCode = 1;
                break;
            case EDQUOT:
            case ENOSPC: errCode = 2;
                break;
            default: errCode = 3;
                break;
        }

        return errCode;
    }
};

#endif //PACKETTYPE_H
