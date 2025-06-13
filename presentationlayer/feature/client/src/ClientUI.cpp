//
// Created by julio-martins on 5/21/25.
//
/**
 * - When trying to receive file, it fails
 * - getch() is conflicting with listen and causing issues with file receive/send command
 */

#ifdef CLIENT
#include <atomic>
#include <string>
#include <thread>

#include "LogUtils.h"
#include "../include/ClientUiController.h"
#include "../../server/include/GridUtils.h"


std::string lastMove;
std::string statusMsg;

void drawCommandTable(int startY, int startX) {
    mvprintw(startY, startX, "Permitted Commands:");
    mvprintw(startY + 1, startX, "i / ^ : Move Up");
    mvprintw(startY + 2, startX, "k / v : Move Down");
    mvprintw(startY + 3, startX, "j / < : Move Left");
    mvprintw(startY + 4, startX, "l / > : Move Right");
    mvprintw(startY + 5, startX, "q : Quit");
}

void drawStatusLine(int screenY) {
    mvprintw(screenY + 3, 0, "Status: %s", statusMsg.c_str());
    clrtoeol(); // Clear the rest of the line
}

void drawLastMovement(int screenY) {
    mvprintw(screenY + 2, 0, "Last movement: %s", lastMove.c_str());
    clrtoeol();
}

void drawScreen() {
    clear();

    drawCommandTable(1, 0);
    drawLastMovement(8);
    drawStatusLine(8);

    refresh();
}

// Declare a flag to signal termination
std::atomic<bool> running(true);

void startListening(ClientUiController &controller) {
    while (running.load()) {
        controller.listen(); // Non-blocking check for raw socket data
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // prevent busy loop
    }
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <interface name>" << std::endl;
        return EXIT_FAILURE;
    }
    if (argc > 2 && argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <interface name> <output file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string interface = std::string(argv[1]);
    std::string default_log_file = (argc == 3 ? std::string(argv[2]) : "/dev/null");
    LogUtils logger = LogUtils(default_log_file);
    logger.start();

    initscr(); // Start ncurses
    cbreak(); // Disable line buffering
    noecho(); // Don't echo input
    keypad(stdscr, TRUE); // Enable special keys (arrows)
    nodelay(stdscr, TRUE); // Make getch() non-blocking
    curs_set(0); // Hide cursor

    int ch;
    ClientUiController clientUiController(interface);
    std::thread listenerThread(startListening, std::ref(clientUiController));

    clientUiController.fileObserver.observe([&clientUiController](std::vector<uint8_t> fileName) {
        clientUiController.setStatusMessage(
            "Saving incoming file"+ std::string(fileName.begin(), fileName.end())
        );
        if (!clientUiController.saveIncomingFile(fileName)) {
            clientUiController.setStatusMessage(
                "Failed to save incoming file" + std::string(fileName.begin(), fileName.end())
            );
        } else {
            clientUiController.setStatusMessage("File successfully saved!");
        }
    });

    clientUiController.statusObserver.observe([](std::string msg) {
        statusMsg = msg;
        drawScreen();
    });

    drawScreen();
    while (running.load()) {
        ch = getch();
        if (ch == ERR) continue;

        auto it = clientUiController.directionMap.find(ch);
        if (it != clientUiController.directionMap.end()) {
            lastMove = it->second;
            if (!clientUiController.sendMovement(GridUtils::toUint8(it->first))) {
                 clientUiController
                    .setStatusMessage("Failed to send move command. Try again!");
            }
        } else {
             clientUiController
                .setStatusMessage("Invalid input. Press q to quit.");
        }

        drawScreen();
        if (ch == 'q') running = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Clean up
    running = false;
    listenerThread.join();
    logger.stop();
    endwin(); // End ncurses mode
    return EXIT_SUCCESS;
}
#endif
