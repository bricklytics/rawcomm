//
// Created by julio-martins on 5/22/25.
//

#ifndef LOGUTILS_H
#define LOGUTILS_H

#include <fstream>

class LogUtils final {
    std::ofstream logFile;
    std::streambuf* coutBuf = nullptr;
    std::streambuf* cerrBuf  = nullptr;
    std::string fileName;
public:
    LogUtils(std::string fileName);
    ~LogUtils() = default;

    void start();
    void stop();
};

#endif //LOGUTILS_H
