import asyncio
from aioquic.asyncio import connect, QuicConnectionProtocol
from aioquic.quic.configuration import QuicConfiguration

# 1. QuicConnectionProtocol을 상속받는, 우리가 만들 이벤트 처리기
class MyQuicClientProtocol(QuicConnectionProtocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._stream_id = None       # 우리가 사용할 스트림 ID
        self._done_event = asyncio.Event() # main()에게 "통신 끝났다"고 알릴 신호

    # 2. [연결 성공 시] 자동으로 호출되는 함수
    def connection_made(self, transport):
        super().connection_made(transport)
        print("[클라이언트] 서버에 연결되었습니다 (Connection Made).")
        
        # 3. 새 스트림 생성
        self._stream_id = self._quic.get_next_available_stream_id(is_unidirectional=False)
        
        # 4. 서버로 데이터 전송
        message = "Hello, QUIC World!"
        print(f"[클라이언트] 서버로 데이터 전송: {message}")
        self._quic.send_stream_data(
            self._stream_id,
            message.encode(),
            end_stream=False # 응답을 받아야 함
        )
        print("[클라이언트] 서버 응답 대기 중...")

    # 5. [이벤트 수신 시] 자동으로 호출되는 함수 (가장 중요)
    def quic_event_received(self, event):
        # 6. 우리가 연 스트림에서 온 이벤트인지 확인
        if hasattr(event, "stream_id") and event.stream_id == self._stream_id:
            
            # 7. 데이터가 포함된 이벤트인지 확인
            if hasattr(event, "data"):
                response_str = event.data.decode()
                print(f"[클라이언트] 서버로부터 응답 수신: {response_str}")
            
            # 8. 서버가 스트림을 닫았거나, 우리가 데이터를 받았다면
            if event.end_stream or (hasattr(event, "data")):
                print("[클라이언트] 통신 완료.")
                self._done_event.set() # main()에 "끝났다"고 신호 보냄
        
        # (다른 모든 이벤트는 부모 클래스가 처리하도록 넘김)
        super().quic_event_received(event)

    # 9. main() 함수가 기다릴 수 있도록 하는 함수
    async def wait_for_done(self):
        await self._done_event.wait()


# --- 메인 실행 로직 ---
async def main():
    print("QUIC 클라이언트를 시작합니다...")
    
    configuration = QuicConfiguration(is_client=True)
    configuration.load_verify_locations(cafile="server.crt") 

    try:
        # 10. 'create_protocol'에 우리가 만든 클래스를 지정
        #     이제 'connection' 변수는 MyQuicClientProtocol의 인스턴스가 됨
        async with connect(
            "127.0.0.1", 
            4433, 
            configuration=configuration,
            create_protocol=MyQuicClientProtocol 
        ) as connection:
            
            # 11. connection_made가 실행되고, 
            #     quic_event_received가 신호를 줄 때까지 기다림
            await connection.wait_for_done()

    except ConnectionError as e:
        print(f"[클라이언트] 연결 실패: {e}")
    except Exception as e:
        print(f"[클라이언트] 오류 발생: {e}")

if __name__ == "__main__":
    asyncio.run(main())