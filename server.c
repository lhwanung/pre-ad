/* server.c */
#include <stdio.h>      // for printf(), perror()
#include <stdlib.h>     // for exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for write(), read(), close()
#include <arpa/inet.h>  // for sockaddr_in, htonl(), htons(), INADDR_ANY; 현재 실행중인 컴퓨터의 IP를 소켓에 부여, inet_ntoa
#include <sys/socket.h> // for socket(), bind(), listen(), accept()

#define BUF_SIZE 1024

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock; // 서버 소켓 (리스닝 소켓)
    int clnt_sock; // 클라이언트 소켓 (데이터 통신용)

    struct sockaddr_in serv_addr; // 서버 주소 구조체
    struct sockaddr_in clnt_addr; // 클라이언트 주소 구조체
    socklen_t clnt_addr_size;

    char message[BUF_SIZE];
    int str_len;

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 1. 소켓 생성 (socket) [cite: 1452]
    // PF_INET = IPv4, SOCK_STREAM = TCP
    // 세 번째 매개변수 실제로 사용할 통신 프로토콜
    // IPPROTO_TCP, IPPROTO_UDP, IPPROTO_HOPOPTS(= 0); 자동
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // 주소 구조체 초기화
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                 // 주소체계를 IPv4 [cite: 111]
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // '0.0.0.0'과 동일 (모든 IP에서 접속 허용) [cite: 356]
    // struct in_addr sin_addr; 32비트 IP 주소 in_addr_t s_addr; 32비트 IPv4 인터넷 주소
    serv_addr.sin_port = htons(atoi(argv[1]));      // 포트번호 (네트워크 바이트 순서로 변환) [cite: 215]

    // 2. 주소 할당 (bind) [cite: 1461]
    // bind함수는 범용 주소 구조체인 sockaddr 포인터를 받기 때문에 형변환
    // 1. endpoint 소켓 2. IP, port#를 저장하기 위한 변수가 있는 구조체
    // 3. 두 번째 인자의 데이터 크기
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    
    // 3. 연결 대기 (listen) [cite: 1476]
    // 연결 대기열(backlog) 크기를 5로 설정
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    printf("Waiting for client connection...\n");

    clnt_addr_size = sizeof(clnt_addr);

    // 4. 연결 수락 (accept) [cite: 1489]
    // 클라이언트가 접속할 때까지 여기서 "블로킹" (멈춤)
    // 접속이 되면, 통신용 새 소켓(clnt_sock)이 생성됨
    // accept()가 실행이 끝난 후, 주소에 찾아 가서 새로운 값(실제 채워진 크기)으로 덮어 씀
    // 1. socket 함수 호출시 반환된 소켓
    // 2. accpet 함수 성공시, 연결된 client의 IP, port#가 담기는 구조체
    // 3. sockaddr 구조체의 크기
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1)
        error_handling("accept() error");
    
    // inet_addr(): 문자열 -> 숫자(인터넷 네트워크 주소), inet_ntoa(): 숫자 -> 문자
    printf("Client connected from %s\n", inet_ntoa(clnt_addr.sin_addr));

    // 5. 데이터 수신(read) 및 송신(write)
    // 리눅스는 소켓을 파일처럼 다루므로 read/write 사용 [cite: 1120]
    // str_len이 0이 되면 (클라이언트가 연결을 끊으면) 반복문 종료
    // clnt_sock 소켓으로부터 BUF_SIZE만큼 데이터를 읽어 message 버퍼에 저장
    while ((str_len = read(clnt_sock, message, BUF_SIZE)) != 0) 
    { 
        // (Echo) 받은 데이터를 그대로 클라이언트에게 전송
        write(clnt_sock, message, str_len);
    }

    // 6. 소켓 닫기 (close)
    printf("Client disconnected.\n");
    close(clnt_sock); // 클라이언트와의 통신 소켓 닫기
    close(serv_sock); // 서버의 리스닝 소켓 닫기
    return 0;
}

void error_handling(char *message)
{
    perror(message); // 오류 메시지 출력
    exit(1);
}