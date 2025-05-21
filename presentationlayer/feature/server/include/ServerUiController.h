//
// Created by julio-martins on 5/20/25.
//

#ifndef SERVERUICONTROLLER_H
#define SERVERUICONTROLLER_H

#include "GridUtils.h"
#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"
#include "../../../../flowcontrollayer/feature/stopandwait/include/StopAndWaitController.h"
#include "../../../../protocollayer/feature/kermit/include/KermitProtocol.h"
#include "../../base/observer/include/DataObserver.h"

class ServerUiController {
    DataTransferRawSocket *transmitter;
    StopAndWaitController *controller;
    KermitProtocol *protocol;

public:
    DataObserver<int> moveObserver;
    DataObserver<std::vector<uint8_t>> fileObserver;

    ServerUiController(const std::string &interface);
    ~ServerUiController();

    void listen();
    bool saveIncomingFile(std::vector<uint8_t>) const;
};

#endif //SERVERUICONTROLLER_H
