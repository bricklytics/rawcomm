//
// Created by julio-martins on 3/29/25.
//

#ifndef IBASESOCKET_H
#define IBASESOCKET_H

#include <vector>
#include <cstdint>

class IBaseSocket {
public:
    bool hasTimeout = false;

    virtual ~IBaseSocket() = default;

    virtual bool openSocket() = 0;
    virtual void setTimeout(int seconds) = 0;
    virtual bool sendData(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> receiveData() = 0;
};

#endif //IBASESOCKET_H
