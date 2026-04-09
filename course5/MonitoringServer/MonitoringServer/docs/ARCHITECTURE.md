# ARCHITECTURE

이 문서는 **코드만 봐서는 보이지 않는 동작 원리**를 설명합니다.
"왜 IOCount 가 1로 시작하는가", "왜 Disconnect 후에도 즉시 free 하지 않는가" 같은 질문의 답이 여기에 있습니다.

---

## 1. 컴포넌트 구성 (한 그림)

```
┌─────────────────────── ChattingServer (process) ───────────────────────┐
│                                                                         │
│   ┌─────────────────────────────────────────────────────────────────┐  │
│   │                       CLanServer (LAN, 21501)                  │  │
│   │   AcceptThread ─┐                                              │  │
│   │                 ▼                                              │  │
│   │   listen sock ─▶ accept ─▶ AllocSession ─▶ IOCP CP ─▶ WSARecv  │  │
│   │                                                       │       │  │
│   │   WorkerThread×N ◀─── GQCS ◀──────────────────────────┘       │  │
│   │   ├ ERecv  → 헤더 검증 → CPacket 생성 → OnRecv()              │  │
│   │   ├ ESend  → SubRef → 다음 SendPost                            │  │
│   │   └ exit   → Disconnect / Release                              │  │
│   │   MonitorThread → TPS 갱신                                     │  │
│   └─────────────────────────────────────────────────────────────────┘  │
│                                  │                                      │
│                                  ▼ OnRecv (CPacket*)                    │
│   ┌─────────────────────────────────────────────────────────────────┐  │
│   │                  ChattingServer (도메인 레이어)                  │  │
│   │   Handle_CS_CHAT_REQ_LOGIN / SECTOR_MOVE / MESSAGE / HEARTBEAT  │  │
│   │   _umapSessionCharacter   _umapAccountSession   _sectorList[][] │  │
│   │   (SRWLock _characterLock 로 보호)                              │  │
│   └─────────────────────────────────────────────────────────────────┘  │
│                                  │                                      │
│                                  ▼ 자신의 지표를 송신                    │
│   ┌─────────────────────────────────────────────────────────────────┐  │
│   │   CMonitorClient ─── connect ───▶ MonitoringServer:20000 (LAN) │  │
│   └─────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘

┌──────────────────── MonitoringServer (process) ────────────────────────┐
│  ┌────────────────────────────┐   ┌─────────────────────────────────┐  │
│  │ CLanServer (port 20000)    │   │ CNetServer (port 21510)         │  │
│  │ - 내부 서버 수신            │   │ - 관제 툴 송신                   │  │
│  │ - 2바이트 헤더(Len만)       │   │ - 5바이트 암호화 헤더            │  │
│  │ - SS_MONITOR_LOGIN/UPDATE   │   │ - CS_MONITOR_TOOL_REQ_LOGIN/... │  │
│  └────────────┬───────────────┘   └────────────▲────────────────────┘  │
│               │ OnRecv                            │ SendPacket          │
│               ▼                                   │                     │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │            CMonitoringServer (도메인 레이어)                      │   │
│  │  - 서버별 최신 지표 보관 (g_iMonitorData[44])                     │   │
│  │  - SystemMonitor: 자기 OS 지표(CPU/메모리/이더넷) 수집             │   │
│  │  - DisplayThread: 콘솔에 통계 표시                                │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

> 핵심: **MonitoringServer 한 프로세스가 IOCP 서버 두 개(CLanServer + CNetServer)를 동시에 운영**합니다. 하나는 내부 서버용, 하나는 외부 관제툴용. 헤더 포맷이 다르기 때문에 클래스를 분리했습니다.

---

## 2. 스레드 모델

| 스레드 | 개수 | 역할 | 종료 조건 |
|---|---|---|---|
| **AcceptThread** | 1 | `accept()` 블로킹 → SOCKETINFO 할당 → IOCP 등록 → 첫 WSARecv | listen 소켓 close |
| **WorkerThread** | `(논리코어/2) × 2` | `GetQueuedCompletionStatus`로 IO 완료 처리 | `PQCS(0,0,NULL)` 종료 신호 |
| **MonitorThread** | 1 | 1초마다 acceptCount/recvMsgCount/sendMsgCount → TPS 환산 | `_isRunning = false` |
| **TimerThread** (도메인) | 1 | 1초마다 콘솔 출력 + Monitor 패킷 송신 | 메인 종료 신호 |
| **UpdateThread** (도메인) | 1 | (채팅서버) 하트비트 타임아웃 검사 | 메인 종료 신호 |

**워커 스레드 수** 는 `(논리코어 / 2) × 2 = 논리코어 수` 입니다. IOCP concurrency limit 은 `논리코어/2`로 두어, IO 대기가 풀릴 때만 추가 스레드가 깨어나도록 합니다.

---

## 3. IOCP 라이프사이클 (가장 중요)

세션 객체(`SOCKETINFO`)는 **여러 스레드가 동시에 같은 객체를 만질 수 있으면서도**, **누군가가 아직 쓰고 있으면 free 되어선 안 됩니다**. 이를 위해 두 가지 메커니즘을 결합합니다.

### 3.1 IOCount + RELEASE_FLAG

`SOCKETINFO::_IOCount` (32비트)
- bit 0..30 : **참조 카운트**
- bit 31    : **RELEASE_FLAG** (해제 진행 중 표식)

```
 31         30 .. 0
