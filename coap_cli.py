import asyncio
from aiocoap import * # CoAP 클라이언트 관련 클래스(함수)들 임포트: aiocoap.()라 안 해도 됨

async def main():
    # 1. CoAP 클라이언트 컨텍스트 생성
    protocol = await Context.create_client_context()

    # 요청할 URI (localhost의 sensor 리소스)
    uri = 'coap://127.0.0.1/sensor'

    print(f"대상 URI: {uri}\n")

    # --- [TEST 1] GET 요청 (현재 상태 확인) ---
    print("1. GET 요청 보내기 (상태 확인)...")
    
    # GET 메시지 생성
    request_get = Message(code=GET, uri=uri)
    
    try:
        # 요청 전송 및 응답 대기
        response = await protocol.request(request_get).response
        print(f"   [응답] 결과 코드: {response.code}")
        print(f"   [응답] 데이터: {response.payload.decode()}\n")
    except Exception as e:
        print(f"   [오류] GET 요청 실패: {e}")

    # --- [TEST 2] PUT 요청 (상태 변경) ---
    print("2. PUT 요청 보내기 (상태를 'ON'으로 변경)...")
    
    # payload에 'ON'을 담아서 PUT 메시지 생성
    request_put = Message(code=PUT, uri=uri, payload=b"ON")
    
    try:
        response = await protocol.request(request_put).response
        print(f"   [응답] 결과 코드: {response.code}")
        print(f"   [응답] 변경된 데이터: {response.payload.decode()}\n")
    except Exception as e:
        print(f"   [오류] PUT 요청 실패: {e}")

    # --- [TEST 3] 다시 GET 요청 (변경 확인) ---
    print("3. GET 요청 보내기 (변경 확인)...")
    request_get_again = Message(code=GET, uri=uri)
    response = await protocol.request(request_get_again).response
    print(f"   [응답] 데이터: {response.payload.decode()}")

if __name__ == "__main__":
    asyncio.run(main())