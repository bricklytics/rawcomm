
#include "PacketSender.h"
#include "PacketReceiver.h"

#define PACKET_SIZE 1500

int main() {
    std::string interface = "eth0";  // Change to your network interface

    PacketSender sender(interface);
    PacketReceiver receiver(interface);

    if (!sender.bindSocket() || !receiver.bindSocket()) {
        std::cerr << "Failed to bind sockets!" << std::endl;
        return EXIT_FAILURE;
    }

    char packetData[PACKET_SIZE] = "Hello, Network!";
    sender.sendPacket(packetData, sizeof(packetData));

    char receivedData[PACKET_SIZE] = {};
    receiver.receivePacket(receivedData, sizeof(char)*PACKET_SIZE);

    std::cout << "Received Data: " << receivedData << std::endl;

    return EXIT_SUCCESS;
}
