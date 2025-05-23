//
// Created by julio-martins on 5/21/25.
//

#ifdef CLIENT
#include <string>

#include "LogUtils.h"
#include "../include/ClientUiController.h"
#include "../../server/include/GridUtils.h"
#include "../include/ClientUiController.h"

void drawCommandTable(int startY, int startX) {
    mvprintw(startY, startX, "Permitted Commands:");
    mvprintw(startY + 1, startX, "i / ^ Move Up");
    mvprintw(startY + 2, startX, "k / v : Move Down");
    mvprintw(startY + 3, startX, "j / < : Move Left");
    mvprintw(startY + 4, startX, "l / > : Move Right");
    mvprintw(startY + 4, startX, "q : Quit");
}

void drawStatusLine(int screenY, const std::string &message) {
    mvprintw(screenY - 1, 0, "Status: %s", message.c_str());
    clrtoeol(); // Clear the rest of the line
}

void drawLastMovement(int screenY, const std::string &move) {
    mvprintw(screenY / 2, 0, "Last movement: %s", move.c_str());
    clrtoeol();
}

void drawScreen(
    int screenY,
    const std::string &lastMove,
    const std::string &statusMsg
) {
    clear();

    drawCommandTable(1, 2);
    drawLastMovement(screenY, lastMove);
    drawStatusLine(screenY, statusMsg);

    refresh();
}

int main() {
    initscr(); // Start ncurses
    cbreak(); // Disable line buffering
    noecho(); // Don't echo input
    keypad(stdscr, TRUE); // Enable special keys (arrows)
    curs_set(0); // Hide cursor

    int screenY, screenX;
    getmaxyx(stdscr, screenY, screenX);

    std::string lastMove = "None";
    std::string statusMsg = "";

    std::string interface = "veth1";
    // std::cout << "Enter the network interface (e.g., lo, eth0): ";
    // std::cin >> interface;

    LogUtils logger = LogUtils("client.log");
    logger.start();

    ClientUiController clientUiController(interface);

    clientUiController.fileObserver.observe([&clientUiController, &statusMsg](std::vector<uint8_t> fileName) {
        if (!clientUiController.saveIncomingFile(fileName)) {
            statusMsg = "Failed to save treausure";
        }
    });

    int ch = getch();
    drawScreen(screenY, lastMove, statusMsg);
    while (ch != 'q') {
        if (ch == ERR) clientUiController.listen();
        else {
            auto it = clientUiController.directionMap.find(ch);
            if (it != clientUiController.directionMap.end()) {
                lastMove = it->second;
                statusMsg = "Command sent to server.";
                if (!clientUiController.sendMovement(GridUtils::toUint8(it->first))) {
                    statusMsg = "Failed to send move command.";
                }
            } else {
                statusMsg = "Invalid input. Press q to quit.";
            }
            drawScreen(screenY, lastMove, statusMsg);
        }
        ch = getch();
    }

    logger.stop();
    endwin(); // End ncurses mode
    return 0;
}
#endif