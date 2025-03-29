
#include <chrono>
#include <thread>

#include "PacketSender.h"
#include "PacketReceiver.h"

#define PACKET_SIZE 1500
#define TIMEOUT_SECONDS 5

int main() {
    std::string interface = "lo";

    std::cout << "Enter the network interface (e.g., lo, eth0): ";
    std::cin >> interface;  // Change to your network interface

    PacketSender sender(interface);
    PacketReceiver receiver(interface);

    if (!sender.bindSocket() || !receiver.bindSocket()) {
        std::cerr << "Failed to bind sockets!" << std::endl;
        return EXIT_FAILURE;
    }

    // Set socket timeout
    receiver.setTimeout(2, 0);  // 2 seconds timeout per recvfrom() call

    char packetData[PACKET_SIZE] = "Hello, Network!";
    sender.sendPacket(packetData, sizeof(packetData));

    char receivedData[PACKET_SIZE] = {};
    auto start_time = std::chrono::steady_clock::now();

    int counter = 0;

    while (counter < TIMEOUT_SECONDS) {
        auto isReceived = receiver.receivePacket(receivedData, sizeof(receivedData));
        if (isReceived) {
            std::cout << "Received Data: " << receivedData << std::endl;
            start_time = std::chrono::steady_clock::now();  // Reset inactivity timer
        }

        // Check elapsed time
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > TIMEOUT_SECONDS) {
            std::cout << "No activity detected for " << TIMEOUT_SECONDS << " seconds. Closing socket." << std::endl;
            break;
        }

        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Reduce CPU usage
    }

    return EXIT_SUCCESS;
}
