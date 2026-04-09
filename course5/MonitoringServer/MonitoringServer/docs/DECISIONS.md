# DECISIONS (ADR)

이 문서는 **왜 이렇게 만들었는가** 만 다룹니다. *어떻게* 동작하는지는 ARCHITECTURE.md, *무엇을* 보내는지는 PROTOCOL.md 입니다.

각 항목 형식:

> **Status**: Accepted / Superseded
> **Context**: 무엇이 문제였나
> **Decision**: 무엇을 결정했나
> **Consequences**: 그래서 무엇이 좋아지고 나빠졌나
> **Alternatives considered**: 무엇을 고민하다 버렸나

---

## ADR-001 — 클라이언트 입력으로 도달 가능한 `__debugbreak()` 제거

**Status**: Accepted (2026-04)

**Context**
- 채팅서버가 더미 트래픽 도중 두 번 다운됨.
  - 1차: 공격 클라가 `header->Len == 0` 을 보내 `Dequeue(buf, 0)` → `CRingBuffer` 의 `__debugbreak`
  - 2차: 클라 송신 속도 > 서버 처리 속도로 RecvBuf 가 가득 → `MoveRear` 가 0 리턴 → `__debugbreak`
- 코드 전체에서 `__debugbreak` 를 11곳 발견. 그 중 클라 입력으로 도달 가능한 것 식별 필요.

**Decision**
- 클라 입력으로 도달 가능한 `__debugbreak` 는 모두 **Disconnect + 카운터 누적**으로 변환.
- 진짜 "있을 수 없는 상태" (자기 자신 코드의 invariant 위반) 만 `__debugbreak` 로 남김. 이건 "고쳐야 할 버그" 의 표식.
- 새 코드 작성 시에도 같은 규칙 적용: **외부 입력 = Disconnect, 내부 invariant = debugbreak**.

**Consequences**
- (+) 서버가 공격 트래픽으로 죽지 않음.
- (+) `__debugbreak` 가 남아 있다는 건 진짜 버그 신호 — 의미가 명확해짐.
- (-) 누군가 실수로 외부 입력 검증을 빼먹어도 즉시 알아채기 어려움. → 카운터 폭증으로 간접 감지.

**Alternatives considered**
- 모든 `__debugbreak` 를 삭제 → 진짜 버그 잡기 어려워짐. 기각.
- assert 매크로로 통일 (Release 빌드에서 disable) → Release 에서 조용히 잘못된 상태로 진행 = 더 위험. 기각.

---

## ADR-002 — 로그 다이어트 (10 GB/h → 수백 MB/h)

**Status**: Accepted (2026-04)

**Context**
- 더미 트래픽 운영 중 시간당 10 GB 로그 발생.
- 디스크 가득참, 운영자가 로그 검색 불가능.
- 원인 4가지:
  1. `_bSplitByLevel = true` 로 같은 로그가 통합 파일과 레벨별 파일 양쪽에 기록됨 (2배)
  2. 정상 클라 종료 (`GQCS` cbTransferred==0) 마다 DEBUG 로그
  3. 정상 클라 RST (WSAECONNRESET) 마다 ERROR 로그
  4. 공격 패킷 (Code/Len/Decode 실패) 마다 ERROR 로그 — 공격 시 초당 수천

**Decision**
1. `_bSplitByLevel = false`. 한 모듈 한 파일.
2. GQCS cbTransferred==0 분기에서 로그 제거.
3. WsaRecvSession 에서 정상 종료성 에러코드 (`10054, 10053, 64, 1236`) 필터링.
4. 공격 패킷 검증 실패는 **로그 대신 카운터** (`_invalidPacketCodeCount` 등) 누적, 콘솔에 표시.

**Consequences**
- (+) 시간당 로그 10 GB → 수백 MB.
- (+) 운영자가 로그를 grep 으로 실제로 읽을 수 있게 됨.
- (+) 공격 트래픽이 와도 로그/디스크 cost 가 O(1).
- (-) "어떤 클라가 끊겼는가" 의 정확한 시점이 로그에 안 남음. 필요시 디버깅 빌드에서 일시 활성화.
- (-) 카운터는 누적값이라 "1분 전 10초 사이에 얼마나 들어왔는가" 같은 슬라이딩 윈도우 정보는 못 봄. 외부 모니터링 시스템이 폴링해서 차분으로 환산해야 함.

