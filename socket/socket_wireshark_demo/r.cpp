#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);    
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7348);
    inet_pton(AF_INET, "127.0.0.48", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    const char* msg = "Hello from client";
    send(sock, msg, strlen(msg), 0);
    std::cout << "Message sent\n";

    close(sock);
    return 0;
}
