# ChattingServer + MonitoringServer

IOCP 기반 멀티스레드 채팅 서버와, 그 채팅 서버의 상태를 수집·표시하는 모니터링 서버 한 쌍입니다.
15,000명 동접 더미 트래픽 환경에서 안정 동작을 검증한 상태입니다.

```
┌──────────────────┐   LAN(20000)    ┌────────────────────┐   NET(21510)   ┌───────────────┐
│  ChattingServer  │ ───────────────▶│  MonitoringServer  │◀───────────────│ Monitor Tool  │
│  (port 21501)    │   서버 지표 송신 │  LAN: 서버 수집     │   외부 표시용   │  (관제 UI)    │
│  20,000 client   │                  │  NET: 관제툴 송신   │                │               │
└──────────────────┘                  └────────────────────┘                └───────────────┘
        ▲
        │ 21501 (게임 클라)
        │
   Game Client
```

---

## 1. 이게 뭐야?

| 컴포넌트 | 위치 | 역할 |
|---|---|---|
| **ChattingServer** | `course4/ChattingServer/ChattingServer_MultiThread/` | 채팅 클라이언트(게임)를 받는 IOCP 서버. 로그인, 섹터 이동, 채팅, 하트비트 처리. |
| **MonitoringServer** | `course5/MonitoringServer/MonitoringServer/` | LAN 서버(내부 서버 지표 수신) + NET 서버(관제 툴에 지표 송신) **두 개의 IOCP 서버를 한 프로세스에서 운영**. |

ChattingServer는 자신을 `MonitorClient`로 LAN 20000 포트에 붙여서 **자기 자신의 상태(CPU, 메모리, 세션 수, TPS …)** 를 주기적으로 보냅니다.
MonitoringServer는 그것을 받아 모아두고, 관제 툴이 NET 21510에 붙으면 **암호화된 5바이트 헤더 프로토콜**로 그대로 흘려줍니다.

---

## 2. 빠른 시작

### 빌드
- Visual Studio 2019+ / x64
- 두 프로젝트 각각 `*.vcxproj` 열어 빌드
  - `ChattingServer_MultiThread.vcxproj`
  - `MonitoringServer.vcxproj`

### 설정
실행 파일 옆에 `ServerConfig.ini` 가 있어야 합니다.

**ChattingServer/ServerConfig.ini**
```ini
[ChatServer]
PORT            = 21501
MAX_SESSION     = 20000
PACKET_CODE     = 0x77
PACKET_KEY      = 0x32
MAX_PAYLOAD     = 500

[Monitor]
IP              = 127.0.0.1   ; MonitoringServer 의 LAN IP
PORT            = 20000
SERVER_NO       = 3           ; 이 채팅서버를 식별하는 번호

[Pool]
MAX_PACKET_POOL = 50000
MAX_CHAR_POOL   = 30000

[System]
LOG_DIRECTORY   = Log
LOG_LEVEL       = DEBUG
CONSOLE_OUTPUT  = false
```

**MonitoringServer/ServerConfig.ini**
```ini
[LanServer]                   ; 내부 서버들이 지표를 올려보내는 포트
PORT            = 20000
MAX_SESSION     = 100

[NetServer]                   ; 외부 관제툴이 붙는 포트 (암호화 헤더)
PORT            = 21510
MAX_SESSION     = 200
PACKET_CODE     = 109
PACKET_KEY      = 30

[Pool]
MAX_PACKET_POOL = 10000

[System]
LOG_DIRECTORY   = Log
LOG_LEVEL       = DEBUG
CONSOLE_OUTPUT  = false
```

### 실행 순서
1. `MonitoringServer.exe` 먼저 실행 (LAN 20000 listen)
2. `ChattingServer_MultiThread.exe` 실행 (자동으로 MonitoringServer 에 MonitorClient 연결)
3. (선택) 관제 툴을 NET 21510 에 접속

### 종료
실행 중 콘솔에서 **`q`** 입력 → 정상 종료(세션 정리 + 워커 스레드 join).

---

## 3. 프로젝트 레이아웃

```
course4/ChattingServer/ChattingServer_MultiThread/
├── main.cpp                  # 진입점, ServerConfig.ini 파싱, q 입력 처리
├── ChattingServer.h/.cpp     # CLanServer 상속, 채팅 도메인 로직
├── CLanServer.h/.cpp         # IOCP 서버 프레임워크 (LAN, 2바이트 헤더)
├── CMonitorClient.h/.cpp     # MonitoringServer 로 지표 송신하는 클라
├── Session.h/.cpp            # IOCount + RELEASE_FLAG 세션
├── CPacketRingBuffer.cpp     # CPacket 풀(refcount)
├── CRingBuffer.cpp           # recv/send 링 버퍼
└── CommonProtocol.h          # CS 패킷 enum + 헤더 정의

course5/MonitoringServer/MonitoringServer/
├── MonitoringServer.cpp      # 진입점
├── CMonitoringServer.h/.cpp  # LAN+NET 서버 합쳐서 운영, 통계 표시
├── CLanServer.h/.cpp         # 내부 서버 수신용 (2바이트 헤더)
├── CNetServer.h/.cpp         # 관제툴 송신용 (5바이트 암호화 헤더)
├── SystemMonitor.h/.cpp      # CPU/메모리 등 OS 지표 수집
├── MonitorProtocol.h         # SS_/CS_MONITOR 패킷 정의
└── CommonProtocol.h          # LAN/NET 헤더 정의
```

---

## 4. 더 읽을거리

| 문서 | 내용 |
|---|---|
| **[ARCHITECTURE.md](ARCHITECTURE.md)** | 스레드 모델, IOCP, IOCount/RELEASE_FLAG, 세션 풀, 락 전략, 메모리 풀. *코드만 봐서는 안 보이는 동작 원리.* |
| **[PROTOCOL.md](PROTOCOL.md)** | LAN(2바이트) / NET(5바이트 암호화) 헤더, CS 패킷 / SS 모니터 패킷 명세. |
| **[OPERATIONS.md](OPERATIONS.md)** | 콘솔에 찍히는 지표 해석, 알람 임계값, 로그 위치, 자주 보는 트러블슈팅. |
| **[DECISIONS.md](DECISIONS.md)** | 왜 이렇게 만들었는가 (ADR). 중복 로그인 정책, 로그 다이어트, IOCount 초기값 1 등. |

---

## 5. 검증 상태 (2026-04 기준)

- 더미 클라 15,000명 동접에서 24h+ 안정 동작
- 공격 패킷 대응:
  - 헤더 Code/Key 불일치 → Disconnect + 카운터 누적
  - Len == 0 / Len > MAX_PAYLOAD → Disconnect
  - 복호화/체크섬 실패 → Disconnect
  - Recv 버퍼 오버플로우 → Disconnect (이전엔 `__debugbreak`)
- 중복 로그인: **kick old, accept new** (임시. 추후 로그인 서버로 분리 예정)
- 로그 볼륨: 10GB/h → 정상 부하시 ~수백MB/h 로 축소 (DECISIONS.md 참고)
