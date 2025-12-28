/* udp_client.c */
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
    char message[BUF_SIZE];
    int str_len;
    socklen_t adr_sz;
    
    // serv_adr: 메시지를 "보낼" 서버의 주소
    // from_adr: 메시지를 "받은" 서버의 주소 (확인용)
    struct sockaddr_in serv_adr, from_adr;

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 1. 소켓 생성 (socket) - SOCK_DGRAM (UDP)
    sock = socket(PF_INET, SOCK_DGRAM, 0);   
    if (sock == -1)
        error_handling("socket() error");

    // "받는 사람" (서버)의 주소 구조체 설정
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]); // 서버 IP
    serv_adr.sin_port = htons(atoi(argv[2]));      // 서버 Port
    
    // 2. connect() 과정이 없습니다!

    while (1) 
    {
        fputs("Input message (Q to quit): ", stdout);
        fgets(message, sizeof(message), stdin);
        
        if (strcmp(message, "q\n") == 0 || strcmp(message, "Q\n") == 0)
            break;

        // 3. 데이터 송신 (sendto)
        // "serv_adr" (서버 주소)로 "message"를 보냄
        sendto(sock, message, strlen(message), 0, 
               (struct sockaddr*)&serv_adr, sizeof(serv_adr));
        
        adr_sz = sizeof(from_adr);
        
        // 4. 데이터 수신 (recvfrom)
        // 서버로부터 온 에코 메시지를 받음
        str_len = recvfrom(sock, message, BUF_SIZE, 0, 
                           (struct sockaddr*)&from_adr, &adr_sz);
        
        message[str_len] = 0; // 문자열 종료 처리
        printf("Message from server: %s", message);
    }

    // 5. 소켓 닫기 (close)
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    perror(message);
    exit(1);
}