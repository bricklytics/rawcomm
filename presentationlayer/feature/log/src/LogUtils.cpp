//
// Created by julio-martins on 5/22/25.
//


#include <iostream>
#include "../include/LogUtils.h"

LogUtils::LogUtils(std::string fileName) {
    this->fileName = fileName;
}

void LogUtils::start() {
    logFile.open(fileName, std::ios::app | std::ios::out);
    if (!logFile) {
        throw std::runtime_error("Failed to open log file.");
    }
    // Backup original stream buffers
    coutBuf = std::cout.rdbuf();
    cerrBuf = std::cerr.rdbuf();

    // Redirect both to log file
    std::cout.rdbuf(logFile.rdbuf());
    std::cerr.rdbuf(logFile.rdbuf());
}

void LogUtils::stop() {
    std::cout.rdbuf(coutBuf);
    std::cerr.rdbuf(cerrBuf);
}
