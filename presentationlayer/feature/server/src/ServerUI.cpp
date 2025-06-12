//
// Created by julio-martins on 5/19/25.
//

/**
 * - When sending file, it doesn`t preserve the file content - malformed packets
 */
#ifdef SERVER
#include <vector>
#include <cstdlib>
#include <ctime>
#include <set>
#include <iostream>
#include <list>
#include <unordered_map>
#include <filesystem>

#include "LogUtils.h"
#include "../include/GridUtils.h"
#include "../include/ServerUiController.h"

Position player{0, 0};
std::vector moveLog = {player};
std::unordered_map<Position, std::string> treasures;
std::string lastMessage;

int wrap(int value, int max) {
    return (value % max + max) % max;
}

std::list<std::string> getFiles() {
    std::string path = "./objetos/";
    std::list<std::string> files = {};

    try {
        for (const auto &entry: std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename());
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        throw std::runtime_error("Filesystem error: " + std::string(e.what()));
    }
    return files;
}

// Generate random unique treasures with file paths
std::unordered_map<Position, std::string> generateTreasures(int count) {
    std::unordered_map<Position, std::string> treasures;
    auto files = getFiles();
    while (treasures.size() < count) {
        Position p{rand() % GRID_SIZE, rand() % GRID_SIZE};
        if (treasures.find(p) == treasures.end()) {
            treasures[p] = "./objetos/" + files.front();
            std::cout << treasures[p] << std::endl;
            files.pop_front();
        }
    }
    return treasures;
}
void drawMoveLog() {
    // Draw movement log
    mvprintw(GRID_SIZE + 3, 0, "Movement log (last %d steps):", LOG_SIZE);
    int logStart = std::max(0, static_cast<int>(moveLog.size()) - LOG_SIZE);
    for (int i = 0; i < LOG_SIZE; ++i) {
        int logIndex = logStart + i;
        if (logIndex < moveLog.size()) {
            mvprintw(GRID_SIZE + 4 + i, 0, "Step %2d: (%d, %d)   ", logIndex + 1, moveLog[logIndex].x, moveLog[logIndex].y);
        } else {
            mvprintw(GRID_SIZE + 4 + i, 0, "                         ");
        }
    }
}

void drawMessageStatus() {
    mvprintw(GRID_SIZE + LOG_SIZE + 5, 0, "Message: %-80s", lastMessage.c_str());
}

void drawGrid() {
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            Position p{x, y};
            bool isPlayer = (player.x == x && player.y == y);
            bool isTreasure = treasures.count(p);
            if (isPlayer) {
                attron(A_REVERSE);
                mvprintw(y + 2, x * 4, "[P]");
                attroff(A_REVERSE);
            } else if (isTreasure) {
                mvprintw(y + 2, x * 4, " T ");
            } else {
                mvprintw(y + 2, x * 4, " . ");
            }
        }
    }
}

void drawScreen() {
    clear();
    drawGrid();
    drawMoveLog();
    drawMessageStatus();
    refresh();
}

int main() {
    srand(time(nullptr));
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    treasures = generateTreasures(GRID_SIZE);
    std::unordered_map<Position, std::string>::iterator found;
    lastMessage = "";

    std::string interface = "veth0";
    // std::cout << "Enter the network interface (e.g., lo, eth0): ";
    // std::cin >> interface;

    int ch;
    ServerUiController serverUiController(interface);
    LogUtils logger = LogUtils("/dev/null");
    logger.start();

    serverUiController.moveObserver.observe([&ch](int move) {
        ch = move;
    });

    serverUiController.fileObserver.observe([&serverUiController, &found](std::string fileName) {
        if (serverUiController.sendFile(fileName)) {
            treasures.erase(found); // only trigger once
            serverUiController.setStatusMessage("File sent successfully!");
        } else {
            serverUiController.setStatusMessage("Failed to send file:" + std::string(fileName));
        }
    });

    serverUiController.statusObserver.observe( [](std::string msg) {
        lastMessage = msg;
        drawScreen();
    });

    drawScreen();
    while (ch != 'q') {
        serverUiController.listen();
        switch (ch) {
            case KEY_UP:    player.y = wrap(player.y - 1, GRID_SIZE); break;
            case KEY_DOWN:  player.y = wrap(player.y + 1, GRID_SIZE); break;
            case KEY_LEFT:  player.x = wrap(player.x - 1, GRID_SIZE); break;
            case KEY_RIGHT: player.x = wrap(player.x + 1, GRID_SIZE); break;
            default: continue;
        }
        ch = ERR;
        moveLog.push_back(player);
        if (moveLog.size() > LOG_SIZE) {
            moveLog.erase(moveLog.begin());
        }

        found = treasures.find(player);
        if (found != treasures.end()) {
            serverUiController.setStatusMessage(
                "Treasure found! Sending file " + found->second + " to client..."
            );
            serverUiController.fileObserver.post(found->second);
        }
        drawScreen();
    }
    logger.stop();
    endwin();
    return 0;
}
#endif
