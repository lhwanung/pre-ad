# chat_server_multi.py
import socket
import threading

HOST = '127.0.0.1'
PORT = 9999

# 1. 공유 자원: 접속한 모든 클라이언트의 소켓 리스트
client_list = []
# 2. 공유 자원 접근을 제어하기 위한 락(Lock)
# 한 번에 한 스레드만 접근하도록
client_lock = threading.Lock()

# 전파할 message(바이트), 그 메시지를 보낸 사람을 인자로
def broadcast(message, sender_conn):
    """모든 클라이언트에게 메시지를 브로드캐스트 (발신자 제외)"""
    
    # 3. 락을 획득하여 리스트를 안전하게 순회
    # 이 with 블록이 실행되는 동안, 다른 스레드가 client_lock을 사용하려 하면 대기시킴
    with client_lock:
        # 리스트를 순회하는 도중 리스트가 변경될 수 있으므로 사본을 만듭니다.
        clients_to_send = list(client_list)
        
    for client_conn in clients_to_send:
        # 메시지를 보낸 클라이언트를 제외한 모든 클라이언트에게 전송
        if client_conn != sender_conn:
            try:
                client_conn.sendall(message)
            except Exception as e:
                print(f"[오류] 브로드캐스트 실패 (to {client_conn}): {e}")
                # 실패한 클라이언트는 나중에 client_handler에서 제거됨

def client_handler(conn, addr):
    """
    각 클라이언트를 전담하는 스레드 함수
    메시지를 수신하고, 이를 모든 클라이언트에게 브로드캐스트
    accept()로 새 클라이언트 접속할 때마다 이 함수를 실행하는 스레드 1개 생성
    """
    print(f"[알림] 새 클라이언트 접속: {addr}")
    welcome_msg = f"\n[알림] {addr} 님이 입장하셨습니다.\n".encode('utf-8')
    broadcast(welcome_msg, conn) # 입장 메시지 브로드캐스트

    try:
        while True:
            # 4. 클라이언트로부터 메시지 수신 (블로킹)
            data = conn.recv(1024)
            if not data:
                break # 클라이언트가 연결을 끊음 (빈 데이터)
            
            # 5. 수신한 메시지를 다른 모든 클라이언트에게 브로드캐스트
            # (보낸 사람의 주소 정보 포함)
            message_to_send = f"[{addr[0]}:{addr[1]}]: {data.decode('utf-8')}".encode('utf-8')
            broadcast(message_to_send, conn)
            
            # 서버 콘솔에도 메시지 출력
            print(f"[{addr[0]}:{addr[1]}]: {data.decode('utf-8')}")

    except ConnectionResetError:
        print(f"[알림] {addr} 클라이언트와 비정상적으로 연결이 종료되었습니다.")
    except Exception as e:
        print(f"[오류] {addr} - {e}")
    finally:
        # 6. 클라이언트 접속 종료 처리
        print(f"[알림] {addr} 님이 퇴장하셨습니다.")
        
        # 7. 공유 리스트에서 해당 클라이언트 제거 (락 사용!)
        with client_lock:
            if conn in client_list:
                client_list.remove(conn)
        
        # 퇴장 메시지 브로드캐스트
        exit_msg = f"\n[알림] {addr} 님이 퇴장하셨습니다.\n".encode('utf-8')
        broadcast(exit_msg, conn)
        
        conn.close() # 소켓 닫기

def start_server():
    """메인 스레드: 서버를 시작하고 클라이언트 접속만 처리"""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # (옵션) 주소 재사용
        # 서버를 비정상 종료했다가 바로 다시 켰을 때, 
        # bind 함수가 "Address already in use (주소가 이미 사용 중)" 오류를 내는 것을 방지
        s.bind((HOST, PORT))
        s.listen()
        
        print(f"멀티 채팅 서버가 {HOST}:{PORT}에서 시작되었습니다...")

        while True:
            try:
                # 8. 메인 스레드는 새 클라이언트 접속만 기다림 (블로킹)
                conn, addr = s.accept()
                
                # 9. 새 클라이언트가 오면, 공유 리스트에 추가 (락 사용!)
                with client_lock:
                    client_list.append(conn)
                
                # 10. 새 클라이언트를 전담할 스레드 생성 및 시작
                handler_thread = threading.Thread(target=client_handler, args=(conn, addr))
                handler_thread.start()
                
            except KeyboardInterrupt:
                print("\n[알림] 서버를 종료합니다. (Ctrl+C)")
                break
            except Exception as e:
                print(f"[오류] 서버 accept 실패: {e}")
                
    # 서버 종료 시 모든 클라이언트 연결 닫기
    with client_lock:
        for client in client_list:
            client.close()
    print("모든 클라이언트 연결을 닫고 서버를 종료합니다.")

if __name__ == "__main__":
    start_server()