┌──┬──────────────────┐
│R │   IOCount (refs) │
└──┴──────────────────┘
```

**증가** (`IncreaseSessionIO`)
- RELEASE_FLAG 가 켜져 있으면 **실패** → 호출자는 그 세션을 더 이상 만지면 안 됨.
- 그렇지 않으면 CAS 로 +1.

**감소** (`DecreaseSessionIO`)
- CAS 로 -1.
- 결과가 0 이면 한 번 더 CAS 로 RELEASE_FLAG 를 켠다 (성공한 스레드만 `FreeSession` 호출).
- 두 단계 CAS 를 쓰는 이유: -1 한 직후 다른 스레드가 +1 하는 경합을 차단하기 위해. 0→플래그 전이가 원자적이어야 free 권한이 한 명에게만 간다.

### 3.2 SessionKey (재활용 ABA 방지)

세션 배열은 인덱스 재활용이 일어나기 때문에, 인덱스만으로 세션을 식별하면 ABA 문제가 생깁니다. 그래서 64비트 `SessionKey = (index << 32) | uniqueId` 를 씁니다.

```cpp
GetSessionptr(key) {
    ptr = &_sessionArray[key.GetIndex()];
    if (!IncreaseSessionIO(ptr)) return nullptr;       // 해제 중
    if (ptr->_sessionKey.GetSessionId() != key.id) {   // 재활용 발생
        InterlockedDecrement(&ptr->_IOCount);           // 증가분 되돌림
        return nullptr;                                  // ★ Release 로직 절대 금지
    }
    return ptr;
}
```

> **함정**: 재활용이 감지된 경우 `DecreaseSessionIO`를 호출하면 안 됩니다. 그건 IOCount==0 시 free 권한을 행사하기 때문에, 다른 누군가가 정상적으로 쓰고 있는 세션을 free 시킬 수 있습니다. 단순 `InterlockedDecrement`로 증가분만 되돌립니다.

### 3.3 AcceptThread 의 초기 IOCount = 1

이전엔 `_IOCount = 0`으로 초기화한 뒤 AcceptThread 에서 `IncreaseSessionIO`로 +1 했었습니다. 이게 **15,000 동접에서 크래시**를 만들었습니다.

문제 시퀀스:
```
[AcceptThread]                          [WorkerThread A]
ptr = AllocSession()                                                 // _IOCount = 0
                                          (어떻게든 같은 세션 ptr 노출됨)
                                          IncreaseSessionIO → IOCount=1
                                          OnRecv → SendPacket → Release path
                                          DecreaseSessionIO → IOCount=0
                                          → CAS RELEASE_FLAG | 0 (= 0x80000000) ★
                                          → FreeSession() 실행
