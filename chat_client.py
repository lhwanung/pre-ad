# chat_client.py
import socket
import threading

HOST = '127.0.0.1' # 서버 IP
PORT = 9999

# --- 수신 스레드 함수 ---
# 서버의 수신 스레드와 코드가 "완전히" 동일합니다.
# 인자로 받는 소켓(s)을 통해 "받는" 역할만 합니다.
def receive_thread(s):
    """서버로부터 메시지를 수신하고 출력하는 스레드"""
    try:
        while True:
            # 1. 서버로부터 데이터를 받음 (블로킹)
            data = s.recv(1024)
            
            # 2. 서버가 연결을 끊으면 (빈 데이터 수신)
            if not data:
                print("\n[알림] 서버가 연결을 종료했습니다.")
                break
                
            data_decoded = data.decode('utf-8')

            # 1. \r로 커서를 줄 맨 앞으로 이동
            # 2. [상대방]: 메시지 출력 (print는 기본적으로 \n 포함)
            print(f"\r[상대방]: {data_decoded}") 
            
            # 3. [나]: 프롬프트를 다음 줄에 다시 출력
            #    flush=True로 버퍼를 비워 프롬프트가 바로 보이게 함
            print("[나]: ", end="", flush=True)

    except ConnectionResetError:
        print("\n[알림] 비정상적으로 연결이 종료되었습니다.")
    except Exception as e:
        print(f"\n[오류] {e}")
    finally:
        print("수신 스레드를 종료합니다.")

# --- 메인 스레드 ---
def start_client():
    # 1. 소켓 생성 (TCP)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            # 2. 서버에 연결 (connect) (블로킹)
            s.connect((HOST, PORT))
        except ConnectionRefusedError:
            print(f"[오류] 서버({HOST}:{PORT})에 연결할 수 없습니다. 서버가 실행 중인지 확인하세요.")
            return

        print(f"\n[알림] 서버({HOST}:{PORT})에 연결되었습니다. 채팅을 시작합니다.")
        
        # 3. "수신" 스레드 생성 및 시작
        receiver = threading.Thread(target=receive_thread, args=(s,))
        receiver.start()

        # 4. "송신"은 메인 스레드가 담당
        try:
            while True:
                # 5. 사용자 입력 대기 (블로킹)
                message = input("[나]: ")
                
                if message.lower() == 'exit':
                    print("[알림] 채팅을 종료합니다.")
                    break
                    
                # 6. 서버에 메시지 전송
                s.sendall(message.encode('utf-8'))
        
        except (ConnectionResetError, BrokenPipeError):
            print("\n[알림] 서버와의 연결이 끊어져 메시지를 보낼 수 없습니다.")
        except Exception as e:
            print(f"\n[오류] {e}")
        finally:
            print("클라이언트를 종료합니다.")
            receiver.join() # 수신 스레드가 완전히 끝날 때까지 기다림

if __name__ == "__main__":
    start_client()