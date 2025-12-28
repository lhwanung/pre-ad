import asyncio
from aioquic.asyncio import QuicConnectionProtocol, serve
from aioquic.quic.configuration import QuicConfiguration

# 1. QUIC 이벤트 처리 클래스
class MyQuicProtocol(QuicConnectionProtocol):
    
    # 2. 스트림(데이터 통로)이 생성될 때 호출
    def quic_event_received(self, event):
        if hasattr(event, "stream_id"):
            print(f"[서버] 스트림 {event.stream_id}에서 이벤트 수신")
            
            # 3. 클라이언트가 보낸 데이터(stream_data)가 있으면
            if hasattr(event, "data") and event.data:
                data_str = event.data.decode()
                print(f"[서버] 클라이언트로부터 데이터 수신: {data_str}")
                
                response = f"서버가 '{data_str}'를 받았습니다."
                
                # 4. 응답을 보내고 스트림을 닫음 (FIN)
                self._quic.send_stream_data(event.stream_id, response.encode(), end_stream=True)
                print(f"[서버] 클라이언트에게 응답 전송: {response}")

async def main():
    print("QUIC 서버를 시작합니다...")
    
    # 1. QUIC 설정 로드 (암호화 설정)
    configuration = QuicConfiguration(is_client=False)
    configuration.load_cert_chain(
        certfile="server.crt",  # 아까 생성한 인증서
        keyfile="server.key"    # 아까 생성한 키
    )

    # 2. 0.0.0.0 (모든 IP)의 4433 포트에서 서버 실행
    # create_protocol에는 이벤트 처리 클래스를 지정
    await serve(
        host="127.0.0.1",
        port=4433,
        configuration=configuration,
        create_protocol=MyQuicProtocol,
    )
    
    print("서버가 4433 포트에서 대기 중...")
    await asyncio.Event().wait()  # 서버가 계속 실행되도록 대기

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("서버 종료.")