#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>

constexpr int PORT     = 5001;
constexpr int BUF_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>\n";
        return 1;
    }

    const char* server_ip = argv[1];

    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(PORT);
    if (::inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        ::close(sock);
        return 1;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
        perror("connect");
        ::close(sock);
        return 1;
    }

    std::cout << "[C++] Connected to " << server_ip << ":" << PORT << "\n";

    std::string line;
    char recv_buf[BUF_SIZE];

    while (true) {
        std::cout << "message> ";
        if (!std::getline(std::cin, line))
            break;

        if (line.empty())
            continue;

        ssize_t sent = ::write(sock, line.data(), line.size());
        if (sent < 0) {
            perror("write");
            break;
        }

        ssize_t n = ::read(sock, recv_buf, sizeof(recv_buf) - 1);
        if (n <= 0) {
            if (n < 0) perror("read");
            break;
        }
        recv_buf[n] = '\0';
        std::cout << "[echo] " << recv_buf << "\n";
    }

    ::close(sock);
    return 0;
}
