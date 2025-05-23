//
// Created by julio-martins on 5/20/25.
//

#ifndef SERVERUICONTROLLER_H
#define SERVERUICONTROLLER_H

#include "../../../../datalayer/feature/datatransfer/include/DataTransferRawSocket.h"
#include "../../../../flowcontrollayer/feature/stopandwait/include/StopAndWaitController.h"
#include "../../../../protocollayer/feature/kermit/include/KermitProtocol.h"
#include "../../observer/include/DataObserver.h"

class ServerUiController {
    DataTransferRawSocket *transmitter;
    StopAndWaitController *controller;
    KermitProtocol *protocol;

    static FileUtils::FileType getFileType(const std::string &filePath);
public:
    DataObserver<int> moveObserver;
    DataObserver<std::string> fileObserver;

    ServerUiController(const std::string &interface);
    ~ServerUiController();

    void listen();
    bool sendFile(const std::string &filePath) const;
};

#endif //SERVERUICONTROLLER_H
