# udp_client.py
import socket

BUF_SIZE = 1024
HOST = '127.0.0.1'  # 서버 IP
PORT = 9999         # 서버 Port

# 1. 소켓 생성 (socket)
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
    
    # 2. connect() 과정이 없음
    
    while True:
        try:
            message = input("서버로 보낼 메시지 (종료: 'exit'): ")
            if message.lower() == 'exit':
                break

            # 3. 데이터 송신 (sendto)
            # C의 sendto()와 동일
            # 메시지를 바이트로 인코딩하여 (HOST, PORT) 주소로 보냄
            # 데이터를 보낼 때, 매번 받는 사람 주소 명시
            s.sendto(message.encode('utf-8'), (HOST, PORT))

            # 4. 데이터 수신 (recvfrom)
            # 서버로부터 에코(응답) 데이터를 받음
            # 데이터를 받을 때, 보낸 사람 주소가 항상 반환
            data, addr = s.recvfrom(BUF_SIZE)
            
            print(f"서버({addr})로부터 에코: {data.decode('utf-8')}")

        except KeyboardInterrupt:
            print("\n사용자에 의해 종료됩니다.")
            break

print("클라이언트를 종료합니다.")