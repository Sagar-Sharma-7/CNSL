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

void printBits(const vector<int>& bits) {
    for (int b : bits) cout << b;
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

    while(true) {
        cout << "\nChoose option:\n1. Send without error\n2. Send with error\n3. Exit\nEnter choice: ";
        int choice; 
        cin >> choice;
        cin.ignore(); // flush newline

        if (choice == 3) {
            cout << "Exiting...\n";
            break;
        }

        cout << "Enter message to send: ";
        string msg;
        getline(cin, msg);

        vector<int> dataBits = stringToBits(msg);
        vector<int> crc = crcDivide(dataBits);
        vector<int> finalBits = appendCRC(dataBits, crc);

        // Flip bit if user wants error
        switch(choice) {
            case 1:
                // no error, do nothing
                break;
            case 2:
                // Introduce error: flip bit at position 5 if available
                if(finalBits.size() > 5) {
                    finalBits[5] = finalBits[5] ^ 1;
                }
                break;
            default:
                cout << "Invalid choice, try again.\n";
                continue;
        }

        // Print outputs
        cout << "\n--- Output ---\n";
        cout << "Message: " << msg << "\n";

        cout << "Message in binary: ";
        printBits(dataBits);
        cout << "\n";

        cout << "Polynomial (divisor): ";
        printBits(POLY);
        cout << "\n";

        cout << "CRC bits: ";
        printBits(crc);
        cout << "\n";

        cout << "Codeword (data + CRC): ";
        printBits(finalBits);
        cout << "\n";

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
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
