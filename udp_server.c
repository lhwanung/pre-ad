/* udp_server.c */
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
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_adr_sz; // 클라이언트 주소 크기 변수
    
    struct sockaddr_in serv_adr, clnt_adr; // 서버 주소, 클라이언트 주소 구조체

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 1. 소켓 생성 (socket)
    // SOCK_DGRAM = UDP
    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // 주소 구조체 초기화
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    // 2. 주소 할당 (bind)
    // TCP와 동일하게 bind는 필요
    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    printf("UDP Server waiting on port %s...\n", argv[1]);

    // 3. listen()과 accept()가 없습니다!
    
    while (1) 
    {
        clnt_adr_sz = sizeof(clnt_adr);
        
        // 4. 데이터 수신 (recvfrom)
        // read() 대신 recvfrom() 사용
        // 데이터가 올 때까지 "블로킹"
        // clnt_adr에 "데이터를 보낸 클라이언트의 주소"가 채워짐 [cite: 2271]
        str_len = recvfrom(serv_sock, message, BUF_SIZE, 0, 
                           (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        
        printf("Message from client %s:%d\n", 
               inet_ntoa(clnt_adr.sin_addr), ntohs(clnt_adr.sin_port));

        // 5. 데이터 송신 (sendto) - 에코
        // write() 대신 sendto() 사용
        // "데이터를 받았던 그 주소(clnt_adr)"로 다시 전송 [cite: 2288]
        sendto(serv_sock, message, str_len, 0, 
               (struct sockaddr*)&clnt_adr, clnt_adr_sz);
    }

    // 6. 소켓 닫기 (close)
    // (이 예제는 무한 루프라 여기에 도달하지 않음)
    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    perror(message);
    exit(1);
}