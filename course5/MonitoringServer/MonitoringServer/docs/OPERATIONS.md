# OPERATIONS

운영자가 매일 봐야 하는 것들.

---

## 1. 채팅 서버 콘솔 보는 법

```
======================================================================
                       ChattingServer Status
                       2026-04-09 14:23:11
----------------------------------------------------------------------
  CPU: 23%       Memory: 1842 MB
  Session: 14987
----------------------------------------------------------------------
  Update TPS: 41203    Accept TPS: 12    Total Accept: 1234567
  PacketPool: 7821/50000   CharPool: 14987/30000
  SendBufFull: 0    HeartbeatTimeout: 3
  InvCode:0  InvLen:2  DecodeFail:0
----------------------------------------------------------------------
  [Thread Activity]
    Join:0  Leave:0  Login:1  SectorMove:0
    ChatMsg:2  Heartbeat:0  TimerLock:0
----------------------------------------------------------------------
```

| 항목 | 의미 | 정상 범위 (15K 동접) | 알람 임계 |
|---|---|---|---|
| **CPU** | 프로세스 CPU % | < 30% | > 70% 30초 지속 |
| **Memory** | 프로세스 RSS (MB) | 1.5–2 GB 평탄 | 시간당 +200 MB 이상 증가 = leak 의심 |
| **Session** | 활성 세션 수 | == 동접 | maxSession 의 95% 도달 |
| **Update TPS** | 1초당 OnRecv 콜 수 | 동접 × 2~5 정도 | 0 에 가까워지면 워커 스턱 |
| **Accept TPS** | 신규 접속/초 | 정상 운영 < 50 | > 200 (재접속 폭주 / 공격) |
| **PacketPool** | 사용/총 | 사용량이 maxPool 의 80% 이내 | 100% 도달 시 SendPacket 실패 |
| **CharPool** | 캐릭터 객체 사용/총 | == Session 수 | maxPool 도달 시 로그인 실패 |
| **SendBufFull** | 누적 SendBuf 가득참 → Disconnect | 0 유지 | 분당 +수십 = 일부 클라 receive 멈춤 |
| **HeartbeatTimeout** | 40초 무응답 끊김 누적 | 천천히 증가 | 분당 +수백 = 라우팅 / 클라이언트 이슈 |
| **InvCode / InvLen / DecodeFail** | 공격 패킷 누적 | 0 또는 매우 천천히 | 분당 수천 이상 = 본격 공격 트래픽 |

### Thread Activity 가 뭐에 좋은가
모든 카운터는 **현재 그 핸들러 안에 들어가 있는 스레드 수**입니다. (락 진입 시 +1, 빠지면 -1)
- 정상적으로는 거의 0~1
- 특정 카운터가 **수십**으로 굳어 있다 = 그 핸들러에서 락 경합이 심하거나 데드락
- `TimerLock` 이 계속 양수 = TimerThread 가 `_characterLock` 못 잡는 중 = 도메인 워커가 락 들고 안 놔주는 중

---

## 2. 모니터링 서버 콘솔 보는 법

```
=================== MonitoringServer ===================
LAN Sessions: 1 / 100      NET Sessions: 0 / 200
LAN Accept TPS: 0          NET Accept TPS: 0
LAN Recv TPS: 8            NET Recv TPS: 0
LAN InvLen: 0
NET InvCode: 0   InvLen: 0   DecodeFail: 0
---------------------------------------------------------
[ServerNo 3] CHAT  CPU:23  MEM:1842  Sess:14987  ...
```

핵심:
- **LAN Sessions** : 지표를 올려보내는 내부 서버 수. 채팅서버 1대만 운영 중이면 1.
- **NET Sessions** : 관제 툴 접속 수. 0 이어도 정상.
- **LAN Recv TPS** : 내부 서버 × 보내는 지표 항목 수 / 1초. 채팅 1대면 약 8 (DataType 30~37 + OS).
- **NET InvCode** : 외부에서 21510 을 두드리는 임의 트래픽이 있다는 뜻. 정상이라면 0.

---

## 3. 로그 위치

```
{exe_dir}/Log/{날짜}/
├── LanServer_{date}.txt    ← LanServer 모듈 로그 (구 .txt 와 _DEBUG.txt 통합)
├── ChatServer_{date}.txt
├── SessionMap_{date}.txt
└── ...
```

- 레벨: DEBUG / INFO / WARN / ERROR / FATAL
- `LOG_LEVEL = DEBUG` 로 두면 위 모든 레벨이 한 파일에 들어갑니다.
- `_bSplitByLevel = false` 로 운영 (DECISIONS.md ADR-002): 같은 로그가 두 파일에 중복 기록되는 문제를 막기 위함.

운영 권장 설정:
- 안정 운영기 → `LOG_LEVEL = INFO`
- 장애 디버깅기 → `LOG_LEVEL = DEBUG` (단, 디스크 폭증 주의)