IncreaseSessionIO  ← 실패! (RELEASE 비트 ON)
WSARecv 안 함, 그러나 ptr 은 이미 다른 누군가에 재할당 가능
```

→ 세션이 초기화 도중 free 됨. `_sendPacketNum=1, _IOCount=0x80000000` 덤프가 이 흔적입니다.

**해결**: `Inintialize()`에서 `_IOCount = 1`로 두고 (AcceptThread 소유권), 모든 초기화(IOCP 등록, WSARecv 발사)가 끝난 직후 AcceptThread 가 본인 몫의 -1 을 합니다.

```cpp
// Session.cpp
_IOCount = 1;	// AcceptThread 소유권 (초기화 완료까지 세션 해제 방지)
```

```cpp
// CLanServer.cpp AcceptThread 끝부분
{   // AcceptThread 소유권 반환 (초기 IOCount=1 분)
    if (DecreaseSessionIO(ptr) == ReleaseResult::Released) { ... }
}
```

이제 워커가 먼저 -1 해도 IOCount 는 최소 1을 유지하므로 free 가 되지 않습니다.

### 3.4 IOCount 증감 규칙 요약

| 시점 | 증감 |
|---|---|
| `Inintialize()` | **+1** (Accept 소유권) |
| `WSARecv` 발사 (Accept 또는 WsaRecvSession) | **+1** |
| `WSASend` 발사 (`SendPost`) | **+1** |
| `GQCS` 가 ERecv/ESend 완료 회수 | **-1** |
| `Disconnect()` 호출 | **-1** (호출자가 GetSessionptr 로 +1 한 분을 회수) |
| `SendPacket()` 종료 | **-1** (위와 동일) |
| AcceptThread 끝 | **-1** (Accept 소유권 반환) |

규칙: **IO 작업을 발사할 때 +1, 완료(또는 실패)를 회수할 때 -1**. 헷갈리면 이걸 떠올리세요.

---

## 4. 송신 경로 (한 번에 하나만)

여러 스레드가 동시에 `WSASend` 를 걸면 안 됩니다. `_IsSending` 플래그로 직렬화합니다.

```cpp
CanSend(ptr) {
    return InterlockedCompareExchange(&_IsSending, 1, 0) == 0;
}
```

- `SendPacket`: enqueue → CanSend 성공 시 `SendPost` 호출. 실패하면 다른 스레드가 이미 보내는 중이므로 그냥 큐에 두고 빠짐.
- `ESend` 완료 핸들러: 보낸 패킷 SubRef → `_IsSending = 0` → 큐에 잔여가 있으면 다시 SendPost.

`_sendPacketNum` 은 "이번 WSASend 한 묶음에 몇 개가 들어갔는가" 를 기록해, 완료 시 정확히 그 개수만 SubRef 합니다.

`SendBuf` 가 가득 차면 (Enqueue 실패) → `_sendBufferFullCount` 누적 + Disconnect. 이는 클라가 받지 못해 송신이 적체된 상황으로, 강제 끊기가 정상 동작입니다.

---

## 5. 패킷 풀 (CPacket refcount)

`CPacket::Alloc()` 으로 할당된 패킷은 **항상 refcount 1** 로 시작합니다. 송신 경로에서:

```cpp
CPacket* p = CPacket::Alloc();   // ref=1
p->PutData(...);
SendPacket(key, p);              // 내부에서 AddRef → ref=2
p->SubRef();                     // ref=1 (SendBuf 가 마지막 1을 소유)
// ESend 완료 시 SubRef → ref=0 → 풀로 반환
```

**Pool capacity 제한** (`MAX_PACKET_POOL`): 공격 트래픽으로 풀이 무한 팽창하는 것을 막기 위함. 0 이면 무제한.

CharacterPool 도 동일 사상으로 `MAX_CHAR_POOL` 로 제한합니다.

---

## 6. RecvBuffer / SendBuffer

세션당 두 개의 링버퍼:

```
RecvBuf : 4 KB  (CRingBuffer, 바이트 큐)
SendBuf : 512   (CPacketRingBuffer, CPacket* 큐)
```

**Recv 흐름**
1. WSARecv 가 GetRearBufferPtr / DirectEnqueueSize 영역에 받음
2. 완료 시 MoveRear(받은바이트). MoveRear 가 0 을 리턴하면 **버퍼 오버플로**(클라가 처리속도보다 빠르게 보냄) → Disconnect.
3. 헤더 사이즈 이상 누적되면 Peek → 검증 → MoveFront(헤더) → Dequeue(payload) → OnRecv.

**SendBuf** 는 패킷 포인터 큐. 가득 차면 위 4절대로 Disconnect.

**왜 RecvBuf 가 4 KB 인가**: 채팅 패킷 최대 페이로드 500 B + 헤더 5 B → 한 번에 한 패킷이 충분히 들어가는 가장 작은 사이즈. 메모리 절약이 목적 (15,000 세션 × 4KB = 60 MB).

---

## 7. 락 전략 (도메인 레이어)

`_characterLock` 은 단일 SRWLock 입니다. 동시성보다 단순함을 우선했습니다.

- **Shared (읽기)**: SECTOR_MOVE 의 sector 조회, MESSAGE 브로드캐스트의 캐릭터 순회
- **Exclusive (쓰기)**: LOGIN, 캐릭터 삭제, **중복 로그인 처리**

> **Handle_CS_CHAT_REQ_LOGIN 은 Exclusive 가 필수**입니다. `_umapAccountSession` 검사+삽입이 두 단계로 일어나기 때문에 Shared 였다면 두 워커가 같은 accountNo 를 동시에 삽입하는 race 가 가능합니다.

**Disconnect 호출은 락 밖에서**: 락을 잡은 채로 Disconnect 를 호출하면, Disconnect 가 워커의 이벤트(GQCS)를 거쳐 다시 OnClientLeave → DeleteCharacter → `_characterLock` 으로 들어와 데드락이 됩니다. 그래서 중복 로그인 처리에서는

```cpp
ReleaseSRWLockExclusive(&_characterLock);
if (oldSessionKey.GetSessionId() != 0)
    Disconnect(oldSessionKey);
