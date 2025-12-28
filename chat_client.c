/* chat_client.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024

void * recv_msg(void * arg); // 수신 스레드가 실행할 함수
void error_handling(char * message);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    
    pthread_t recv_thread_id; // 수신 스레드의 ID 저장
    char msg[BUF_SIZE];

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("\n[알림] 서버(%s)와(과) 채팅이 시작되었습니다.\n", argv[1]);

    // 1. "수신" 스레드 생성 및 시작
    if (pthread_create(&recv_thread_id, NULL, recv_msg, (void *)&sock) != 0) {
        error_handling("pthread_create() error");
    }

    // 2. "송신"은 메인 스레드가 담당
    printf("[나]: ");
    // printf가 버퍼에 쌓아둔 출력 내용을 즉시 화면에 표시
    fflush(stdout);
    
    while (fgets(msg, BUF_SIZE, stdin) != NULL) 
    {
        if (strcmp(msg, "exit\n") == 0) {
            printf("[알림] 채팅을 종료합니다.\n");
            shutdown(sock, SHUT_WR); // "나 이제 보낼 거 없어" (FIN 전송)
            break;
        }
        
        // fgets로 받은 메시지를 sock을 통해 서버로 전송
        if (write(sock, msg, strlen(msg)) == -1) {
            printf("[알림] 서버와의 연결이 끊겼습니다.\n");
            break;
        }
        
        printf("[나]: ");
        fflush(stdout);
    }

    // 3. 메인 스레드 종료 전, 수신 스레드가 끝날 때까지 대기
    pthread_join(recv_thread_id, NULL);

    printf("클라이언트를 종료합니다.\n");
    // (close(sock)은 수신 스레드가 이미 처리했을 것임)
    return 0;
}

// 수신 스레드 함수 (서버와 100% 동일)
void * recv_msg(void * arg)
{
    int sock = *((int*)arg);
    // main에서 넘겨준 &sock를 원래 타입으로
    // *(int*): 포인터가 가리키는 곳의 실제 값 가져옴
    char msg[BUF_SIZE];
    int str_len;

    while ((str_len = read(sock, msg, BUF_SIZE - 1)) > 0) 
    {
        msg[str_len] = 0; 
        printf("\r[상대방]: %s", msg);
        printf("[나]: ");
        fflush(stdout);
    }

    printf("\n[알림] 상대방이 연결을 종료했습니다.\n[나]: ");
    fflush(stdout);
    
    close(sock); // 소켓을 닫음 (메인 스레드의 write()가 실패하도록 유도)
    return NULL;
}

void error_handling(char *message)
{
    perror(message);
    exit(1);
}