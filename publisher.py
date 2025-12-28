import paho.mqtt.client as mqtt # MQTT 통신을 위한 핵심 라이브러리
import time # 데이터 전송 간격(delay)을 주거나 타임스탬프를 찍기 위해 사용
import json # 파이썬의 딕셔너리 데이터를 IoT 표준 통신 포맷인 JSON 문자열로 반환하기 위해
import random # 실제 센서가 없으므로, 가짜 온도 값을 무작위로 생성

# MQTT 브로커 정보
BROKER_ADDRESS = "localhost"  # 127.0.0.1 또는 실제 브로커 IP
PORT = 1883
TOPIC = "iot/sensor/temp"  # 데이터를 발행할 토픽

# MQTT 클라이언트 생성(mqtt.Client) 
# 클라이언트 ID를 "Publisher_Client"로 설정
# mqtt.CallbackAPIVersion.VERSION1: Paho 라이브러리의 최신 버전에서 요구하는 콜백 API 버전 지정
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, "Publisher_Client")

# 브로커에 연결
try:
    client.connect(BROKER_ADDRESS, PORT)
    print(f"브로커({BROKER_ADDRESS}:{PORT})에 연결되었습니다.")
except Exception as e:
    print(f"브로커 연결 실패: {e}")
    exit()

# 백그라운드에서 네트워크 루프를 시작
# MQTT 통신을 위한 별도의 백그라운드 스레드 시작
""" 
이 함수가 있어야 네트워크 트래픽(데이터 전송, 수신 확인 등)을
메인 프로그램의 흐름과 상관없이 자동으로 처리
"""
client.loop_start() # 백그라운드 네트워크 스레드 시작

print("발행을 시작합니다...") 

try:
    while True:
        # 1. 시뮬레이션 데이터 생성 (예: 20~25도 사이의 온도)
        # round(..., 2): 소수점 둘째 자리까지만 남기고 반올림
        # random.uniform(20.0, 25.0): 20.0에서 25.0 사이 임의의 실수(float) 생성
        temperature = round(random.uniform(20.0, 25.0), 2)
        
        # 2. 데이터를 JSON 형식으로 만들기
        # 실제로 보낼 데이터 꾸러미
        # IoT 데이터는 보통 JSON 형식을 많이 사용합니다.
        # json.dumps(): 파이썬 딕셔너리를 JSON 문자열로 직렬화(serialize)
        # 네트워크로 데이터를 보낼 땐 객체 상태가 아닌 문자열(바이트)상태여야
        payload = json.dumps({
            "sensor_id": "TEMP-001",
            "temperature": temperature,
            "timestamp": time.time() # 현재 시간 반환
        })
        
        # 3. 데이터 발행 (Publish)
        # client.publish(토픽, 메시지, QoS): 데이터를 브로커에게 전송
        result = client.publish(TOPIC, payload, qos=0) 
        # qos=0: QoS 레벨 0. 최대 한 번 전송(Fire and Forget), 전송 보장은 하지 않지만 가장 빠르고 가벼움
        # qos=1: 전송 보장(At least once), 중복 가능
        # qos=2: 전송 완벽 보장(Exactly once), 중복 X, 유실 X
        # QoS: 메시지 전달 보장 수준
        # 데이터 통신 네트워크상에 흐르는 데이터의 중요도를 분류하여, 이를 기반으로 우선순위를 부여
        # 우선 순위가 높을수록 빠르고 안전하게 전송 보장

        # 발행 성공 여부 확인
        # result.rc: Return code(MQTT publish 요청의 결과 코드)
        # MQTT_ERR_SUCCESS는 성공(0) 의미
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"발행 성공: 토픽 '{TOPIC}', 메시지: {payload}")
        else:
            print(f"발행 실패: {result}")
            
        # 5초 대기(5초 동안 프로세스 일시 정지)
        time.sleep(5)
        
except KeyboardInterrupt:
    # 사용자가 Ctrl+C를 누르면 종료
    print("발행을 중지합니다.")

finally:
    # 네트워크 루프 정지 및 연결 종료
    client.loop_stop() # 백그라운드 네트워크 스레드 정지
    client.disconnect() # 브로커와의 연결 끊음
    print("브로커 연결이 종료되었습니다.")

# json.load(): json 파일을 읽어서 json object를 python dict로 가져옴
# json.loads(): string 타입 객체를 dict 타입으로 가져옴
# json.dump(): python dict 타입의 객체를 json 파일로 씀(JSON 문자열을 파일로 직접 출력)
# json.dumps(): python dit 타입의 객체를 string 타입으로 가져옴