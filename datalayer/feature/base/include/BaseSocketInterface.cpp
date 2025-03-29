//
// Created by julio-martins on 3/28/25.
//

#include <vector>
#include <cstdint>

class BaseSocketInterface {
  public:
    virtual ~BaseSocketInterface() = default;
    virtual bool sendData(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> receiveData() = 0;
};