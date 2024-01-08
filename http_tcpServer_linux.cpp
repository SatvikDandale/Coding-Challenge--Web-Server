#include "http_tcpServer_linux.h"

#include <bits/stdc++.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>

namespace {
const int BUFFER_SIZE = 30720;
void log(const std::string &message) { std::cout << message << "\n"; }

void exitWithError(const std::string &errorMessage) {
    log("Error: " + errorMessage);
    exit(1);
}
}  // namespace

namespace http {
TcpServer::TcpServer(std::string ip_address, int port)
    : m_ip_address(ip_address),
      m_port(port),
      m_socket(),
      m_new_socket(),
      m_incomingMessage(),
      m_socketAddress(),
      m_socketAddress_len(sizeof(m_socketAddress)),
      m_serverMessage(buildResponse()) {
    m_socketAddress.sin_family = AF_INET;
    m_socketAddress.sin_port = htons(m_port);
    m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

    if (startServer() != 0) {
        std::ostringstream ss;
        ss << "Failed to start server with PORT: "
           << ntohs(m_socketAddress.sin_port);
        log(ss.str());
    }
}
TcpServer::~TcpServer() { closeServer(); }

int TcpServer::startServer() {
    m_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (m_socket < 0) {
        exitWithError("Cannot create socket");
        return 1;
    }

    if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0) {
        exitWithError("Cannot connect socket to the address.");
        return 1;
    }

    return 0;
}

void TcpServer::closeServer() {
    close(m_socket);
    close(m_new_socket);
    exit(0);
}

std::string TcpServer::buildResponse() {
    std::string htmlFile =
        "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from "
        "your Server :) </p></body></html>";
    std::ostringstream ss;
    ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: "
       << htmlFile.size() << "\n\n"
       << htmlFile;

    return ss.str();
}

void TcpServer::startListen() {
    if (listen(m_socket, 20) < 0) {
        exitWithError("Socket listen failed");
    }
    std::ostringstream ss;
    ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr)
       << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
    log(ss.str());

    int bytesReceived;

    while (true) {
        log("====== Waiting for a new connection ======\n\n\n");
        acceptConnection(m_new_socket);

        char buffer[BUFFER_SIZE] = {0};
        bytesReceived = read(m_new_socket, buffer, BUFFER_SIZE);
        if (bytesReceived < 0) {
            exitWithError(
                "Failed to read bytes from client socket connection.");
        }

        std::ostringstream ss;
        ss << "------ Received Request from client ------\n\n";
        log(ss.str());
        log(buffer);
        getRequestedPath(buffer);

        sendResponse();

        close(m_new_socket);
    }
}

void TcpServer::acceptConnection(int &new_socket) {
    new_socket =
        accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
    if (new_socket < 0) {
        std::ostringstream ss;
        ss << "Server failed to accept incoming connection from ADDRESS: "
           << inet_ntoa(m_socketAddress.sin_addr)
           << "; PORT: " << ntohs(m_socketAddress.sin_port);
        exitWithError(ss.str());
    }
}

void TcpServer::sendResponse() {
    long bytesSent;

    bytesSent =
        write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());

    if (bytesSent == m_serverMessage.size()) {
        log("------ Server Response sent to client ------\n\n");
    } else {
        log("Error sending response to client");
    }
}

void TcpServer::getRequestedPath(std::string requestMessage) {
    std::vector<std::string> tokens;
    std::stringstream ss(requestMessage);
    std::string s;

    // Get first line from the request to get the requested path.
    getline(ss, s, '\n');

    ss.clear();
    ss.str(s);

    while (getline(ss, s, ' ')) {
        tokens.push_back(s);
    }

    // Expected HTTP format:
    // GET /path HTTP/1.1 -> HTTP method, path and http protocol
    if (tokens[0] == "GET") {
        std::string path = tokens[1];
        log("Requested path: " + path);
        if (path == "/") path = "/index.html";
        std::ifstream file("www" + path);
        if (!file.is_open()) {
            log("404 ERROR: Resource with path " + path + "not found.");
            m_serverMessage = "HTTP/1.1 404 Not Found\r\n\r\n";
            return;
        }
        std::string file_contents;
        s.clear();
        while(!file.eof()) {
            getline(file, s);
            file_contents += s;
        }
        file.close();

        // Response headers
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n\r\n";
        response += file_contents;
        m_serverMessage = response;

    } else {
        std::string response = "------ METHOD NOT SUPPORTED ------";
        m_serverMessage = response;
    }

}

}  // namespace http