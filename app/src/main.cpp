

#include <thread>

#include "DataTransferRawSocket.h"
#include "StopAndWaitController.h"
#include "KermitProtocol.h"
#include "PacketType.h"

int main() {
    #ifdef CLIENT
    std::string interface = "veth1";
    DataTransferRawSocket *sender = new DataTransferRawSocket(interface);
    if (!sender->openSocket()) {
        return EXIT_FAILURE;
    }
    #elif SERVER

    std::string interface = "veth0";
    DataTransferRawSocket *receiver = new DataTransferRawSocket(interface);
    if (!receiver->openSocket()) {
        return EXIT_FAILURE;
    }
    #endif

    std::string upstream;
    std::vector<uint8_t> packetData;
    auto counter = 0;

#ifdef CLIENT
    StopAndWaitController *controller = new StopAndWaitController(sender);
    KermitProtocol kermitProtocol(controller);
    while (true) {
        std::cout << "Enter data to send: " << std::endl;
        std::cin >> upstream;
        if (upstream.compare(":E") == 0) break;

        packetData.clear();
        packetData.insert(packetData.begin(), upstream.begin(), upstream.end());
        kermitProtocol.sendFile(FileUtils::FileType::TEXT, "tesouros/1.txt");
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Reduce CPU usage
    }
#endif
#ifdef  SERVER
    StopAndWaitController *controller = new StopAndWaitController(receiver);
    KermitProtocol kermitProtocol(controller);

    while (counter <= RETRIES){
        auto downStream = kermitProtocol.receiveFile(std::vector<uint8_t>(1,'1'));
        if (downStream) {
            std::cout << "Received Data: " << std::endl;
            counter = 0;
        }
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Reduce CPU usage
    }
#endif

    return EXIT_SUCCESS;
}
