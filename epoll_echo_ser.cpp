#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

constexpr int PORT       = 5001;
constexpr int MAX_EVENTS = 128;
constexpr int BUF_SIZE   = 1024;

int make_socket_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}

int main() {
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serv_addr{};
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(PORT);

    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&serv_addr),
               sizeof(serv_addr)) == -1) {
        perror("bind");
        ::close(listen_fd);
        return 1;
    }

    if (::listen(listen_fd, SOMAXCONN) == -1) {
        perror("listen");
        ::close(listen_fd);
        return 1;
    }

    if (make_socket_nonblocking(listen_fd) == -1) {
        perror("fcntl");
        ::close(listen_fd);
        return 1;
    }

    int epfd = ::epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        ::close(listen_fd);
        return 1;
    }

    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = listen_fd;
    if (::epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl listen_fd");
        ::close(epfd);
        ::close(listen_fd);
        return 1;
    }

    std::cout << "[C++/epoll] Listening on port " << PORT << "\n";

    epoll_event events[MAX_EVENTS];

    while (true) {
        int n = ::epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t evs = events[i].events;

            if (fd == listen_fd) {
                // accept 루프
                while (true) {
                    sockaddr_in caddr{};
                    socklen_t clen = sizeof(caddr);
                    int cfd = ::accept(listen_fd,
                                       reinterpret_cast<sockaddr*>(&caddr),
                                       &clen);
                    if (cfd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        perror("accept");
                        break;
                    }

                    if (make_socket_nonblocking(cfd) == -1) {
                        perror("fcntl client");
                        ::close(cfd);
                        continue;
                    }

                    epoll_event cev{};
                    cev.events  = EPOLLIN;
                    cev.data.fd = cfd;
                    if (::epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev) == -1) {
                        perror("epoll_ctl client");
                        ::close(cfd);
                        continue;
                    }

                    std::cout << "[C++/epoll] client fd=" << cfd
                              << " connected, ip="
                              << ::inet_ntoa(caddr.sin_addr)
                              << " port=" << ntohs(caddr.sin_port)
                              << "\n";
                }
            } else {
                if (evs & (EPOLLERR | EPOLLHUP)) {
                    std::cout << "[C++/epoll] fd=" << fd << " error/hup\n";
                    ::close(fd);
                    ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    continue;
                }

                if (evs & EPOLLIN) {
                    char buf[BUF_SIZE];

                    while (true) {
                        ssize_t cnt = ::read(fd, buf, sizeof(buf));
                        if (cnt == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                // 지금은 더 읽을 데이터 없음
                                break;
                            }
                            perror("read");
                            ::close(fd);
                            ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                            break;
                        } else if (cnt == 0) {
                            std::cout << "[C++/epoll] client fd=" << fd
                                      << " closed\n";
                            ::close(fd);
                            ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                            break;
                        } else {
                            ssize_t w = ::write(fd, buf, cnt);
                            if (w == -1) {
                                perror("write");
                                ::close(fd);
                                ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    ::close(epfd);
    ::close(listen_fd);
    return 0;
}
