#define _GNU_SOURCE              // GNU 확장 기능을 사용하도록 매크로 정의 (예: 일부 확장된 함수/플래그 사용 가능)

#include <stdio.h>               // printf, perror 등 표준 입출력 함수 선언
#include <stdlib.h>              // exit, malloc/free 등 일반 유틸 함수 선언
#include <string.h>              // memset, memcpy 등 문자열/메모리 관련 함수 선언
#include <unistd.h>              // close, read, write, fcntl 등 POSIX 함수 선언
#include <errno.h>               // errno 전역 변수 및 오류 코드 상수 정의
#include <fcntl.h>               // fcntl, O_NONBLOCK 등 파일 제어 및 플래그 상수 정의
#include <sys/types.h>           // 시스템 자료형 정의 (size_t, ssize_t, socklen_t 등)
#include <sys/socket.h>          // socket, bind, listen, accept, setsockopt 등 소켓 함수 선언
#include <sys/epoll.h>           // epoll_create1, epoll_ctl, epoll_wait 등 epoll 관련 함수/구조체 선언
#include <netinet/in.h>          // sockaddr_in 구조체, AF_INET, INADDR_ANY 등 인터넷 주소 관련 상수/구조체
#include <arpa/inet.h>           // htons, htonl, ntohs, ntohl 등 바이트 순서 변환 함수

#define PORT       5000          // 서버가 바인드하고 listen할 TCP 포트 번호
#define MAX_EVENTS 128           // epoll_wait에서 한 번에 처리할 수 있는 최대 이벤트 수
#define BUF_SIZE   1024          // 클라이언트로부터 읽고 쓸 때 사용할 버퍼 크기

// 소켓을 논블로킹 모드로 변경하는 유틸리티 함수
static int make_socket_nonblocking(int fd) { // static 쓰는 이유: 이 함수가 정의된 파일 내에서만 사용되도록 제한
    int flags = fcntl(fd, F_GETFL, 0);             // 현재 파일 디스크립터의 플래그를 가져옴
    if (flags == -1) return -1;                    // 실패 시 -1 반환

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) // 기존 플래그에 O_NONBLOCK을 OR 해서 다시 설정
        // O_NONBLOCK 플래그: 읽기/쓰기 호출이 즉시 완료될 수 없을 때 블록하지 않고 -1과 errno=EAGAIN/EWOULDBLOCK 반환
        return -1;                                 // 설정 실패 시 -1 반환

    return 0;                                      // 성공 시 0 반환
}
/*
fcntl(int fd, int cmd, ...); // fcntl: file control
  - fd: 제어할 파일 디스크립터 (여기서는 소켓)
  - cmd:
      * F_GETFL: 현재 상태 플래그(읽기/쓰기 모드, 논블로킹 여부 등)를 가져옴
      * F_SETFL: 상태 플래그를 설정
      * F_DUPFD: 파일 디스크립터 복사: dup1, dup2와 유사
      * F_GETFD: 파일 디스크립터 플래그(예: FD_CLOEXEC)를 가져옴
      * F_SETFD: 파일 디스크립터 플래그를 설정
  - 추가 인자 (...):
      * F_SETFL일 때, 설정할 플래그 값 (기존 플래그 | O_NONBLOCK 처럼 사용)
O_NONBLOCK:
  - 읽기/쓰기 호출이 즉시 완료될 수 없을 때, 블록(대기)하지 않고 -1과 errno=EAGAIN/EWOULDBLOCK을 반환하도록 함
*/

