/* chat_server_multi.c (요구사항 2: 보낸 사람 제외) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 100 

void * handle_client(void * arg); 
void broadcast_msg(char * msg, int sender_sock); 
void error_handling(char * message);

int client_socks[MAX_CLIENTS]; 
int client_count = 0; 
pthread_mutex_t clients_mutex; 

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    pthread_t thread_id; 

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    // 뮤텍스 초기화, pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
    if (pthread_mutex_init(&clients_mutex, NULL) != 0) {
        perror("pthread_mutex_init() error");
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    // 소켓 계층 옵션 설정
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    printf("Multi-Thread Chat Server started on port %s...\n", argv[1]);

    while (1) 
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*) &clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1) {
            perror("accept() error"); 
            continue; 
        }
       
        // 뮤텍스 잠금, pthread_mutex_lock(pthread_mutex_t *mutex)
        // 잠궜으면 다른 스레드는 대기
        pthread_mutex_lock(&clients_mutex); 
        if (client_count < MAX_CLIENTS) {
            client_socks[client_count++] = clnt_sock;
        } else {
            printf("Max clients reached. Connection rejected.\n");
            close(clnt_sock);
            // 뮤텍스 잠금 해제
            pthread_mutex_unlock(&clients_mutex); 
            continue;
        }
        pthread_mutex_unlock(&clients_mutex); 

        if (pthread_create(&thread_id, NULL, handle_client, (void *)(intptr_t)clnt_sock) != 0) {
            // intptr_t: 정수 값(소켓 번호) 자체를 보내기 위해 사용
            perror("pthread_create() error");
            pthread_mutex_lock(&clients_mutex);
            client_count--;
            pthread_mutex_unlock(&clients_mutex);
            close(clnt_sock);
        }

        pthread_detach(thread_id); 
        printf("New client connected. (IP: %s, Socket: %d)\n", inet_ntoa(clnt_addr.sin_addr), clnt_sock);
    }
    
    close(serv_sock);
    // 뮤텍스 해제(메모리 반환): 스레드 종료 시 호출
    // pthread_mutex_destroy(pthread_mutex_t *mutex)
    pthread_mutex_destroy(&clients_mutex); 
    return 0;
}

void * handle_client(void * arg)
{
    int clnt_sock = (intptr_t)arg;
    int str_len = 0;
    char msg[BUF_SIZE];
    char broadcast_buffer[BUF_SIZE + 50]; 

    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    // 소켓이 연결된 피어의 주소 검색. sockaddr 구조체에 주소 저장
    getpeername(clnt_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    char * clnt_ip = inet_ntoa(clnt_addr.sin_addr);
    int clnt_port = ntohs(clnt_addr.sin_port);

    // 입장 메시지 (프롬프트 미포함)
    // sprintf(char str, const char format, ...)
    // : 서식을 지정하여 데이터를 문자열로 만들고, 그 결과를 특정 문자열 버퍼에 저장
    sprintf(broadcast_buffer, "\r[알림] (%s:%d) 님이 입장하셨습니다.\n", clnt_ip, clnt_port);
    broadcast_msg(broadcast_buffer, clnt_sock); // sender_sock을 제외하고 전송

    while ((str_len = read(clnt_sock, msg, BUF_SIZE - 1)) > 0)
    {
        msg[str_len] = 0; 
        printf("[%s:%d]: %s", clnt_ip, clnt_port, msg); // 서버 콘솔 출력
        
        // 브로드캐스트 메시지 (프롬프트 미포함)
        sprintf(broadcast_buffer, "\r[%s:%d]: %s", clnt_ip, clnt_port, msg); 
        
        broadcast_msg(broadcast_buffer, clnt_sock); // sender_sock을 제외하고 전송
    }

    // --- 종료 처리 ---
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (client_socks[i] == clnt_sock) {
            client_socks[i] = client_socks[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("[알림] (%s:%d) 님이 퇴장하셨습니다.\n", clnt_ip, clnt_port);
    
    // 퇴장 메시지
    sprintf(broadcast_buffer, "\r[알림] (%s:%d) 님이 퇴장하셨습니다.\n", clnt_ip, clnt_port);
    broadcast_msg(broadcast_buffer, clnt_sock); // sender_sock을 제외하고 전송

    close(clnt_sock); 
    return NULL;
}

// [핵심] 보낸 사람(sender)을 "제외"하고 전송
void broadcast_msg(char * msg, int sender_sock)
{
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < client_count; i++)
    {
        // 보낸 사람을 "제외"하는 if문
        if (client_socks[i] != sender_sock)
        {
            if(write(client_socks[i], msg, strlen(msg)) <= 0)
            {
                 // 전송 실패
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex); 
}

void error_handling(char *message)
{
    perror(message);
    exit(1);
}