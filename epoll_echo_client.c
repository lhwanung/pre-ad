#include <stdio.h>      // 표준 입출력 (printf, fprintf 등)
#include <stdlib.h>     // 표준 라이브러리 (exit 등)
#include <string.h>     // 문자열 처리 (memset, strlen 등)
#include <unistd.h>     // POSIX 함수 (read, write, close 등)
#include <arpa/inet.h>  // 네트워크 관련 (sockaddr_in, inet_pton 등)

#define PORT     5000   // 서버 포트 번호 (하드코딩)
#define BUF_SIZE 1024   // 송수신 버퍼 크기

int main(int argc, char *argv[]) {
    // 인수 검증: 서버 IP 하나를 받아야 함
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];              // 명령행에서 전달된 서버 IP
    int sock = socket(AF_INET, SOCK_STREAM, 0);   // TCP 소켓 생성
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serv_addr;                 // 서버 주소 구조체 (IPv4)
    memset(&serv_addr, 0, sizeof(serv_addr));     // 구조체 초기화
    serv_addr.sin_family = AF_INET;               // 주소 체계: IPv4
    serv_addr.sin_port   = htons(PORT);           // 포트: 호스트 바이트 -> 네트워크 바이트

    // 문자열 형태의 IP를 이진 형태로 변환하여 serv_addr.sin_addr에 저장
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        exit(1);
    }

    // 서버에 연결 시도
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    printf("[C client] Connected to %s:%d\n", server_ip, PORT); // 연결 성공 메시지

    char send_buf[BUF_SIZE];   // 사용자 입력 저장 버퍼
    char recv_buf[BUF_SIZE];   // 서버로부터 받은 데이터 저장 버퍼

    // 메인 루프: 사용자 입력을 서버로 보내고, 에코 응답을 읽어 출력
    while (1) {
        printf("message> ");
        fflush(stdout);                      // 프롬프트 즉시 출력

        if (!fgets(send_buf, sizeof(send_buf), stdin)) // 표준 입력에서 한 줄 읽기
            break;                             // EOF 또는 에러면 종료

        size_t len = strlen(send_buf);
        if (len == 0)
            continue;                          // 빈 문자열이면 다음 반복

        // 서버로 데이터 전송
        if (write(sock, send_buf, len) < 0) {
            perror("write");
            break;
        }

        // 서버로부터 에코(응답) 수신
        ssize_t n = read(sock, recv_buf, sizeof(recv_buf) - 1);
        if (n <= 0) {
            if (n < 0) perror("read");        // 읽기 실패 시 에러 출력
            break;                            // 연결 종료 또는 에러면 루프 탈출
        }
        recv_buf[n] = '\0';                    // 문자열 종료자 추가
        printf("[echo] %s", recv_buf);         // 에코 메시지 출력
    }

    close(sock); // 소켓 닫기
    return 0;
}
