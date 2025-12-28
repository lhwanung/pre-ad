/* client.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUF_SIZE];
    int str_len;

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 1. 소켓 생성 (socket)
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    // 서버 주소 구조체 초기화
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // 연결할 서버 IP [cite: 257]
    serv_addr.sin_port = htons(atoi(argv[2]));      // 연결할 서버 Port [cite: 215]

    // 2. 서버에 연결 요청 (connect) [cite: 1510]
    // &: 구조체의 주소를 넘겨야 하기 때문
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");
    
    printf("Connected to server...\n");

    while(1) 
    {
        // 키보드로부터 메시지 입력
        fputs("Input message (Q to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);
        
        if (strcmp(message, "q\n") == 0 || strcmp(message, "Q\n") == 0)
            break;

        // 3. 데이터 송신 (write)
        // write(int fd, const void *buf, size_t nbytes)
        // fd 데이터를 보낼 파일 디스크립터
        // buf 보낼 데이터가 저장된 버퍼의 주소
        // nbytes 보낼 데이터 길이
        write(sock, message, strlen(message));
        
        // 4. 데이터 수신 (read) - 서버의 에코 수신
        // read(int fd, void *buf, size_t nbytes)
        // fd 데이터를 받을 소켓
        // buf 수신된 데이터를 저장할 버퍼
        // read()는 실제로 받은 바이트 수를 반환
        str_len = read(sock, message, BUF_SIZE - 1);
        message[str_len] = 0; // 문자열의 끝을 표시
        
        printf("Message from server: %s", message);
    }
    
    // 5. 소켓 닫기 (close)
    printf("Connection closed.\n");
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    perror(message);
    exit(1);
}