/* chat_client.c (요구사항 1: 스스로 출력 안 함) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024

void * recv_msg(void * arg); 
void error_handling(char * message);

int g_sock; 
char g_my_id[30]; 
pthread_mutex_t g_print_mutex; // mutex 선언

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr;
    pthread_t recv_thread_id;
    char msg[BUF_SIZE];

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    if (pthread_mutex_init(&g_print_mutex, NULL) != 0) {
        perror("pthread_mutex_init() error");
        exit(1);
    }

    g_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (g_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // 문자열 -> 네트워크 정수
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(g_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    // 내 ID 설정
    struct sockaddr_in my_addr;
    socklen_t my_addr_len = sizeof(my_addr);
    // 소켓의 로컬 이름 검색, 소켓의 주소(이름)을 sockaddr 구조체 포인터에 저장
    getsockname(g_sock, (struct sockaddr*)&my_addr, &my_addr_len);
    sprintf(g_my_id, "[%s:%d]", inet_ntoa(my_addr.sin_addr), ntohs(my_addr.sin_port));
    
    /*  int inet_pton(int af(예: AF_INET), const char *src(예: "127.0.0.1"), void *dst(예: &addr.sin_addr));
        사람이 읽는 문자열 형태(presentation)의 IP 주소를 네트워크 바이트 순서의 이진 형태로
        const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
        네트워크 바이트 순서의 IP 주소를 사람이 읽을 수 있는 문자열로 반환
        af: 주소 체계, src: 변환할 네트워크 바이트 순서의 주소(예: &addr.sin_addr)
        dst: 결과 문자열을 저장할 버퍼, size: 버퍼 크기 
        
        // 개선된 코드 (IPv4 & IPv6 모두 지원)
        if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
            error_handling("inet_pton() error");
        }

        // IPv6의 경우
        if (inet_pton(AF_INET6, argv[1], &serv_addr.sin6_addr) <= 0) {
            error_handling("inet_pton() error");
        }
    */
    
    printf("\n[알림] 서버(%s)와(과) 채팅이 시작되었습니다. (내 ID: %s)\n", argv[1], g_my_id);

    if (pthread_create(&recv_thread_id, NULL, recv_msg, NULL) != 0) {
        error_handling("pthread_create() error");
    }
    
    // 첫 번째 프롬프트 출력
    pthread_mutex_lock(&g_print_mutex);
    printf("%s: ", g_my_id); 
    fflush(stdout);
    pthread_mutex_unlock(&g_print_mutex);
    
    // 스트림에서 문자열 가져옴
    while (fgets(msg, BUF_SIZE, stdin) != NULL) 
    {
        if (strcmp(msg, "exit\n") == 0) {
            printf("[알림] 채팅을 종료합니다.\n");
            shutdown(g_sock, SHUT_WR);
            break;
        }

        if (write(g_sock, msg, strlen(msg)) == -1) {
            printf("[알림] 서버와의 연결이 끊겼습니다.\n");
            break;
        }

        pthread_mutex_lock(&g_print_mutex);
        printf("%s: ", g_my_id); 
        fflush(stdout);
        pthread_mutex_unlock(&g_print_mutex);
        /* ----------------- */
    }

    pthread_join(recv_thread_id, NULL); 
    printf("클라이언트를 종료합니다.\n");
    pthread_mutex_destroy(&g_print_mutex); 
    return 0;
}

// 수신 스레드 함수
void * recv_msg(void * arg)
{
    char msg[BUF_SIZE];
    int str_len;

    // 서버가 "\r[IP:PORT]: ...\n" 형식으로 (남의 메시지만) 보내줌
    while ((str_len = read(g_sock, msg, BUF_SIZE - 1)) > 0) 
    {
        msg[str_len] = 0; 
        
        pthread_mutex_lock(&g_print_mutex); 
        
        printf("\r%s", msg);         // 1. 받은 메시지(줄바꿈 포함)를 그대로 출력
        printf("%s: ", g_my_id);  // 2. 내 프롬프트를 다음 줄에 새로 출력
        fflush(stdout);              // 3. 즉시 표시
        
        pthread_mutex_unlock(&g_print_mutex);
    }

    pthread_mutex_lock(&g_print_mutex);
    printf("\n[알림] 상대방이 연결을 종료했습니다.\n(Enter를 눌러 종료)\n");
    fflush(stdout);
    pthread_mutex_unlock(&g_print_mutex);
    
    close(g_sock); 
    return NULL;
}

void error_handling(char *message)
{
    perror(message);
    exit(1);
}