```

순서로 락을 풀고 호출합니다.

---

## 8. 메모리 풀

| 풀 | 어디서 | 용도 |
|---|---|---|
| **CPacket pool** | `MemoryPoolForLockFree<CPacket>` | recv/send 패킷 객체 재사용 |
| **CRingBuffer chunk** | `CPacketRingBuffer` 내부 | refcount 패킷의 연속 chunk |
| **Character pool** | (채팅서버) 캐릭터 객체 | 로그인 시 alloc, 로그아웃 시 free |
| **세션 배열** | `cSessionMap::_sessionArray` | 정적 배열, 인덱스 재활용 |

세션 배열은 처음부터 `maxSession` 만큼 통째로 할당해 두고, free 된 인덱스는 `_deletedSessionIndex` 스택에 LIFO 로 쌓아 재사용합니다. 새 인덱스는 무한 증가하는 게 아니라 반드시 이 스택을 먼저 비운 뒤 `_nextIndex` 를 씁니다 — 캐시 친화적입니다.

---

## 9. 클라이언트 종료 처리 (정상 vs 비정상)

| 상황 | 감지 | 로그 정책 |
|---|---|---|
| 클라가 정상 close | GQCS 가 cbTransferred==0 으로 리턴 | **로그 안 찍음** (재접속 더미 폭증 방지) |
| 클라가 RST (10054) | WSARecv 가 WSAECONNRESET 으로 실패 | **로그 안 찍음** (필터링) |
| 라우팅 끊김 (64, 1236) | 같음 | **로그 안 찍음** |
| 그 외 WSA 에러 | 같음 | LEVEL_ERROR 로 한 줄 |
| 공격 패킷 | header 검증 실패 | **로그 대신 카운터** (`_invalidPacketCodeCount` 등) |

이 정책 변경으로 시간당 로그가 10 GB → 수백 MB 로 줄었습니다 (DECISIONS.md ADR-002 참고).

---

## 10. Stop() 시퀀스

순서가 중요합니다.

1. **listen 소켓 close** → AcceptThread 의 `accept()` 가 INVALID 리턴 → 스레드 종료
2. AcceptThread 종료 대기 (이후로 신규 세션 없음)
3. 모든 활성 세션에 `Disconnect()` 호출
4. `_sessionCount == 0` 까지 대기 (최대 5초)
5. 워커 스레드 수만큼 `PQCS(0,0,NULL)` → WaitForMultipleObjects
6. MonitorThread 에 `_isRunning=false` → join
7. IOCP / SessionMap 해제

> 4단계에서 5초 안에 정리되지 않으면 그냥 진행합니다 (강제 종료). 이는 hang 방지를 위함이며, 실무라면 더 길게 두거나 강제 abort 로 전환합니다.

---

## 11. 에러가 났을 때 가장 먼저 보는 곳

| 증상 | 의심 |
|---|---|
| `_IOCount = 0x80000000` 덤프 | 초기화 race — Session.cpp 의 `_IOCount = 1` 누락 |
| `_sendPacketNum != 0` 인데 죽음 | ESend 완료 처리 도중 락 순서 문제 |
| `Dequeue` 에서 `__debugbreak` | 헤더 검증 통과한 0/음수 Len 가능성, CommonProtocol.h 의 검증식 확인 |
| `RecvBuf overflow` 로그 폭증 | 클라 송신 속도 > 서버 처리 속도 — 워커 수 / 큐 동시성 점검 |
| `SendBufFull` 누적 증가 | 특정 세션이 receive 안 함 — 클라가 hung 또는 일부러 stall |
