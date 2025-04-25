

#include "DataTransferRawSocket.h"
#include <thread>

int main() {
    std::string interface;
    std::cout << "Enter the network interface (e.g., lo, eth0): ";
    std::cin >> interface;

    #ifdef CLIENT
    DataTransferRawSocket sender(interface);
    if (!sender.openSocket()) {
        return EXIT_FAILURE;
    }
    #elif SERVER
    DataTransferRawSocket receiver(interface);
    if (!receiver.openSocket()) {
        return EXIT_FAILURE;
    }
    #endif

    std::string upstream;
    std::vector<uint8_t> packetData;
    auto counter = 0;

#ifdef CLIENT
    while (true) {
        std::cout << "Enter data to send: " << std::endl;
        std::cin >> upstream;
        if (upstream.compare(":E") == 0) break;

        packetData.clear();
        packetData.insert(packetData.begin(), upstream.begin(), upstream.end());
        sender.sendData(packetData);
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Reduce CPU usage
    }
#endif
#ifdef  SERVER
    while (counter < RETRIES){
        auto downStream = receiver.receiveData();
        if (!downStream.empty()) {
            std::cout << "Received Data: " << downStream.data() << std::endl;
            counter = 0;
        }
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Reduce CPU usage
    }
#endif

    return EXIT_SUCCESS;
}