---

## 4. 자주 보는 트러블슈팅

### 4.1 `Session = maxSession` 도달 후 신규 접속 안 됨
- 원인: 정상 부하면 `MAX_SESSION` 늘림. 비정상이면 어딘가 누수.
- 확인 순서:
  1. `HeartbeatTimeout` 이 안 늘고 있으면 → 클라들이 정상이지만 접속을 안 끊음 → 정상 만석
  2. `_sessionCount` 만 늘고 `Update TPS` 가 같이 안 늘면 → "좀비 세션" 의심 (소켓은 살아있는데 데이터 X)
  3. `cSessionMap::FreeSession` 호출 추적: free 가 안 도는지, AcceptThread 에서 계속 +1 되는지

### 4.2 Memory 가 시간당 증가
- 1순위: **PacketPool**. `MAX_PACKET_POOL` 이 무제한이거나 너무 큼. 공격 트래픽으로 풀이 폭증한 사례가 가장 흔함.
- 2순위: **로그 파일** (디스크 사용량과 혼동 주의). RSS 가 아니라 디스크가 늘고 있는 거면 `_bSplitByLevel` 확인.
- 3순위: 도메인 자료구조 (캐릭터/섹터). `CharPool` 사용량이 평탄하면 OK.

### 4.3 SendBufFull 이 분당 수십 이상 증가
- 일부 클라가 receive 를 안 함 (네트워크 지연, 클라 hang, 일부러 stall).
- 정상적인 송신 속도라면 `SEND_BUFSIZE = 512` 가 부족하지는 않음 — 문제는 클라 쪽.
- 심각하면 클라 IP 패턴 추적 (로그의 `_IP` 필드).

### 4.4 InvCode / InvLen / DecodeFail 폭증
- NET 21510 / 21501 에 임의 TCP 트래픽이 들어옴 = 포트 스캐너 또는 본격 공격
- 대응:
  - 방화벽에서 외부 IP 컷
  - `MAX_PACKET_POOL` 상한 확인 (없으면 풀 폭증)
  - 카운터만 보고 자동 알람 — 로그는 안 찍히게 설계됨 (DECISIONS.md ADR-002)

### 4.5 크래시 덤프에 `_IOCount = 0x80000000`
- 세션 초기화 race. ARCHITECTURE.md §3.3.
- `Session.cpp::Inintialize()` 에 `_IOCount = 1;` 가 들어 있는지 먼저 확인.

### 4.6 크래시 덤프가 `__debugbreak` 에서 멈춤
- 어떤 `__debugbreak()` 인지 콜스택으로 식별.
- 클라이언트 입력으로 도달 가능한 모든 `__debugbreak` 는 이미 Disconnect 로 변환됨 (DECISIONS.md ADR-001). 새로 발견되면 같은 패턴으로 변환.

---

## 5. 종료 절차

콘솔 포커스 → **`q`** 입력. 다음이 순차적으로 실행됩니다:

1. listen 소켓 close (신규 접속 차단)
2. 모든 세션 Disconnect
3. 5초 내 세션 0 되길 대기
4. 워커 / 모니터 / 타이머 스레드 join
5. 리소스 해제

> 5초 안에 끝나지 않으면 강제 진행합니다. 정상 종료되었는지 확인은 마지막 로그 라인 (Stop 완료) 으로.

---

## 6. 알람 설정 권장 (외부 모니터링 시스템에서)

| 메트릭 | 임계 | 액션 |
|---|---|---|
| Session > maxSession × 0.9 | 즉시 | 용량 검토 |
| CPU > 70% 5분 지속 | warn | 워커 스레드 / 락 점검 |
| Memory 시간당 +500MB | crit | 풀 사이즈 확인, 덤프 |
| InvCode/InvLen/DecodeFail 분당 +1000 | warn | 공격 의심, IP 로그 |
| HeartbeatTimeout 분당 +500 | warn | 네트워크 문제 |
| SendBufFull 분당 +10 | warn | 클라 수신 저하 |
| Update TPS 가 0 | crit | 워커 스턱 — 즉시 덤프 |

---

## 7. 운영 중 절대 하지 말 것

- ❌ `_characterLock` 잡은 상태에서 `Disconnect()` 호출 (데드락)
- ❌ `__debugbreak()` 에 도달했을 때 그냥 무시하고 코드만 추가 (왜 도달했는지부터)
- ❌ 운영 중 `LOG_LEVEL = DEBUG` 로 두고 방치 (디스크 폭증)
- ❌ `MAX_PACKET_POOL = 0` (무제한) 으로 운영 (공격 시 풀 폭증)
- ❌ AcceptThread 에서 무거운 로직 (락, DB 등) — accept 큐가 바로 막힘
