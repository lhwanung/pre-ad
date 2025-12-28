import paho.mqtt.client as mqtt
import json

# MQTT 브로커 정보
BROKER_ADDRESS = "localhost"
PORT = 1883
TOPIC = "iot/sensor/temp"  # 구독할 토픽

# 1. 브로커에 성공적으로 연결되었을 때 자동으로 호출되는 콜백
# 클라이언트가 브로커에게 connect() 요청을 보내고, 
# 브로커로부터 연결 수락(CONNACK) 응답을 받았을 때 자동으로 호출
"""
client: 현재 실행 중인 클라이언트 객체 자신
userdata: 사용자가 임의로 전달할 수 있는 데이터
flags: 브로커가 보낸 세션 관련 플래그(정보)
rc: Return code, 연결 시도에 대한 브로커의 응답 코드
"""
def on_connect(client, userdata, flags, rc): 
    if rc == 0:
        print("브로커에 성공적으로 연결되었습니다.")
        # 연결 성공 시, 원하는 토픽을 구독(Subscribe)
        # 여러 토픽을 구독하려면 리스트로 [(topic1, qos), (topic2, qos)] 형태 사용 가능
        client.subscribe(TOPIC, qos=0)
        print(f"토픽 구독 시작: '{TOPIC}'")
    else:
        print(f"브로커 연결 실패. 반환 코드: {rc}")

# 2. 브로커로부터 메시지를 수신했을 때 호출되는 콜백
"""
msg(MQTTMessage 객체): 배달된 실제 소포 꾸러미
    msg.topic: 데이터가 어느 주소(토픽)로 왔는지
    msg.payload: 실제 데이터 내용(bytes 타입으로 들어옴)
    msg.qos: 메시지의 품질 레벨
    msg.retain: 이 메시지가 브로커에 저장되어 있던 최신 유지 메시지인지
"""
def on_message(client, userdata, msg):
    print(f"메시지 수신: 토픽 '{msg.topic}'")

    try:
        # 수신된 메시지(msg.payload)는 바이트(bytes) 형식이므로
        # utf-8로 디코딩한 후, json으로 파싱합니다.
        # 디코딩: 기계어(bytes)를 사람 말(string)로 번역
        # 파싱: 문자열을 프로그램 변수(JSON/Dictionary)로 변환: json.loads
        payload_data = json.loads(msg.payload.decode("utf-8"))
        
        # 데이터 처리
        # 딕셔너리에서 원하는 값 추출
        sensor_id = payload_data.get("sensor_id")
        temperature = payload_data.get("temperature")
        
        print(f"  [데이터] 센서 ID: {sensor_id}, 온도: {temperature}°C")
        
    except json.JSONDecodeError:
        print(f"  [오류] JSON 디코딩 실패: {msg.payload.decode('utf-8')}")
    except Exception as e:
        print(f"  [오류] 메시지 처리 중 오류 발생: {e}")

# 3. 메시지 발행(Publish)이 완료되었을 때 호출되는 콜백 (QoS 1, 2)
# 이 예제에서는 QoS 0을 사용하므로 on_publish는 큰 의미가 없지만,
# 참고용으로 추가합니다.
def on_publish(client, userdata, mid):
    # 이 콜백은 publisher에서도 사용할 수 있습니다.
    print(f"메시지 발행 완료 (MID: {mid})")


# MQTT 클라이언트 생성
# 클라이언트 ID를 "Subscriber_Client"로 설정
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, "Subscriber_Client")

# 콜백 함수들 연결
# 위에서 정의한 함수들을 클라이언트 객체의 이벤트 핸들러에 끼워 넣기
client.on_connect = on_connect
client.on_message = on_message
# client.on_publish = on_publish # 필요시 주석 해제

# 브로커에 연결
try:
    client.connect(BROKER_ADDRESS, PORT)
except Exception as e:
    print(f"브로커 연결 실패: {e}")
    exit()

# loop_forever()는 메시지 수신을 위해 무한 루프를 돌며 대기합니다.
# 이 함수는 블로킹(Blocking) 함수입니다.
# 데이터를 기다리는 것 외에 메인 스레드에서 할 일 X
try:
    print("구독자가 메시지를 대기합니다... (종료: Ctrl+C)")
    client.loop_forever()
except KeyboardInterrupt:
    print("구독자가 종료됩니다.")
finally:
    client.disconnect()
    print("브로커 연결이 종료되었습니다.")