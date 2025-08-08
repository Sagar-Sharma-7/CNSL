#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>

using namespace std;

const vector<int> POLY = {1,1,0,1};
const int POLY_SIZE = POLY.size();

vector<int> bytesToBits(const vector<uint8_t>& bytes, int bitsCount) {
    vector<int> bits;
    for (size_t i = 0; i < bytes.size(); ++i) {
        for (int j = 7; j >= 0; --j) {
            bits.push_back((bytes[i] >> j) & 1);
        }
    }
    if (bits.size() > bitsCount) bits.resize(bitsCount);
    return bits;
}

vector<int> crcDivide(vector<int> data) {
    for (int i = 0; i < POLY_SIZE - 1; ++i) {
        data.push_back(0);
    }
    vector<int> remainder(data.begin(), data.begin() + POLY_SIZE);
    int total_bits = data.size();

    for (int i = POLY_SIZE; i <= total_bits; ++i) {
        if (remainder[0] == 1) {
            for (int j = 0; j < POLY_SIZE; ++j) {
                remainder[j] ^= POLY[j];
            }
        }
        if (i == total_bits) break;
        remainder.erase(remainder.begin());
        remainder.push_back(data[i]);
    }
    return remainder;
}

string bitsToString(const vector<int>& bits) {
    string s;
    for (size_t i = 0; i + 7 < bits.size(); i += 8) {
        char c = 0;
        for (int j = 0; j < 8; ++j) {
            c = (c << 1) | bits[i + j];
        }
        s.push_back(c);
    }
    return s;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7348);

    // Using loopback address
    inet_pton(AF_INET, "127.0.0.48", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    // Receive data from server
    uint8_t buffer[1024];
    ssize_t n = read(sock, buffer, sizeof(buffer));
    if (n <= 0) {
        perror("read");
        close(sock);
        return 1;
    }

    // We know message length (for demo, "Hello CRC" is 9 chars)
    // bits count = 9 chars * 8 bits + 3 CRC bits = 72 + 3 = 75 bits
    int bitsCount = 75;

    vector<uint8_t> recvBytes(buffer, buffer + n);
    vector<int> recvBits = bytesToBits(recvBytes, bitsCount);

    // Separate message bits and crc bits
    vector<int> messageBits(recvBits.begin(), recvBits.begin() + bitsCount - (POLY_SIZE - 1));
    vector<int> crcBits(recvBits.end() - (POLY_SIZE - 1), recvBits.end());

    // Recalculate CRC on message bits
    vector<int> calculatedCRC = crcDivide(messageBits);

    bool error = false;
    for (int i = 0; i < (POLY_SIZE -1); ++i) {
        if (crcBits[i] != calculatedCRC[i]) {
            error = true;
            break;
        }
    }

    if (!error) {
        cout << "No error detected.\n";
        string originalMessage = bitsToString(messageBits);
        cout << "Original message: " << originalMessage << endl;
    } else {
        cout << "Error detected in received message.\n";
        const char* errMsg = "error detected, send data again";
        send(sock, errMsg, strlen(errMsg), 0);
    }

    close(sock);
    return 0;
}
