//
// Created by julio-martins on 4/17/25.
//


#include "../../../feature/checksum/include/ChecksumStrategy.h"
#include "../../../../app//tests/base/include/IBaseTest.h"

class ChecksumStrategyTest: public IBaseTest {
    ChecksumStrategy strategy;

public:
    void checkSumStrategyGenerateTest() {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        uint8_t expectedSum = 0xF5;

        uint8_t sum = strategy.generate(data);

        assert<uint8_t, uint8_t>(sum,expectedSum);
    }

    void checkSumStrategyAssertTest() {
        std::vector<uint8_t> data_with_checksumm = {0x01, 0x02, 0xF5, 0x03, 0x04};
        uint8_t expectedSum = 0xFF;
        uint8_t sum = strategy.generate(data_with_checksumm);

        assert<uint8_t, uint8_t>(sum, expectedSum);
    }

    ChecksumStrategyTest() {
        execute([this] {
            checkSumStrategyAssertTest();
            checkSumStrategyGenerateTest();
        });
    }
};