**Alternatives considered**
- 로그 레벨을 INFO 로 올림 → ERROR 분류된 로그가 제거되지 않음. 부분적 해결만. 기각.
- 로그 레이트 리미팅 (초당 N개로 캡) → 구현 비용 큼. 카운터로 충분. 보류.
- ELK / Fluentd 같은 로그 파이프라인 도입 → 학습 프로젝트 범위 초과. 보류.

---

## ADR-003 — IOCount 초기값을 1로 (AcceptThread 소유권)

**Status**: Accepted (2026-04, supersedes 초기 IOCount=0)

**Context**
- 15K 동접 더미 환경에서 `_sendPacketNum=1, _IOCount=0x80000000` 덤프로 크래시 발생.
- 분석 결과: AcceptThread 가 `_IOCount = 0` 인 세션을 IOCP 에 등록한 직후, 워커가 같은 ptr 을 발견(이론상 가능한 race) → 워커가 -1 한 결과 0 → RELEASE_FLAG 자동 setting → 워커가 FreeSession → 그 사이 AcceptThread 의 `IncreaseSessionIO` 는 RELEASE_FLAG 로 인해 실패 → 그러나 ptr 은 이미 다른 AcceptThread 라운드에서 재할당 가능.
- 다시 말해, **세션 객체가 "초기화 완료" 상태가 되기 전에 release 될 수 있는 창**이 있음.

**Decision**
- `Inintialize()` 에서 `_IOCount = 1` 로 시작 (AcceptThread 가 가상의 +1 소유).
- AcceptThread 는 IOCP 등록 + 첫 WSARecv 발사가 모두 끝난 직후, `DecreaseSessionIO` 를 명시적으로 호출하여 본인 몫의 -1 을 반환.
- 이후의 모든 IOCount 규칙은 그대로.

**Consequences**
- (+) 초기화 도중 release 가 원천 차단됨.
- (+) "Accept 가 release 를 트리거할 수 있다" 는 한 줄로 모델이 단순해짐 (다른 워커와 동등한 참여자).
- (-) `Inintialize` 후의 어떤 코드 경로에서도 -1 을 누락하면 세션이 영원히 살아 있음. → 코드 리뷰 시점에 +1/-1 짝맞춤 강제.

**Alternatives considered**
- AllocSessionptr 안에서 IncreaseSessionIO 를 lock 안에서 호출 → 락 holding 시간 증가, 정상 동작도 느려짐. 기각.
- IOCP 등록을 하지 않고 ptr 을 노출 안 함 → IOCP 등록 자체가 "노출" 이라 회피 불가. 기각.
- IsInitializing 플래그 추가 → 새 race 발생 가능, 더 복잡. 기각.

---

## ADR-004 — 중복 로그인: kick old, accept new (임시)

**Status**: Accepted as **temporary** measure (2026-04)

**Context**
- 더미 클라가 빈번히 재접속. 같은 AccountNo 가 동시에 두 세션을 가질 수 있음.
- 옵션:
  - **A) reject new, keep old**: "이미 로그인됨" 응답. 정상 클라가 네트워크 끊겨 재접속하면 영원히 못 들어옴.
  - **B) kick old, accept new**: 이전 세션을 끊고 새 세션 받기. 정상 재접속이 잘 됨. 단, 공격자가 임의 AccountNo 로 정당한 세션을 강제 종료시킬 수 있음.
  - **C) Login Server 분리**: 세션 토큰 발급 → 채팅서버는 토큰 검증만. 공격자가 토큰을 못 얻음. 가장 정석.

**Decision**
- 단기: **B (kick old, accept new)** 를 채택.
- 장기: **C (Login Server)** 를 도입할 때까지의 임시 조치임을 코드와 docs 에 명시.

