# tcp_client.py
import socket

HOST = '127.0.0.1'  # 서버의 IP 주소 (위 서버 코드와 동일해야 함)
PORT = 9999        # 서버의 포트 번호 (위 서버 코드와 동일해야 함)

# 1. 소켓 생성
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    
    # 2. 서버에 연결 (Connect)
    # 서버의 HOST와 PORT로 연결을 시도합니다.
    try:
        s.connect((HOST, PORT))
    except ConnectionRefusedError:
        print(f"[!] 서버({HOST}:{PORT})에 연결할 수 없습니다. 서버가 실행 중인지 확인하세요.")
        exit()
        
    print(f"[*] 서버({HOST}:{PORT})에 연결되었습니다.")

    try:
        while True:
            # 3. 사용자 입력 및 데이터 송신 (Send)
            message = input("서버로 보낼 메시지 (종료: 'exit'): ")
            if message.lower() == 'exit':
                break
                
            # 메시지를 바이트(bytes)로 인코딩하여 서버에 전송
            s.sendall(message.encode('utf-8'))
            
            # 4. 데이터 수신 (Receive)
            # 서버로부터 에코(응답) 데이터를 받습니다.
            data = s.recv(1024)
            
            print(f"[Server Echo] {data.decode('utf-8')}")
            
    except KeyboardInterrupt:
        print("\n[!] 사용자에 의해 종료됩니다.")
        
    # 5. 연결 종료 (Close)
    # 'with' 구문이 끝날 때 소켓이 자동으로 닫힙니다.
    print("[*] 서버와의 연결을 종료합니다.")