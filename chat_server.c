/* chat_server.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // read, write, close
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>    // POSIX Threads

#define BUF_SIZE 1024

void * recv_msg(void * arg); // 수신 스레드가 실행할 함수
void error_handling(char * message);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    
    pthread_t recv_thread_id; // 수신 스레드의 ID를 저장할 변수
    char msg[BUF_SIZE];

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    printf("Waiting for client connection...\n");

    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1)
        error_handling("accept() error");
        
    printf("\n[알림] (%s)와(과) 채팅이 시작되었습니다.\n", inet_ntoa(clnt_addr.sin_addr));

    // 1. "수신" 스레드 생성 및 시작
    // (clnt_sock의 "주소"를 넘겨줌)
    // 스레드 함수의 이름은 그 자체로 함수의 주소
    if (pthread_create(&recv_thread_id, NULL, recv_msg, (void *)&clnt_sock) != 0) {
        error_handling("pthread_create() error");
    }

    // 2. "송신"은 메인 스레드가 담당
    printf("[나]: ");
    fflush(stdout); // 프롬프트가 바로 보이도록 버퍼 비우기
    
    while (fgets(msg, BUF_SIZE, stdin) != NULL) 
    {
        if (strcmp(msg, "exit\n") == 0) {
            printf("[알림] 채팅을 종료합니다.\n");
            shutdown(clnt_sock, SHUT_WR); // "나 이제 보낼 거 없어" (FIN 전송)
            break;
        }
        
        // write()가 -1을 반환하면 (상대방이 끊음), 루프 종료
        if (write(clnt_sock, msg, strlen(msg)) == -1) {
            printf("[알림] 상대방과의 연결이 끊겼습니다.\n");
            break;
        }

        printf("[나]: ");
        fflush(stdout);
    }

    // 3. 메인 스레드 종료 전, 수신 스레드가 끝날 때까지 대기
    pthread_join(recv_thread_id, NULL);

    printf("서버를 종료합니다.\n");
    close(clnt_sock);
    close(serv_sock);
    return 0;
}

// 수신 스레드 함수
void * recv_msg(void * arg)
{
    // (void*)로 전달받은 인자를 (int*)로 변환 후, *로 역참조하여 값을 얻음
    int sock = *((int*)arg); 
    char msg[BUF_SIZE];
    int str_len;

    // read()가 0 (연결 종료) 또는 -1 (오류)을 반환할 때까지 반복
    while ((str_len = read(sock, msg, BUF_SIZE - 1)) > 0) 
    {
        msg[str_len] = 0; // 문자열의 끝을 표시
        
        // [콘솔 정리 트릭]
        printf("\r[상대방]: %s", msg); // \r로 줄의 처음으로 이동
        printf("[나]: "); // 프롬프트 다시 출력
        fflush(stdout); // 버퍼 즉시 비우기
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