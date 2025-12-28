import asyncio # 비동기 프로그래밍을 위한 asyncio 임포트
import aiocoap.resource as resource # CoAP 리소스 관련 클래스 임포트
import aiocoap # CoAP 프로토콜 관련 클래스 임포트

class SensorResource(resource.Resource): 
    # resource.Resource 클래스를 상속받아 새로운 리소스 정의(CoAP에서 모든 대상은 리소스)
    # CoAP 네트워크상의 하나의 주소(URL)처럼 동작
    """
    이 클래스는 '센서' 리소스를 나타냅니다.
    CoAP 서버에서 GET, POST, PUT, DELETE 요청을 처리하는 기본 클래스
    """
    def __init__(self):
        super().__init__() # 부모 클래스 초기화(안 하면 라이브러리 고장)

        # 초기 센서 상태 (예: LED가 꺼져 있음)
        self.content = b"OFF" # b: 바이트 문자열로 상태 저장
        # 네트워크 통신은 바이트 단위로 이루어지기 때문

    async def render_get(self, request): # async 함수로 GET 요청 처리
        """
        클라이언트가 GET 요청을 보냈을 때 호출됩니다.
        현재 상태(self.content)를 반환합니다.
        """
        # 디코딩: 바이트를 사람이 읽을 수 있는 문자열로 변환
        print(f"[Server] GET 요청 수신: 현재 상태 반환 -> {self.content.decode()}")

        # 응답 메시지 생성 (payload에 현재 상태 담기)
        return aiocoap.Message(payload=self.content)

    async def render_put(self, request):
        """
        클라이언트가 PUT 요청을 보냈을 때 호출됩니다.
        요청에 담긴 payload로 상태를 업데이트합니다.
        """
        # 클라이언트가 보낸 데이터 확인
        new_content = request.payload
        print(f"[Server] PUT 요청 수신: 상태 변경 요청 -> {new_content.decode()}")
        
        # 상태 업데이트
        self.content = new_content
        
        # 변경되었다는 응답 반환
        return aiocoap.Message(code=aiocoap.CHANGED, payload=self.content)
        # 코드 2.04는 성공적으로 변경되었음을 의미

async def main():
    # aiocoap의 리소스 루트(트리) 생성
    # CoAP 서버가 관리하는 모든 리소스의 최상위 루트
    root = resource.Site()
    
    # 'sensor'라는 경로에 SensorResource 인스턴스 연결
    # 접속 주소 예: coap://localhost/sensor
    root.add_resource(['sensor'], SensorResource())

    # 서버 바인딩 (UDP 5683 포트가 CoAP 표준)
    # CoAP 서버 컨텍스트 생성, 비동기적으로 소켓에 바인딩하여 서버 시작
    # 컨텍스트: 서버의 실행 환경(네트워크 소켓 등)을 관리
    # create_server_context는 비동기 함수이므로 await 필요
    # await은 CPU가 다른 작업을 할 수 있도록 함
    # await은 OS 레벨에서 비동기 작업(바인딩)이 완료될 때까지 대기
    await aiocoap.Context.create_server_context(root, bind=('127.0.0.1', 5683))
    
    print("CoAP 서버가 실행 중입니다... (Port: 5683)")
    
    # 서버가 계속 실행되도록 유지
    # 현재 실행 중인 이벤트 루프를 가져와서 무한 대기 상태의 Future 객체 생성
    # create_future는 해당 이벤트 루프에 소속된 미완료(pending 상태) Future 객체를 생성
    # await: 생성한 Future가 완료될 때까지 현재 코루틴을 무한정 대기
    # 코루틴: 중단했다가 다시 이어 실행할 수 있는 함수
    # Future: 비동기 작업의 결과를 나타내는 객체
    await asyncio.get_running_loop().create_future()

if __name__ == "__main__":
    # 비동기 루프 실행
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n서버를 종료합니다.")