int main(void) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);   // 서버 리슨용 TCP 소켓 생성
    if (listen_fd == -1) {                             // 소켓 생성 실패 시
        perror("socket");                              // 오류 메시지 출력
        exit(1);                                       // 비정상 종료
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 포트 재사용 옵션 설정
    /*
    setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
      - sockfd: 옵션을 설정할 소켓 디스크립터
      - level: 옵션 수준
          * SOL_SOCKET: 소켓 레벨 옵션
      - optname:
          * SO_REUSEADDR: TIME_WAIT 상태의 포트를 바로 재사용 가능하게 하는 옵션
      - optval: 옵션 값이 들어있는 버퍼 주소 (&opt)
      - optlen: 옵션 값의 크기 (sizeof(opt))
    */

    struct sockaddr_in serv_addr;                      // 서버 주소 정보를 담을 구조체
    memset(&serv_addr, 0, sizeof(serv_addr));          // 구조체 전체를 0으로 초기화
    serv_addr.sin_family      = AF_INET;               // 주소 패밀리: IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);     // 모든 NIC(인터페이스)에서 오는 연결 수신
    serv_addr.sin_port        = htons(PORT);           // 포트를 네트워크 바이트 오더로 설정

    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind");                               // 바인드 실패 시 오류 출력
        close(listen_fd);                             // 리슨 소켓 닫기
        exit(1);                                      // 종료
    }

    if (listen(listen_fd, SOMAXCONN) == -1) {
        perror("listen");                             // listen 실패 시 오류 출력
        close(listen_fd);                             // 리슨 소켓 닫기
        exit(1);                                      // 종료
    }
    /*
    listen(int sockfd, int backlog);
      - sockfd: 접속 요청을 받을 리슨 소켓
      - backlog: 커널이 대기열에 쌓아둘 수 있는 최대 대기 연결 수
          * SOMAXCONN: 시스템이 허용하는 최대값 사용
    역할:
      - 소켓을 '수동 대기' 상태로 만들어 클라이언트의 connect 요청을 받을 준비를 함
    */

    if (make_socket_nonblocking(listen_fd) == -1) {   // 리슨 소켓을 논블로킹 모드로 설정
        perror("fcntl");                              // 설정 실패 시 오류 출력
        close(listen_fd);                             // 소켓 닫기
        exit(1);                                      // 종료
    }

    int epfd = epoll_create1(0);                      // epoll 인스턴스 생성
    if (epfd == -1) {                                 // 실패 시
        perror("epoll_create1");                      // 오류 출력
        close(listen_fd);                             // 리슨 소켓 닫기
        exit(1);                                      // 종료
    }
    /*
    epoll_create1(int flags);
      - flags:
          * 0: 특별한 플래그 없이 기본 epoll 인스턴스 생성
          * EPOLL_CLOEXEC 등 사용 가능
      - 반환값:
          * 성공: epoll 인스턴스의 파일 디스크립터
          * 실패: -1 (errno 설정)
    epoll 인스턴스:
      - 여러 파일 디스크립터(소켓 등)에 대한 이벤트(읽기/쓰기/에러)를 감시하는 커널 객체
    */

    struct epoll_event ev;                            // epoll에 등록할 이벤트 구조체
    ev.events  = EPOLLIN;                             // 읽기 가능 이벤트(데이터 도착/새 연결) 감지
    ev.data.fd = listen_fd;                           // 이 이벤트는 리슨 소켓과 연관

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl listen_fd");                // 리슨 소켓 epoll 등록 실패 시
        close(epfd);                                  // epoll 인스턴스 닫기
        close(listen_fd);                             // 리슨 소켓 닫기
        exit(1);                                      // 종료
    }
    /*
    epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
      - epfd: epoll 인스턴스의 파일 디스크립터 (epoll_create1로 생성)
      - op: 수행할 동작
          * EPOLL_CTL_ADD: 새로운 fd를 epoll에 등록
          * EPOLL_CTL_MOD: 기존 fd의 이벤트 수정
          * EPOLL_CTL_DEL: fd를 epoll에서 제거
      - fd: 감시 대상 파일 디스크립터 (여기서는 listen_fd)
      - event: 감시할 이벤트 종류 및 관련 데이터 (EPOLLIN, EPOLLOUT 등 + data 필드)
    struct epoll_event {
        uint32_t events;  // 이벤트 비트 플래그 (EPOLLIN, EPOLLOUT, EPOLLERR 등)
        epoll_data_t data;// 사용자 정의 데이터 (여기서는 fd를 저장)
    };
    */

    printf("[C/epoll] Listening on port %d\n", PORT); // 서버가 해당 포트에서 리슨 중이라고 출력

    struct epoll_event events[MAX_EVENTS];            // epoll_wait 결과를 담을 배열 (최대 MAX_EVENTS개)

    while (1) {                                       // 메인 이벤트 루프 (무한 루프)
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1); // 이벤트가 발생할 때까지 대기
        if (n == -1) {                                // epoll_wait 실패 시
            if (errno == EINTR) continue;             // 시그널로 인한 중단(EINTR)이면 다시 대기
            perror("epoll_wait");                     // 그 외 에러는 출력
            break;                                    // 루프 종료
        }
        /*
        epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
          - epfd: epoll 인스턴스 디스크립터
          - events: 발생한 이벤트를 담을 배열의 포인터
          - maxevents: 위 배열에 담을 수 있는 최대 이벤트 개수
          - timeout:
              * -1: 무한 대기
              * 0: 즉시 반환 (논블로킹)
              * 양수: 밀리초 단위 타임아웃
          - 반환값:
              * 발생한 이벤트 수 (0 이상)
              * 실패 시 -1 (errno 설정)
        */
        
        for (int i = 0; i < n; i++) {                 // 발생한 각 이벤트를 순회하면서 처리
            int fd = events[i].data.fd;               // 이벤트와 연관된 파일 디스크립터
            uint32_t evs = events[i].events;          // 어떤 이벤트(EPOLLIN, EPOLLERR 등)가 발생했는지

            if (fd == listen_fd) {                    // 리슨 소켓에서 이벤트 발생: 새 클라이언트 연결 도착
                // 새 연결 처리 (accept 루프)
                while (1) {
                    struct sockaddr_in caddr;         // 클라이언트 주소 정보
                    socklen_t clen = sizeof(caddr);   // 주소 길이
                    int cfd = accept(listen_fd, (struct sockaddr*)&caddr, &clen); // 새 연결 수락

                    if (cfd == -1) {                  // accept 실패
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;                    // 논블로킹: 더 이상 대기 중인 연결 없음, 루프 탈출
                        perror("accept");             // 다른 에러는 출력
                        break;                        // 그리고 루프 탈출
                    }

                    if (make_socket_nonblocking(cfd) == -1) { // 새 클라이언트 소켓을 논블로킹으로 전환
                        perror("fcntl client");        // 실패 시 에러 출력
                        close(cfd);                    // 해당 클라이언트 소켓 닫기
                        continue;                      // 다음 연결 처리 시도
                    }

                    struct epoll_event cev;            // 클라이언트용 epoll 이벤트 구조체
                    cev.events  = EPOLLIN;             // 이 클라이언트에서 읽기 이벤트(데이터 도착) 감시
                    cev.data.fd = cfd;                 // 이 이벤트의 data에 클라이언트 fd 저장

                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev) == -1) {
                        perror("epoll_ctl client");    // 클라이언트 fd epoll 등록 실패
                        close(cfd);                    // 소켓 닫기
                        continue;                      // 다음 클라이언트 처리
                    }

                    printf("[C/epoll] client fd=%d connected\n", cfd); // 새 클라이언트 접속 로그 출력
                }
            } else {
                // 클라이언트 소켓(fd)에 대한 이벤트 처리
                if (evs & (EPOLLERR | EPOLLHUP)) {     // 에러 또는 연결 종료(HUP) 이벤트
                    printf("[C/epoll] fd=%d error/hup\n", fd);
                    close(fd);                         // 소켓 닫기
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // epoll 감시 목록에서 제거
                    continue;                          // 다음 이벤트 처리
                }

                if (evs & EPOLLIN) {                   // 읽기 가능 이벤트가 발생한 경우
                    char buf[BUF_SIZE];                // 읽기/쓰기용 버퍼

                    while (1) {                        // 가능한 만큼 반복해서 읽기
                        ssize_t cnt = read(fd, buf, sizeof(buf)); // 클라이언트로부터 데이터 읽기
                        if (cnt == -1) {               // read 에러
                            if (errno == EAGAIN || errno == EWOULDBLOCK) { 
                                // 논블로킹: 현재 시점에는 더 이상 읽을 데이터 없음
                                break;                 // 읽기 루프 종료, 다음 이벤트로
                            }
                            perror("read");            // 다른 read 에러
                            close(fd);                 // 소켓 닫기
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // epoll에서 제거
                            break;                     // 읽기 루프 종료
                        } else if (cnt == 0) {
                            // 클라이언트가 orderly shutdown (FIN 보냄): 연결 종료
                            printf("[C/epoll] client fd=%d closed\n", fd);
                            close(fd);                 // 소켓 닫기
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // epoll에서 제거
                            break;                     // 읽기 루프 종료
                        } else {
                            // cnt > 0 인 경우: 실제로 cnt 바이트만큼 데이터를 읽어옴
                            // 에코 서버: 받은 데이터를 그대로 다시 클라이언트에게 돌려줌
                            ssize_t w = write(fd, buf, cnt); // 읽은 만큼 그대로 쓰기
                            if (w == -1) {             // write 에러
                                perror("write");
                                close(fd);             // 소켓 닫기
                                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // epoll에서 제거
                                break;                 // 읽기 루프 종료
                            }
                        }
                    }
                }
            }
        }
    }

    close(epfd);                                      // epoll 인스턴스 닫기
    close(listen_fd);                                 // 리슨 소켓 닫기
    return 0;                                         // 정상 종료
}