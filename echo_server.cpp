#include <iostream>
#include <string> 
#include <unistd.h> // for close(), read(), write()
#include <arpa/inet.h>

constexpr int PORT     = 5001;
constexpr int BUF_SIZE = 1024;

int main() {
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0); // ::는 전역 네임스페이스의 함수를 사용함을 명시
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serv_addr{}; // {}로 초기화
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(PORT);

    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) { // reinterpret_cast: C++ 스타일(포인터 타입) 캐스팅(형변환)
        perror("bind");
        ::close(listen_fd);
        return 1;
    }

    if (::listen(listen_fd, 5) < 0) {
        perror("listen");
        ::close(listen_fd);
        return 1;
    }

    std::cout << "[C++] Listening on port " << PORT << "...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(listen_fd,
                                 reinterpret_cast<sockaddr*>(&client_addr), 
                                 // reinterpret_cast 쓰는 이유: sockaddr_in* -> sockaddr* 로 포인터 타입 변환
                                 &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        std::cout << "[C++] Client connected: "
                  << ::inet_ntoa(client_addr.sin_addr)
                  << ":" << ntohs(client_addr.sin_port) << "\n";

        char buf[BUF_SIZE];

        while (true) {
            ssize_t n = ::read(client_fd, buf, BUF_SIZE);
            if (n < 0) {
                perror("read");
                break;
            } else if (n == 0) {
                std::cout << "[C++] Client disconnected\n";
                break;
            }

            ::write(client_fd, buf, n);  // 에코
        }

        ::close(client_fd);
    }

    ::close(listen_fd);
    return 0;
}
