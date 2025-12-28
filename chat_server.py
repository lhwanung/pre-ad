# chat_server.py
import socket
import threading

HOST = '127.0.0.1'
PORT = 9999

# --- 수신 스레드 함수 ---
# 이 함수는 conn 소켓을 인자로 받아, 오직 "받는" 역할만 담당합니다.
def receive_thread(conn):
    """클라이언트로부터 메시지를 수신하고 출력하는 스레드"""
    try:
        while True:
            # 1. 클라이언트로부터 데이터를 받음 (블로킹)
            data = conn.recv(1024)
            
            # 2. 클라이언트가 연결을 끊으면 (빈 데이터 수신)
            if not data:
                print("\n[알림] 상대방이 연결을 종료했습니다.")
                break
                
            data_decoded = data.decode('utf-8')

            # 1. \r로 커서를 줄 맨 앞으로 이동
            # 2. [상대방]: 메시지 출력 (print는 기본적으로 \n 포함)
            print(f"\r[상대방]: {data_decoded}") 
            
            # 3. [나]: 프롬프트를 다음 줄에 다시 출력
            #    flush=True로 버퍼를 비워 프롬프트가 바로 보이게 함
            print("[나]: ", end="", flush=True)

    # 클라이언트가 비정상적으로(강제 종료 등) 연결을 끊었을 때 사용
    except ConnectionResetError:
        print("\n[알림] 비정상적으로 연결이 종료되었습니다.")
    except Exception as e:
        print(f"\n[오류] {e}")
    finally:
        print("수신 스레드를 종료합니다.")
        # 수신이 끝났다는 것은 어차피 채팅이 끝났다는 의미이므로, 
        # 메인 스레드도 종료될 수 있도록 conn을 닫아버릴 수 있지만, 
        # 여기서는 단순히 스레드만 종료합니다.
        
# --- 메인 스레드 ---
def start_server():
    # 1. 소켓 생성 (TCP)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        
        print(f"채팅 서버가 {HOST}:{PORT}에서 시작되었습니다. 클라이언트 대기 중...")
        
        # 2. 클라이언트 접속 대기 (accept) (블로킹)
        conn, addr = s.accept()
        
        with conn:
            print(f"\n[알림] {addr}와(과) 채팅이 시작되었습니다.")
            
            # 3. "수신" 스레드 생성 및 시작
            # target: 실행할 함수 이름
            # args: 해당 함수에 전달할 인자 (튜플 형태, 인자가 1개라도 콤마(,) 필수!)
            receiver = threading.Thread(target=receive_thread, args=(conn,))
            receiver.start() # 스레드 시작

            # 4. "송신"은 메인 스레드가 담당
            try:
                while True:
                    # 5. 사용자 입력 대기 (블로킹)
                    message = input("[나]: ")
                    
                    if message.lower() == 'exit':
                        print("[알림] 채팅을 종료합니다.")
                        break
                        
                    # 6. 클라이언트에 메시지 전송
                    conn.sendall(message.encode('utf-8'))
            
            except (ConnectionResetError, BrokenPipeError):
                print("\n[알림] 상대방과의 연결이 끊어져 메시지를 보낼 수 없습니다.")
            except Exception as e:
                print(f"\n[오류] {e}")
            finally:
                print("서버를 종료합니다.")
                # 송신 루프가 끝나면 스레드도 정리
                receiver.join() # 수신 스레드가 완전히 끝날 때까지 기다림

# 해당 구문이 사용된 파이썬 파일을 직접 실행했을 때만 실행
if __name__ == "__main__":
    start_server()