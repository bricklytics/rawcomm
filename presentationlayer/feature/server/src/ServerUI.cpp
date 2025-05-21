//
// Created by julio-martins on 5/19/25.
//

#include <vector>
#include <cstdlib>
#include <ctime>
#include <set>
#include <iostream>
#include <unordered_map>

#include "../include/GridUtils.h"
#include "../include/ServerUiController.h"


int wrap(int value, int max) {
    return (value % max + max) % max;
}

// Generate random unique treasures with file paths
std::unordered_map<Position, std::string> generateTreasures(int count) {
    std::unordered_map<Position, std::string> treasures;
    while (treasures.size() < count) {
        Position p{rand() % GRID_SIZE, rand() % GRID_SIZE};
        if (treasures.find(p) == treasures.end()) {
            treasures[p] = "./objetos/" + std::to_string(treasures.size() + 1);
        }
    }
    return treasures;
}

void drawGrid(const Position &player,
              const std::unordered_map<Position, std::string> &treasures,
              const std::vector<Position> &log,
              const std::string &lastMessage) {
    clear();
    mvprintw(0, 0, "Use arrow keys to move. Press 'q' to quit.");

    // Draw grid
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

    // Draw movement log
    mvprintw(GRID_SIZE + 3, 0, "Movement log (last %d steps):", LOG_SIZE);
    int logStart = std::max(0, static_cast<int>(log.size()) - LOG_SIZE);
    for (int i = 0; i < LOG_SIZE; ++i) {
        int logIndex = logStart + i;
        if (logIndex < log.size()) {
            mvprintw(GRID_SIZE + 4 + i, 0, "Step %2d: (%d, %d)   ", logIndex + 1, log[logIndex].x, log[logIndex].y);
        } else {
            mvprintw(GRID_SIZE + 4 + i, 0, "                         ");
        }
    }

    // Display latest message
    mvprintw(GRID_SIZE + LOG_SIZE + 5, 0, "Message: %-80s", lastMessage.c_str());

    refresh();
}

int main() {
    srand(time(nullptr));
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    Position player{0, 0};
    std::vector moveLog = {player};
    std::unordered_map<Position, std::string> treasures = generateTreasures(8);
    std::string lastMessage = "";

    std::string interface;
    std::cout << "Enter the network interface (e.g., lo, eth0): ";
    std::cin >> interface;

    int ch;
    ServerUiController serverUiController(interface);

    serverUiController.moveObserver.observe([&ch](int move) {
        ch = move;
    });

    serverUiController.fileObserver.observe([&serverUiController](std::string fileName) {
        if (!serverUiController.sendFile(fileName)) {
            std::cerr << "Failed to receive file." << std::endl;
        }
    });

    drawGrid(player, treasures, moveLog, lastMessage);
    while (ch != 'q') {
        serverUiController.listen();
        switch (ch) {
            case KEY_UP:    player.y = wrap(player.y - 1, GRID_SIZE); break;
            case KEY_DOWN:  player.y = wrap(player.y + 1, GRID_SIZE); break;
            case KEY_LEFT:  player.x = wrap(player.x - 1, GRID_SIZE); break;
            case KEY_RIGHT: player.x = wrap(player.x + 1, GRID_SIZE); break;
            default: continue;
        }

        moveLog.push_back(player);
        if (moveLog.size() > LOG_SIZE) {
            moveLog.erase(moveLog.begin());
        }

        auto found = treasures.find(player);
        if (found != treasures.end()) {
            lastMessage = "Treasure found. Sending file " + found->second + " to client...";
            drawGrid(player, treasures, moveLog, lastMessage);

            serverUiController.fileObserver.post(found->second);
            treasures.erase(found); // only trigger once
        }
        drawGrid(player, treasures, moveLog, lastMessage);
    }

    endwin();
    return 0;
}
