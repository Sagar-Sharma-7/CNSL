#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>

using namespace std;

// Polynomial for CRC-4: x^3 + x^2 + 1 => binary 1101 (bits: 1 1 0 1)
const vector<int> POLY = {1,1,0,1}; 
const int POLY_SIZE = POLY.size();

vector<int> stringToBits(const string& msg) {
    vector<int> bits;
    for (char c : msg) {
        for (int i = 7; i >= 0; --i) {
            bits.push_back((c >> i) & 1);
        }
    }
    return bits;
}

vector<int> crcDivide(vector<int> data) {
    // Append 3 zeros for CRC division (degree 3)
    for (int i = 0; i < POLY_SIZE - 1; ++i) {
        data.push_back(0);
    }
    vector<int> remainder(data.begin(), data.begin() + POLY_SIZE);
    int total_bits = data.size();

    for (int i = POLY_SIZE; i <= total_bits; ++i) {
        if (remainder[0] == 1) {
            // XOR remainder with polynomial
            for (int j = 0; j < POLY_SIZE; ++j) {
                remainder[j] ^= POLY[j];
            }
        }
        if (i == total_bits) break;
        // Shift left and bring next bit in
        remainder.erase(remainder.begin());
        remainder.push_back(data[i]);
    }
    return remainder; // This is CRC remainder
}

vector<int> appendCRC(const vector<int>& data, const vector<int>& crc) {
    vector<int> combined = data;
    combined.insert(combined.end(), crc.begin(), crc.end());
    return combined;
}

// Convert bit vector to byte vector (pack 8 bits per byte)
vector<uint8_t> bitsToBytes(const vector<int>& bits) {
    vector<uint8_t> bytes;
    int size = bits.size();
    for (int i = 0; i < size; i += 8) {
        uint8_t b = 0;
        for (int j = 0; j < 8 && (i + j) < size; ++j) {
            b = (b << 1) | bits[i + j];
        }
        // If less than 8 bits, shift left remaining
        if ((size - i) < 8) {
            b <<= (8 - (size - i));
        }
        bytes.push_back(b);
    }
    return bytes;
}

// Convert bytes back to bits
vector<int> bytesToBits(const vector<uint8_t>& bytes, int bitsCount) {
    vector<int> bits;
    for (size_t i = 0; i < bytes.size(); ++i) {
        for (int j = 7; j >= 0; --j) {
            bits.push_back((bytes[i] >> j) & 1);
        }
    }
    if (bits.size() > bitsCount) {
        bits.resize(bitsCount);
    }
    return bits;
}

// Convert bits back to string (8 bits per char)
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
    // Setup socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(7348);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        return 1;
    }

    listen(server_fd, 1);
    cout << "Waiting for connection..." << endl;

    int client_fd = accept(server_fd, nullptr, nullptr);
    if(client_fd < 0){
        perror("accept");
        return 1;
    }

    // Original message to send
    string msg = "Hello CRC";
    cout << "Original message: " << msg << endl;

    // Convert message to bits
    vector<int> dataBits = stringToBits(msg);

    // Calculate CRC remainder
    vector<int> crc = crcDivide(dataBits);

    cout << "CRC bits: ";
    for (int b : crc) cout << b;
    cout << endl;

    // Append CRC to data bits
    vector<int> finalBits = appendCRC(dataBits, crc);

    finalBits[5] = finalBits[5] ^ 1;

    // Pack bits to bytes
    vector<uint8_t> sendBytes = bitsToBytes(finalBits);

    // Send bytes over TCP
    ssize_t sent = send(client_fd, sendBytes.data(), sendBytes.size(), 0);
    if (sent < 0) {
        perror("send");
        close(client_fd);
        close(server_fd);
        return 1;
    }
    cout << "Sent " << sent << " bytes with CRC appended.\n";

    // Wait for client response
    char buffer[1024] = {0};
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        cout << "Client says: " << buffer << endl;
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
