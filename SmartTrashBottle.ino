// DFPlayer Mini 핀 정의 (SoftwareSerial 사용)
#define DFP_RX 3   // 나노 D3 → DFPlayer TX
#define DFP_TX 2   // 나노 D2 → DFPlayer RX

// TCS3200 컬러 센서 핀 정의
// VCC, GND는 나노의 5V, GND 핀에 직접 연결
#define S0 4       // TCS3200 S0
#define S1 5       // TCS3200 S1
#define S2 6       // TCS3200 S2
#define S3 7       // TCS3200 S3
#define sensorOut 8     // TCS3200 OUT
#define LED 9      // TCS3200 LED

// 적외선 수신기 핀 정의
// VCC, GND는 나노의 5V, GND 핀에 직접 연결
#define IR_RECV_PIN 11

// 서보모터 핀 정의
#define SERVO_PIN 10   // D10 핀 사용
#define CLOSE_ANGLE 0  // 닫힘 각도
#define OPEN_ANGLE 90  // 열림 각도

#include <SoftwareSerial.h>
#include <DFMiniMp3.h>
#include <Servo.h>

// DFPlayer 통신용 클래스
class Mp3Notify;
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;

SoftwareSerial dfpSerial(DFP_RX, DFP_TX);  // RX, TX
DfMp3 dfPlayer(dfpSerial);

// 서보모터 객체
Servo trashServo;

// DFPlayer 알림 처리 클래스 (Makuna 라이브러리용)
class Mp3Notify
{
public:
  static void OnError(DfMp3& mp3, uint16_t errorCode)
  {
    Serial.print(F("DFPlayer Error: "));
    Serial.println(errorCode);
  }

  static void OnPlayFinished(DfMp3& mp3, DfMp3_PlaySources source, uint16_t track)
  {
    Serial.print(F("Play finished: "));
    Serial.println(track);
  }

  static void OnPlaySourceOnline(DfMp3& mp3, DfMp3_PlaySources source)
  {
    Serial.println(F("Play source online"));
  }

  static void OnPlaySourceInserted(DfMp3& mp3, DfMp3_PlaySources source)
  {
    Serial.println(F("SD card inserted"));
  }

  static void OnPlaySourceRemoved(DfMp3& mp3, DfMp3_PlaySources source)
  {
    Serial.println(F("SD card removed"));
  }
};

int redFreq = 0;
int greenFreq = 0;
int blueFreq = 0;

// 모터 상태 관리
bool isOpen = false;
unsigned long openTime = 0;
const unsigned long CLOSE_DELAY = 5000;  // 5초

// 함수 프로토타입 선언
void smoothServo(int targetAngle, int delayTime = 12);
void openLid();
void closeLid();
String detectColor(int r, int g, int b);
void playSound(int trackNumber);
void testServo();
void setup() {
  // 서보모터 초기화 (최우선 - 의도치 않은 작동 방지)
  trashServo.attach(SERVO_PIN);
  trashServo.write(CLOSE_ANGLE);  // 초기: 닫힘 상태 (0도)

  // TCS3200 신호 핀 설정
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(sensorOut, INPUT);

  // 주파수 스케일링 20% (S0=HIGH, S1=LOW)
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // LED 상시 켜기
  digitalWrite(LED, HIGH);

  // 적외선 수신기 설정
  pinMode(IR_RECV_PIN, INPUT);

  Serial.begin(9600);
  Serial.println(F("Servo motor control ready"));

  // DFPlayer Mini 초기화

  Serial.println(F("Initializing DFPlayer..."));

  dfpSerial.begin(9600);
  delay(100);

  dfPlayer.begin();
  delay(1000);  // 충분한 초기화 시간

  Serial.println(F("DFPlayer initialized!"));

  // EQ 설정 (음성 최적화)
  dfPlayer.setEq(DfMp3_Eq_Pop);  // Pop EQ - 음성에 최적
  delay(100);

  // 볼륨 설정 (최대)
  dfPlayer.setVolume(30);  // 최대 30 (0~30 범위) - 이미 최대값
  delay(200);

  // SD 카드 확인
  uint16_t count = dfPlayer.getTotalTrackCount(DfMp3_PlaySource_Sd);
  Serial.print(F("Total tracks on SD: "));
  Serial.println(count);

  // 루트 디렉토리 파일 재생 방식
  Serial.println(F("Using root directory playback (0001.mp3, 0002.mp3, 0003.mp3)"));

  Serial.println(F("Smart Trash Bottle Ready!"));
  Serial.println(F("Commands:"));
  Serial.println(F("  t=servo test, i=IR sensor test"));
  Serial.println(F("  1/2/3=play track, +=vol up, -=vol down, s=status"));
}

