# tcp_server.py
import socket

# 서버의 IP 주소와 사용할 포트 번호
HOST = '127.0.0.1'  # 로컬호스트(자기 자신) 주소
PORT = 9999        # 1024 이후의 임의의 포트 번호

# 1. 소켓 생성
# AF_INET: IPv4 주소 체계를 사용
# SOCK_STREAM: TCP 프로토콜을 사용
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    
    # 2. 주소 할당 (Bind)
    # 소켓을 특정 IP 주소와 포트 번호에 연결합니다.
    s.bind((HOST, PORT))
    print(f"[*] 서버가 {HOST}:{PORT} 에서 대기 중입니다...")

    # 3. 연결 대기 (Listen)
    # 클라이언트의 연결 요청을 기다리기 시작합니다.
    # (1)은 동시에 1개의 연결 요청을 대기열에 둘 수 있음을 의미합니다.
    s.listen(1)

    # 4. 연결 수락 (Accept) - 중요!
    # 클라이언트가 접속하면, '연결'을 수락하고
    # 통신을 위한 새로운 소켓(conn)과 클라이언트 주소(addr)를 반환합니다.
    # 이 부분에서 프로그램은 클라이언트가 접속할 때까지 "멈춰" 있습니다 (Blocking)
    conn, addr = s.accept()
    
    # 'with' 구문을 사용하여 conn 소켓을 자동으로 닫도록 합니다.
    with conn:
        print(f"[*] {addr} 에서 클라이언트가 접속했습니다.")
        
        while True:
            # 5. 데이터 수신 (Receive)
            # 클라이언트로부터 데이터를 받습니다. (최대 1024바이트)
            # 데이터가 없으면 이 부분에서 "멈춰" 있습니다 (Blocking)
            data = conn.recv(1024)
            
            # 클라이언트가 연결을 끊으면 (빈 데이터를 보내면) 루프 종료
            if not data:
                break
                
            # 받은 데이터를 UTF-8로 디코딩하여 출력
            print(f"[Client] {data.decode('utf-8')}")

            # 6. 데이터 송신 (Send/Echo)
            # 받은 데이터를 그대로 클라이언트에게 다시 보냅니다.
            conn.sendall(data)
            
    print(f"[*] {addr} 클라이언트와의 연결이 종료되었습니다.")