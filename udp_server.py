# udp_server.py
import socket

# C의 #define BUF_SIZE 1024
BUF_SIZE = 1024
HOST = '127.0.0.1'  # '0.0.0.0' (INADDR_ANY)도 가능
PORT = 9999

# 1. 소켓 생성 (socket)
# C의 socket(PF_INET, SOCK_DGRAM, 0)과 동일
# 'with' 구문을 사용해 자동 close 처리
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
    
    # 2. 주소 할당 (bind)
    # C의 bind() 함수와 동일. (HOST, PORT) 튜플을 사용
    s.bind((HOST, PORT))
    print(f"UDP 서버가 {HOST}:{PORT} 에서 대기 중입니다...")

    # 3. listen()과 accept()가 없음

    while True:
        # 4. 데이터 수신 (recvfrom)
        # C의 recvfrom()과 동일
        # data: 수신된 바이트 데이터
        # addr: (ip, port) 튜플로 된 클라이언트 주소
        data, addr = s.recvfrom(BUF_SIZE)
        
        print(f"{addr} 로부터 메시지 받음: {data.decode('utf-8')}")

        # 5. 데이터 송신 (sendto) - 에코
        # C의 sendto()와 동일
        # 받은 데이터(data)를 받은 주소(addr)로 다시 보냄
        s.sendto(data, addr)