void loop() {
  // DFPlayer 처리 (중요!)
  dfPlayer.loop();

  // 시리얼 명령 처리
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    // 서보모터 테스트
    if (cmd == 't' || cmd == 'T') {
      testServo();
      return;
    }
    // DFPlayer 테스트 (1, 2, 3 입력 시 해당 트랙 재생)
    else if (cmd >= '1' && cmd <= '3') {
      int track = cmd - '0';
      Serial.print(F("Manual play track: "));
      Serial.println(track);
      playSound(track);
      return;
    }
    // 볼륨 증가 ('+' 입력)
    else if (cmd == '+') {
      dfPlayer.increaseVolume();
      Serial.println(F("Volume +"));
      return;
    }
    // 볼륨 감소 ('-' 입력)
    else if (cmd == '-') {
      dfPlayer.decreaseVolume();
      Serial.println(F("Volume -"));
      return;
    }
    // 상태 확인 ('s' 입력)
    else if (cmd == 's' || cmd == 'S') {
      Serial.println(F("Status:"));
      Serial.print(F("  Volume: "));
      Serial.println(dfPlayer.getVolume());
      Serial.print(F("  Total tracks: "));
      Serial.println(dfPlayer.getTotalTrackCount(DfMp3_PlaySource_Sd));
      Serial.print(F("  Servo angle (D10): "));
      Serial.print(trashServo.read());
      Serial.println(F(" degrees"));
      Serial.print(F("  IR Sensor (D11): "));
      Serial.println(digitalRead(IR_RECV_PIN) == HIGH ? "HIGH (no object)" : "LOW (object detected)");
      return;
    }
    // 적외선 센서 테스트 ('i' 입력)
    else if (cmd == 'i' || cmd == 'I') {
      Serial.println(F("=== IR Sensor Monitor ==="));
      Serial.println(F("Monitoring for 10 seconds..."));
      Serial.println(F("Wave your hand in front of the sensor!"));
      Serial.println();

      unsigned long startTime = millis();
      while (millis() - startTime < 10000) {  // 10초 동안 모니터링
        int irValue = digitalRead(IR_RECV_PIN);
        Serial.print(F("IR Sensor (D11): "));
        Serial.print(irValue == HIGH ? "HIGH" : "LOW");
        Serial.print(F(" ("));
        Serial.print(irValue);
        Serial.print(F(") - "));
        Serial.println(irValue == LOW ? "OBJECT DETECTED" : "No object");
        delay(500);  // 0.5초마다 출력
      }

      Serial.println();
      Serial.println(F("Monitoring complete!"));
      Serial.println(F("Summary:"));
      Serial.println(F("  LOW (0) = Object detected"));
      Serial.println(F("  HIGH (1) = No object"));
      return;
    }
  }

  // 적외선 센서 감지 (LOW = 물체 감지, HIGH = 감지 안됨)
  int irValue = digitalRead(IR_RECV_PIN);

  if (irValue == LOW && !isOpen) {
    // 물체 감지 시 색상 먼저 확인
    Serial.println(F("물체 감지! 색상 감지 시작"));

    delay(10);  // 센서 안정화

    // 색상 읽기
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);
    redFreq = pulseIn(sensorOut, LOW);

    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);
    greenFreq = pulseIn(sensorOut, LOW);

    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH);
    blueFreq = pulseIn(sensorOut, LOW);

    // 값 출력
    Serial.print("R:");
    Serial.print(redFreq);
    Serial.print(" G:");
    Serial.print(greenFreq);
    Serial.print(" B:");
    Serial.print(blueFreq);
    Serial.print(" -> ");

    // 색상 판별
    String color = detectColor(redFreq, greenFreq, blueFreq);
    Serial.print(F("감지된 색상: "));
    Serial.println(color);

    // 색상이 인식된 경우만 음성 재생 후 모터 작동
    if (color != "알 수 없음") {
      // 색상별 음성 재생
      if (color == "빨강") {
        Serial.println(F("캔입니다"));
        playSound(1);  // 0001.mp3
      } else if (color == "초록") {
        Serial.println(F("플라스틱입니다"));
        playSound(3);  // 0003.mp3
      } else if (color == "파랑") {
        Serial.println(F("병입니다"));
        playSound(2);  // 0002.mp3
      }

      // 음성 재생 완료 대기 (음성 길이에 맞게 조정)
      Serial.println(F("음성 재생 중... 대기"));
      delay(1000);  // 1초 대기 (빠른 모터 동작을 위해 단축)

      // 서보모터로 뚜껑 열기
      Serial.println(F("쓰레기통 열림"));
      openLid();
      isOpen = true;
      openTime = millis();
    } else {
      Serial.println(F("색상을 인식할 수 없어 쓰레기통을 열지 않습니다."));
    }
  }

  // 5초 후 자동으로 닫기
  if (isOpen && (millis() - openTime >= CLOSE_DELAY)) {
    Serial.println(F("쓰레기통 닫힘"));
    closeLid();
    isOpen = false;
  }

  delay(100);  // 짧은 딜레이로 반응성 향상
}

