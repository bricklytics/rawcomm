//
// Created by julio-martins on 4/17/25.
//

#ifndef IBASETEST_H
#define IBASETEST_H

#include <iostream>
#include <functional>

namespace Color {
    const std::string RED = "\033[1;31m";
    const std::string GREEN = "\033[1;32m";
    const std::string RESET = "\033[0m";
};

class IBaseTest {

    public:
        ~IBaseTest() = default;

        // Test the socket creation
        template <typename T, typename R>
        static bool assert(T expected, R current) {
            if (expected == current) {
                std::cout << Color::GREEN << "Passed!" << Color::RESET << std::endl;
                return true;
            }
            std::cerr << Color::RED << "Failed!" << Color::RESET
                      << " Expected: " << expected << ", Current: " << current << std::endl;
            return false;
        }

        static void execute(const std::function<void()>& test_cases) {
            test_cases();
        }
};
#endif //IBASETEST_H
