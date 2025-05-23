//
// Created by julio-martins on 5/21/25.
//

#ifndef CLIENTCONTROLLER_H
#define CLIENTCONTROLLER_H
#include <ncurses.h>

#include "../../../../flowcontrollayer/feature/stopandwait/include/StopAndWaitController.h"
#include "../../../../protocollayer/feature/kermit/include/KermitProtocol.h"
#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"
#include "../../observer/include/DataObserver.h"

class ClientUiController {
    StopAndWaitController *controller;
    DataTransferRawSocket *transmitter;
    KermitProtocol *protocol;

public:
    DataObserver<std::vector<uint8_t>> fileObserver;
    DataObserver<uint8_t> movementObserver;

    // Movement mappings
    const std::unordered_map<int, std::string> directionMap = {
        {'i', "^ Up"},
        {KEY_UP, "^ Up"},
        {'k', "v Down"},
        {KEY_DOWN, "v Down"},
        {'j', "< Left"},
        {KEY_LEFT, "< Left"},
        {'l', "> Right"},
        {KEY_RIGHT, "> Right"},
        {'q', "Quit"}
    };

    ClientUiController(std::string interface);
    ~ClientUiController();

    bool listen();
    bool saveIncomingFile(std::vector<uint8_t> fileName) const;
    bool sendMovement(uint8_t move) const;
};
#endif //CLIENTCONTROLLER_H
