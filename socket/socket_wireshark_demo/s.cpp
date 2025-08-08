#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
using namespace std;

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7348);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        return 1;
    }

    listen(server_fd, 1);
    cout << "waiting for connection..." << endl;

    int client_fd = accept(server_fd, nullptr, nullptr);
    if(client_fd < 0){
        perror("accept");
        return 1;
    }

    char buffer[1024] = {0};
    read(client_fd, buffer, sizeof(buffer));
    cout << "received: " << buffer << endl;

    close(client_fd);
    close(server_fd);
}