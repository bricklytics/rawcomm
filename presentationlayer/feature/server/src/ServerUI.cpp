//
// Created by julio-martins on 5/19/25.
//

#include <vector>
#include <cstdlib>
#include <ctime>
#include <set>
#include <iostream>
#include "../include/GridUtils.h"
#include "../include/ServerUiController.h"


int wrap(int value, int max) {
    return (value % max + max) % max;
}

void drawGrid(const Position& player, const std::set<Position>& treasures, const std::vector<Position>& log) {
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
            mvprintw(GRID_SIZE + 4 + i, 0, "                    "); // Clear leftover lines
        }
    }

    refresh();
}

std::set<Position> generateTreasures(int count) {
    std::set<Position> treasures;
    while (treasures.size() < count) {
        Position p{rand() % GRID_SIZE, rand() % GRID_SIZE};
        treasures.insert(p);
    }
    return treasures;
}

int main() {
    srand(time(nullptr));
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    Position player{0, 0};
    std::vector moveLog = {player};
    std::set<Position> treasures = generateTreasures(8);

    std::string interface;
    std::cout << "Enter the network interface (e.g., lo, eth0): ";
    std::cin >> interface;

    int ch;
    ServerUiController serverUiController(interface);

    serverUiController.moveObserver.observe([&ch] (int move){
        ch = move;
    });

    serverUiController.fileObserver.observe( [&serverUiController](std::vector<uint8_t> fileName){
        if (serverUiController.saveIncomingFile(fileName)) {
            std::cerr << "File received successfully." << std::endl;
        }
    });

    drawGrid(player, treasures, moveLog);
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
        drawGrid(player, treasures, moveLog);
        if (moveLog.size() > LOG_SIZE) {
            moveLog.erase(moveLog.begin());
        }
    }

    endwin();
    return 0;
}