**Why B 가 임시인지**
- 게임 로비/로그인 서버가 별도로 없는 현 구조에서, 정상 클라의 끊김/재접속을 막으면 동접 회복이 안 됨. 더미 테스트 환경에서 가장 큰 실용적 문제.
- 공격자가 AccountNo 를 알아내어 남의 세션을 끊는 위협은 로그인 서버가 추가되는 시점에 자동 해소.

**Consequences**
- (+) 정상 재접속 시나리오 안정.
- (+) `_umapAccountSession` 한 줄로 구현. 단순.
- (-) AccountNo 가 노출되면 임의 종료 공격 가능 (현재 인증 토큰 검증 X).
- (-) 동시 두 세션이 미세한 시점에 존재할 수 있음 (락 밖에서 Disconnect 호출 → 워커가 비동기로 처리). 도메인 로직은 SessionKey 매칭으로 이를 흡수.

**Alternatives considered**
- IP 레이트 리미팅으로 재접속 폭주만 방어 → 더미 테스트에선 IP 가 같으므로 정상 트래픽도 막힘. 기각.
- Redis 기반 일회용 토큰 → 의존성 추가. 학습 범위 초과. ADR-007 후보로 보류.

---

## ADR-005 — 도메인 락은 단일 SRWLock

**Status**: Accepted (2026-04)

**Context**
- 채팅서버 도메인 자료구조: `_umapSessionCharacter`, `_umapAccountSession`, `_sectorList[50][50]`.
- 옵션:
  - 자료구조마다 락 → 정합성 보장 위해 락 순서 규칙 필요, 데드락 위험
  - 단일 SRWLock → 동시성 손해 있음, 정합성 단순

**Decision**
- 단일 `_characterLock` (SRWLock) 사용.
- 읽기 다수 / 쓰기 소수 패턴: SECTOR_MOVE / MESSAGE 브로드캐스트 = Shared, LOGIN / LEAVE = Exclusive.
- **LOGIN 은 Exclusive 필수**: `_umapAccountSession` 검사+삽입이 두 단계라 Shared 로는 race 발생.

**Consequences**
- (+) 락 순서 고민 없음. 데드락 가능성 ↓.
- (+) Thread Activity 카운터로 락 holding 추적 단순.
- (-) 동접 한계는 락 경합으로 결정됨. 30K 이상에서는 락 분리 필요할 가능성.

**Alternatives considered**
- Lock-free hash map → 구현 비용 / 디버깅 비용 큼. 기각.
- Sector 별 락 → 캐릭터가 sector 를 옮길 때 두 락 동시 holding 필요, 데드락 위험. 기각.

---

## ADR-006 — Disconnect 호출은 락 밖에서

**Status**: Accepted (ADR-005 따름)

**Context**
- 락 안에서 `Disconnect()` 를 호출하면, Disconnect → DecreaseSessionIO → (마지막 -1 시) FreeSession 경로가 직접 도는 것이 아니라, 워커의 GQCS 가 회수해 OnClientLeave → DeleteCharacter → 다시 `_characterLock` 로 들어옴 → **재진입 데드락**.

**Decision**
- 도메인 핸들러에서 Disconnect 가 필요한 경우, 끊을 SessionKey 를 로컬 변수에 저장 → `ReleaseSRWLockExclusive` → `Disconnect(savedKey)` 순서로 작성.
- 코드 리뷰 시 "락 안에서 Disconnect" 패턴 발견 즉시 거부.

**Consequences**
- (+) 데드락 차단.
- (-) 락 풀림 ~ Disconnect 사이 짧은 윈도우에 외부에서 같은 세션을 만질 수 있음 → SessionKey 매칭으로 흡수 (`GetSessionptr` 가 재활용 감지 시 nullptr 리턴).

---

## ADR-007 — 풀 capacity 상한 도입 (`MAX_PACKET_POOL`, `MAX_CHAR_POOL`)

**Status**: Accepted (2026-04)

**Context**
- `MemoryPoolForLockFree` 는 기본적으로 무제한 확장. 공격 트래픽으로 패킷 alloc 이 폭증하면 메모리가 무한 팽창.
- 한 번 fragmentation 된 풀은 회수해도 다시 줄지 않음.

