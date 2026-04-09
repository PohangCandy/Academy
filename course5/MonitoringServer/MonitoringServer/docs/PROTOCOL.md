# PROTOCOL

세 가지 헤더 포맷이 등장합니다. 어떤 포트는 어떤 포맷을 쓰는지부터 외워두세요.

| 포트 | 헤더 포맷 | 누가 누구한테 |
|---|---|---|
| **21501** (ChattingServer) | 5바이트 암호화 (`PacketHeader`) | 게임 클라 → 채팅 서버 |
| **20000** (Monitoring LAN) | 2바이트 단순 (`LanPacketHeader`) | 내부 서버(채팅서버) → 모니터링 서버 |
| **21510** (Monitoring NET) | 5바이트 암호화 (`PacketHeader`) | 모니터링 서버 → 관제 툴 |

> "암호화" 라고 하지만 비공개 키 암호가 아니라 **PACKET_CODE / PACKET_KEY 와 RandKey, CheckSum 을 결합한 단순 obfuscation** 입니다. 변조/리플레이를 막기 위한 게 아니라, 헤더 포맷을 모르는 임의의 TCP 트래픽을 즉시 거르기 위한 1차 방어선입니다.

---

## 1. LAN 헤더 (2 바이트)

```
┌─────────────┐
│  Len (WORD) │  payload 길이 (헤더 제외)
└─────────────┘
```

```cpp
#define dfLAN_HEADERSIZE  (2)
struct LanPacketHeader { unsigned short Len; };  // pack(1)
```

- 사용처: ChattingServer → MonitoringServer LAN(20000), MonitoringServer 의 LAN 수신측
- 검증: `Len == 0 || Len > MAX` 이면 즉시 Disconnect + `_invalidPacketLenCount` 누적

---

## 2. NET 헤더 (5 바이트, "암호화")

```
┌──────┬──────────┬─────────┬─────────┐
│ Code │   Len    │ RandKey │CheckSum │
│ (1B) │  (2B)    │  (1B)   │  (1B)   │
└──────┴──────────┴─────────┴─────────┘
```

```cpp
#define dfPACKET_HEADERSIZE  (5)
struct PacketHeader {
    unsigned char  Code;       // 고정값. 채팅: 0x77, 모니터NET: 109(0x6D)
    unsigned short Len;        // payload 길이
    unsigned char  RandKey;    // 송신측이 랜덤 생성
    unsigned char  CheckSum;   // payload 의 단순 합 (Encode 단계에서 계산)
};
```

| 키 | ChattingServer (21501) | MonitoringServer NET (21510) |
|---|---|---|
| `PACKET_CODE` | `0x77` | `109` (0x6D) |
| `PACKET_KEY`  | `0x32` | `30`  (0x1E) |

### 2.1 송신 인코딩 (`CPacket::EncodeForNet`)

1. payload 의 1바이트 합으로 `CheckSum` 계산 → 헤더에 기록
2. `RandKey` 랜덤 생성 → 헤더에 기록
3. payload 바이트들을 `RandKey`/`PACKET_KEY` 기반 누적 XOR 로 변환

### 2.2 수신 디코딩 (`CPacket::DecodeForNet`)

1. `header->Code != PACKET_CODE` → **즉시 Disconnect** + `_invalidPacketCodeCount`
2. `header->Len == 0 || > 500` → **즉시 Disconnect** + `_invalidPacketLenCount`
3. payload 역 XOR → CheckSum 재계산
4. CheckSum 불일치 → **Disconnect** + `_decodeForNetFailCount`

세 카운터는 `LEVEL_ERROR` 로그가 아니라 누적 카운터로만 표시됩니다 (DECISIONS.md ADR-002 참고).

---

## 3. CS 패킷 (게임 클라 ↔ ChattingServer)

`CommonProtocol.h::en_PACKET_TYPE` 의 패킷들. 모든 패킷은 위 5바이트 NET 헤더 다음에 **첫 WORD 가 Type** 으로 시작합니다.

### 3.1 en_PACKET_CS_CHAT_REQ_LOGIN
```
WORD  Type
INT64 AccountNo
WCHAR ID[20]            // null 포함 가능
WCHAR Nickname[20]
char  SessionKey[64]    // 인증 토큰 (현재 검증 생략)
```
서버 처리: `Handle_CS_CHAT_REQ_LOGIN` (Exclusive lock)
- `_umapAccountSession` 으로 중복 체크 → **kick old, accept new**
- 캐릭터 생성 → `_umapSessionCharacter` 등록
- 응답: `en_PACKET_SC_CHAT_RES_LOGIN { BYTE Status, INT64 AccountNo }`

### 3.2 en_PACKET_CS_CHAT_REQ_SECTOR_MOVE
```
WORD  Type
INT64 AccountNo
WORD  SectorX            // 0..49
WORD  SectorY            // 0..49
```
- 캐릭터를 sector grid 의 새 위치로 이동
- 응답: `en_PACKET_SC_CHAT_RES_SECTOR_MOVE` (브로드캐스트는 새/구 섹터 인접 9칸)

### 3.3 en_PACKET_CS_CHAT_REQ_MESSAGE
```
WORD  Type
INT64 AccountNo
WORD  MessageLen
WCHAR Message[MessageLen / 2]
```
- 자기 섹터 + 인접 8칸의 모든 캐릭터에게 `en_PACKET_SC_CHAT_RES_MESSAGE` 브로드캐스트

### 3.4 en_PACKET_CS_CHAT_REQ_HEARTBEAT
```
WORD  Type
```
- 클라가 30초 간격으로 송신
- 서버는 마지막 수신 시각만 갱신. UpdateThread 가 40초 이상 끊긴 세션을 Disconnect.

