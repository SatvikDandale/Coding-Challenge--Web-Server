#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

void printError(const char* error) { std::cerr << error << "\n"; }

int main() {
    // Creating a socket.
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        printError("Error creating socket.");
        return -1;
    }

    // Set up the server address structure
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address
    if (bind(serverSocket, reinterpret_cast<const sockaddr*>(&serverAddr),
             sizeof(serverAddr)) == -1) {
        printError("Error binding socket.");
        close(serverSocket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) == -1) {
        printError("Error listening on socket.");
        close(serverSocket);
        return -1;
    }

    // Start accepting connections
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientSocket;
    while (true) {
        clientSocket =
            accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr),
                   &clientAddrLen);
        if (clientSocket == -1) {
            printError("Error accepting connections.");
            continue;
        }
        std::cout << "Accepted connection from "
                  << inet_ntoa(clientAddr.sin_addr) << "\n";

        char buffer[1024];
        ssize_t bytesRead;
        memset(buffer, 0, sizeof(buffer));

        bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead < 0) {
            printError("Error receiving data.");
            close(clientSocket);
        }
        if (bytesRead == 0) {
            std::cout << "Client has disconnected.\n";
            close(clientSocket);
        }

        if (buffer[bytesRead - 1] == '\n') buffer[bytesRead - 1] = 0;

        std::cout << "Message from client: " << buffer << "\n";

        send(clientSocket, buffer, bytesRead, 0);

        close(clientSocket);
    }

    // Close socket
    close(serverSocket);

    return 0;
}