**Decision**
- ServerConfig.ini 에서 `MAX_PACKET_POOL`, `MAX_CHAR_POOL` 설정. 0 이면 무제한.
- 채팅서버: PacketPool 50000, CharPool 30000 (세션 수의 1.5배).
- 모니터링서버: PacketPool 10000.
- 상한 도달 시 Alloc 실패 → SendPacket 실패로 전파 → Disconnect.

**Consequences**
- (+) 메모리 폭증을 정량적으로 막음.
- (+) 콘솔의 PacketPool 표시로 상태 즉시 가시화.
- (-) 정상 트래픽이 일시적으로 풀을 다 쓰면 일부 클라가 강제 끊김. 정상 부하 측정 후 여유 있게 설정 필요.

---

## ADR-008 — 세션당 RecvBuf 4 KB / SendBuf 512

**Status**: Accepted (2026-04, supersedes 16 KB / 16385)

**Context**
- 이전: RecvBuf 16 KB, SendBuf 16385. 15K 세션 시 ~500 MB 차지.
- 채팅 패킷 max payload = 500 B → 16 KB 는 명백한 과잉.

**Decision**
- RecvBuf = 4 KB (최대 패킷 한 개 + 여유), SendBuf = 512 (CPacket\* 큐).
- 세션당 ~8 KB → 15K 세션 = ~120 MB.

**Consequences**
- (+) 메모리 4배 절감.
- (-) 클라가 한 번에 매우 많이 보내거나 서버 처리가 늦으면 RecvBuf 가 빠르게 참 → Disconnect. 이는 의도된 동작 (느린 처리/공격 트래픽 방어).
- (-) SendBuf 512 가 부족한 운영 시나리오 (대규모 브로드캐스트 폭주) 가 발견되면 ADR-009 로 재조정 필요.

---

## ADR-009 — `getSendBufferFullCount()` 는 누적값

**Status**: Accepted (2026-04, supersedes 순간값 리셋)

**Context**
- 이전 구현은 `InterlockedExchange(&_sendBufferFullCount, 0)` 로 읽으면서 0 으로 리셋.
- 1초 사이에 0 → 5 → 0 식으로 보여 운영자가 "지금 정상" 으로 오인.

**Decision**
- 단순 read 만 수행 (`return _sendBufferFullCount;`). 누적값 표시.

**Consequences**
- (+) 시작 후 누적된 SendBufFull 횟수가 그대로 보임 — 추세 판단 가능.
- (-) "최근 1초" 같은 슬라이딩 정보는 외부 모니터링에서 차분으로 계산해야 함. (운영상 OK)

---

## ADR-010 — MonitoringServer = LAN + NET 단일 프로세스

**Status**: Accepted (2026-04)

**Context**
- 두 종류의 트래픽: 내부 서버(2바이트 헤더) / 외부 관제툴(5바이트 암호화).
- 옵션: (a) 두 프로세스로 분리, (b) 한 프로세스 내 두 클래스, (c) 한 클래스에 헤더 분기.

**Decision**
- (b) 채택. `CLanServer`(LAN) + `CNetServer`(NET) 를 각자 인스턴스화하여 `CMonitoringServer` 가 둘 다 owning.
- 두 클래스는 동일한 IOCP / 세션 관리 골격을 공유하지만 헤더 파싱 부분만 다름. 코드 중복은 의도적 (헤더 포맷이 영원히 같지는 않을 거라 가정).

**Consequences**
- (+) 한 프로세스에서 모든 지표가 한 자료구조 (`g_iMonitorData`) 에 모임 — 도메인 로직 간단.
- (+) 종료/시작 라이프사이클이 1개.
- (-) 코드 중복: 골격은 95% 동일, 헤더 파싱과 PACKET_KEY 만 다름. 향후 공통 베이스로 추출 가능.

**Alternatives considered**
- 하나의 클래스에서 `if (isLanPort)` 분기 → 헤더 검증 / 인코딩 경로가 양쪽으로 갈라져 가독성↓. 기각.
- 두 프로세스 분리 + 공유 메모리 → IPC 비용. 기각.