### 3.5 SS 내부 (en_PACKET_SS_*)
- `en_PACKET_SS_Create_Character (10000)` / `en_PACKET_SS_Session_Release (10001)`
- 채팅 서버 내부의 워커 → UpdateThread 큐잉용. 네트워크에 노출되지 않습니다.

---

## 4. SS 모니터 패킷 (ChattingServer → MonitoringServer LAN)

`MonitorProtocol.h::en_PACKET_TYPE`. 베이스 = 20000.

### 4.1 en_PACKET_SS_MONITOR_LOGIN (20001)
```
WORD Type
int  ServerNo            // ServerConfig.ini [Monitor] SERVER_NO
```
- 채팅 서버가 LAN 접속 직후 1회 전송
- MonitoringServer 측에서 ServerNo ↔ 세션 매핑 등록

### 4.2 en_PACKET_SS_MONITOR_DATA_UPDATE (20002)
```
WORD Type
BYTE DataType            // 1..44 (아래 표)
int  DataValue
int  TimeStamp           // time(NULL)
```
- 채팅 서버가 1초마다 자신의 모든 지표를 N개 송신 (각 DataType 당 1패킷)
- MonitoringServer 는 `g_iMonitorData[ServerNo][DataType]` 에 최신값 갱신

#### DataType 표
| 그룹 | 값 | 의미 |
|---|---|---|
| Login | 1 | LoginServer 동작 ON/OFF |
|       | 2 | LoginServer CPU % |
|       | 3 | LoginServer 메모리 MB |
|       | 4 | LoginServer 세션 수 |
|       | 5 | LoginServer 인증 TPS |
|       | 6 | LoginServer 패킷풀 |
| Game  | 10 | GameServer ON/OFF |
|       | 11 | GameServer CPU % |
|       | 12 | GameServer 메모리 MB |
|       | 13 | GameServer 세션 수 |
|       | 14 | Auth mode 플레이어 수 |
|       | 15 | Game mode 플레이어 수 |
|       | 16 | GameServer Accept TPS |
|       | 17 | GameServer Recv TPS |
|       | 18 | GameServer Send TPS |
|       | 19 | GameServer DB Write TPS |
|       | 20 | GameServer DB Write 큐 |
|       | 21 | Auth Thread FPS |
|       | 22 | Game Thread FPS |
|       | 23 | GameServer 패킷풀 |
| Chat  | 30 | ChatServer ON/OFF |
|       | 31 | ChatServer CPU % |
|       | 32 | ChatServer 메모리 MB |
|       | 33 | ChatServer 세션 수 |
|       | 34 | 인증성공 플레이어 수 |
|       | 35 | UPDATE TPS |
|       | 36 | ChatServer 패킷풀 |
|       | 37 | UPDATE MSG 풀 |
| OS    | 40 | 서버 PC CPU 총사용률 |
|       | 41 | NonPaged 메모리 MB |
|       | 42 | 네트워크 RECV KB/s |
|       | 43 | 네트워크 SEND KB/s |
|       | 44 | 사용가능 메모리 |

> 41(NonPaged), 44(Available) 는 메모리 leak 추적용. 정상 부하에서는 평탄해야 합니다.

---

## 5. CS 모니터 패킷 (관제 툴 ↔ MonitoringServer NET)

베이스 = 25000. 관제 툴이 NET 21510 에 5바이트 암호화 헤더로 접속합니다.

### 5.1 en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN (25001)
```
WORD Type
char LoginSessionKey[32]   // 인증 토큰
```

### 5.2 en_PACKET_CS_MONITOR_TOOL_RES_LOGIN (25002)
```
WORD Type
BYTE Status
```
| Status | 의미 |
|---|---|
| 1 | OK |
| 2 | 서버이름 없음 |
| 3 | 세션키 오류 |

### 5.3 en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE (25003)
```
WORD Type
BYTE ServerNo
BYTE DataType
int  DataValue
int  TimeStamp
```
- 서버가 관제 툴에 푸시. 1초마다 전체 지표 broadcast (현재 모든 인증된 NET 세션에 동일하게).

---

## 6. 패킷 검증 흐름 (요약)

```
recv 도착
   │
   ▼
헤더 5/2 바이트 누적?  ── No ──▶ 더 받음
   │ Yes
   ▼
Code 일치? ── No ──▶ Disconnect, +invalidCode
   │ Yes
   ▼
0 < Len ≤ MAX? ── No ──▶ Disconnect, +invalidLen
   │ Yes
   ▼
헤더+payload 누적? ── No ──▶ 더 받음
   │ Yes
   ▼
Dequeue payload
   │
   ▼
DecodeForNet (NET 한정) ── 실패 ──▶ Disconnect, +decodeFail
   │ 성공
   ▼
OnRecv(CPacket*)
```

이 4단계 검증이 통과되지 않은 패킷은 **OnRecv 에 절대로 도달하지 않습니다**. 도메인 핸들러는 자기 패킷의 payload 길이만 검사하면 됩니다 (Type/AccountNo 등).

---

## 7. 새 패킷 추가 절차

1. `CommonProtocol.h` 또는 `MonitorProtocol.h` 의 enum 끝에 추가 (값 충돌 주의)
2. 송신측: `CPacket::Alloc` → `*p << Type << ...` → `SendPacket(key, p)` → `p->SubRef()`
3. 수신측: `OnRecv` 의 `switch(Type)` 분기 추가
4. payload 의 최소/최대 길이를 핸들러 진입 즉시 검증하고, 부족하면 Disconnect
5. **로그는 LEVEL_ERROR 가 아니라 카운터로 누적** (공격 트래픽 폭증 방지)