// 서보모터 테스트 함수
void testServo() {
  Serial.println(F("=== Servo Motor Test ==="));

  Serial.println(F("Step 1: Close (0 degrees)"));
  closeLid();
  Serial.print(F("  Current angle: "));
  Serial.println(trashServo.read());
  delay(2000);

  Serial.println(F("Step 2: Open (90 degrees)"));
  openLid();
  Serial.print(F("  Current angle: "));
  Serial.println(trashServo.read());
  delay(3000);

  Serial.println(F("Step 3: Close (0 degrees)"));
  closeLid();
  Serial.print(F("  Current angle: "));
  Serial.println(trashServo.read());
  delay(1000);

  Serial.println(F("Test Complete!"));
  Serial.println();
}

// 부드러운 서보모터 제어 함수
void smoothServo(int targetAngle, int delayTime) {
  int currentAngle = trashServo.read();

  if (currentAngle < targetAngle) {
    // 각도 증가
    for (int angle = currentAngle; angle <= targetAngle; angle++) {
      trashServo.write(angle);
      delay(delayTime);
    }
  } else {
    // 각도 감소
    for (int angle = currentAngle; angle >= targetAngle; angle--) {
      trashServo.write(angle);
      delay(delayTime);
    }
  }
}

// 뚜껑 열기
void openLid() {
  Serial.print(F("  [Servo] Opening to "));
  Serial.print(OPEN_ANGLE);
  Serial.println(F(" degrees..."));
  smoothServo(OPEN_ANGLE);
  Serial.println(F("  [Servo] Opened"));
}

// 뚜껑 닫기
void closeLid() {
  Serial.print(F("  [Servo] Closing to "));
  Serial.print(CLOSE_ANGLE);
  Serial.println(F(" degrees..."));
  smoothServo(CLOSE_ANGLE);
  Serial.println(F("  [Servo] Closed"));
}

String detectColor(int r, int g, int b) {
  // 빨강: R이 가장 낮고, G/B와 차이가 충분히 큼
  if (r < g && r < b && (g - r) > 10 && (b - r) > 10) {
    return "빨강";
  }
  // 초록: G가 가장 낮고, R/B와 차이가 충분히 큼
  else if (g < r && g < b && (r - g) > 10 && (b - g) > 10) {
    return "초록";
  }
  // 파랑: B가 가장 낮고, R/G와 차이가 충분히 큼
  else if (b < r && b < g && (r - b) > 10 && (g - b) > 10) {
    return "파랑";
  }
  else {
    return "알 수 없음";
  }
}

// DFPlayer로 음성 재생 함수 (루트 디렉토리 재생)
void playSound(int trackNumber) {
  // SD 카드 루트의 파일을 재생 (0001.mp3, 0002.mp3, 0003.mp3)
  // playGlobalTrack: 루트 디렉토리의 파일 직접 재생
  Serial.print(F("Playing track "));
  Serial.println(trackNumber);
  dfPlayer.playGlobalTrack(trackNumber);
}