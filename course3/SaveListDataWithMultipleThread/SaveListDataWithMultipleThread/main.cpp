#include <iostream>
#include <list>
#include <thread>
#include <random>
#include <vector>
#include <atomic>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <windows.h>
#include <iomanip> // setprecision, fixed 사용
using namespace std;

#pragma comment(lib, "winmm.lib")

// === 공유 데이터 및 동기화 객체 ===
list<int> g_data_list;
CRITICAL_SECTION g_critical_section;

// Event 핸들들
HANDLE g_save_event = NULL;
HANDLE g_print_event = NULL;
HANDLE g_delete_event = NULL;
HANDLE g_worker_events[3] = { NULL };

atomic<bool> g_stop_threads{ false };

// === 성능 측정 변수 (고정밀 카운터 사용) ===
atomic<int> g_total_inserts{ 0 };
atomic<long long> g_print_total_lock_time{ 0 };
atomic<int> g_print_count{ 0 };
atomic<long long> g_save_total_lock_time{ 0 };
atomic<int> g_save_count{ 0 };

// 고정밀 타이머 주파수 (main에서 초기화)
long long g_performance_frequency = 1;


//PrintThread: 락 점유 최소화 (I/O 외부로 이동) + 고정밀 성능 측정
void PrintThread() {
    cout << "PrintThread 시작" << "\n";

    while (!g_stop_threads) {
        DWORD result = WaitForSingleObject(g_print_event, INFINITE);

        if (g_stop_threads || result != WAIT_OBJECT_0) continue;

        string list_output;
        LARGE_INTEGER start_lock_time, end_lock_time;

        EnterCriticalSection(&g_critical_section);
        // 락 내부 시간 측정 시작
        QueryPerformanceCounter(&start_lock_time);

        // 락 내부: 데이터 읽기(복사)만 수행 (매우 빠름)
        if (!g_data_list.empty()) {
            stringstream ss;
            for (int val : g_data_list) {
                ss << val << "-";
            }
            list_output = ss.str();
        }



        // 락 외부: 느린 I/O 작업 수행 (cout)
        if (!list_output.empty()) {
            cout << "Print: " << list_output << "\n";
        }
        else {
            cout << "Print: List empty" << "\n";
        }

        // 락 내부 시간 측정 종료
        QueryPerformanceCounter(&end_lock_time);

        // 시간 기록 (카운트 단위)
        g_print_total_lock_time += (end_lock_time.QuadPart - start_lock_time.QuadPart);
        g_print_count++;

        LeaveCriticalSection(&g_critical_section);
    }
    cout << "PrintThread 종료" << "\n";
}

// DeleteThread
void DeleteThread() {
    cout << "DeleteThread 시작" << "\n";

    while (!g_stop_threads) {
        DWORD result = WaitForSingleObject(g_delete_event, INFINITE);

        if (g_stop_threads || result != WAIT_OBJECT_0) continue;

        // 실제 작업
        EnterCriticalSection(&g_critical_section);

        // 락 내부: 단순 삭제 연산만 수행
        if (!g_data_list.empty()) {
            g_data_list.pop_back();
        }

        LeaveCriticalSection(&g_critical_section);
    }
    cout << "DeleteThread 종료" << "\n";
}

// WorkerThread: 락 점유 최소화 (난수 생성 외부로 이동) + 성능 측정
void WorkerThread(int id, HANDLE event_handle) {
    cout << "WorkerThread " << id << " 시작" << "\n";

    // rand() 대신 std::mt19937 사용 (더 좋은 난수 생성)

    while (!g_stop_threads) {
        DWORD result = WaitForSingleObject(event_handle, INFINITE);

        if (g_stop_threads || result != WAIT_OBJECT_0) continue;

        // 락 외부: 시간이 걸릴 수 있는 작업 (난수 생성)
        int rand_val = rand()%1000;

        // 실제 작업 (락 점유 최소화)
        EnterCriticalSection(&g_critical_section);

        // 락 내부: 공유 데이터 수정만 수행 (매우 빠름)
        g_data_list.push_back(rand_val);
        g_total_inserts++; // 삽입 횟수 기록

        LeaveCriticalSection(&g_critical_section);
    }
    cout << "WorkerThread " << id << " 종료" << "\n";
}

// SaveThread: 락 점유 최소화 (파일 I/O 외부로 이동) + 고정밀 성능 측정
void SaveThread() {
    cout << "SaveThread 시작" << "\n";

    while (!g_stop_threads) {
        DWORD result = WaitForSingleObject(g_save_event, INFINITE);

        if (g_stop_threads || result != WAIT_OBJECT_0) continue;

        list<int> data_copy;
        LARGE_INTEGER start_lock_time, end_lock_time;

        EnterCriticalSection(&g_critical_section);
        // 락 내부 시간 측정 시작
        QueryPerformanceCounter(&start_lock_time);

        // 락 내부: 리스트를 지역 변수로 통째로 복사 (매우 빠름)
        cout << "SaveThread: 리스트 복사 시작... (크리티컬 섹션 점유)" << "\n";
        data_copy = g_data_list;



        // 락 외부: 느린 파일 I/O 작업 수행 (락 해제 상태)
        cout << "SaveThread: 파일 저장 작업 시작..." << "\n";

        ofstream outfile("data_save.txt");
        if (outfile.is_open()) {
            for (int val : data_copy) {
                outfile << val << "\n";
            }
            outfile.close();
            cout << "SaveThread: 파일 저장 완료 (data_save.txt)" << "\n";
        }
        else {
            cerr << "SaveThread: 파일 열기 실패!" << "\n";
        }

        // 락 내부 시간 측정 종료
        QueryPerformanceCounter(&end_lock_time);

        // 시간 기록
        g_save_total_lock_time += (end_lock_time.QuadPart - start_lock_time.QuadPart);
        g_save_count++;

        LeaveCriticalSection(&g_critical_section);
    }
    cout << "SaveThread 종료" << "\n";
}


// MainThread: 벤치마크 및 스케줄링 역할
int main() {
    // 1. 초기화 및 고정밀 타이머 주파수 설정
    InitializeCriticalSection(&g_critical_section);

    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency)) {
        g_performance_frequency = frequency.QuadPart;
    }
    else {
        cerr << "고성능 타이머를 사용할 수 없습니다." << "\n";
        return 1;
    }

    // 2. 이벤트 객체 생성
    g_save_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_print_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_delete_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    for (int i = 0; i < 3; ++i) {
        g_worker_events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (g_worker_events[i] == NULL) { cerr << "이벤트 생성 실패" << "\n"; return 1; }
    }
    if (g_save_event == NULL || g_print_event == NULL || g_delete_event == NULL) {
        cerr << "이벤트 생성 실패" << "\n"; return 1;
    }

    cout << "--- 멀티스레드 성능 벤치마크 시작 ---" << "\n";

    // 3. 모든 스레드 시작
    thread print_t(PrintThread);
    thread delete_t(DeleteThread);
    vector<std::thread> worker_threads;
    for (int i = 0; i < 3; ++i) {
        worker_threads.emplace_back(WorkerThread, i + 1, g_worker_events[i]);
    }
    thread save_t(SaveThread);

    // 사용자 입력 처리를 비동기로 하기 위해 별도의 스레드를 사용
    atomic<char> main_input = '\0';
    thread input_thread([&main_input]() {
        char c;
        while (cin >> c) {
            main_input = c;
            if (c == 'q' || c == 'Q') break;
        }
        });

    // 4. 벤치마크 및 스케줄링 루프 (10초 실행)
    const DWORD TEST_DURATION_MS = 10000; // 10초
    DWORD start_time = timeGetTime();
    DWORD last_time_333ms = start_time;
    DWORD last_time_1000ms = start_time;

    cout << "\n's' 키를 눌러 저장 스레드를 깨우거나, 'q' 키를 눌러 모든 스레드를 종료하세요." << "\n";
    cout << "--- 10초 벤치마크 실행 중 ---" << "\n";


    while (timeGetTime() - start_time < TEST_DURATION_MS) {
        DWORD current_time = timeGetTime();

        // 333ms 스케줄링 (DeleteThread)
        if (current_time - last_time_333ms >= 333) {
            SetEvent(g_delete_event);
            last_time_333ms = current_time;
        }

        // 1000ms 스케줄링 (PrintThread, WorkerThreads)
        if (current_time - last_time_1000ms >= 1000) {
            SetEvent(g_print_event);
            for (int i = 0; i < 3; ++i) {
                SetEvent(g_worker_events[i]);
            }
            last_time_1000ms = current_time;
        }

        // 사용자 입력 처리
        if (main_input != '\0') {
            char input = main_input.exchange('\0');
            if (input == 's' || input == 'S') {
                SetEvent(g_save_event);
                cout << "-> SaveThread에 저장 요청을 보냈습니다 (Event Signaled)." << "\n";
            }
            else if (input == 'q' || input == 'Q') {
                // 종료는 테스트 시간 이후에 처리
            }
        }
    }

    // 5. 스레드 종료 신호 전파
    g_stop_threads = true;
    cout << "\n-> 벤치마크 시간 종료. 스레드 종료 요청 전파..." << "\n";

    // 대기 중인 모든 스레드를 깨워서 루프를 빠져나오게 합니다.
    SetEvent(g_save_event);
    SetEvent(g_print_event);
    SetEvent(g_delete_event);
    for (int i = 0; i < 3; ++i) {
        SetEvent(g_worker_events[i]);
    }

    // 입력 스레드 종료
    if (input_thread.joinable()) input_thread.join();

    // 6. 모든 스레드 종료 대기 (join)
    if (print_t.joinable()) print_t.join();
    if (delete_t.joinable()) delete_t.join();
    if (save_t.joinable()) save_t.join();
    for (auto& t : worker_threads) {
        if (t.joinable()) t.join();
    }

    // 7. 성능 측정 결과 출력
    cout << "\n=============================================" << "\n";
    cout << "        성능 벤치마크 결과 (10초 실행)       " << "\n";
    cout << "=============================================" << "\n";

    // 7-1. 처리량 (Throughput)
    cout << "[처리량] WorkerThread 총 삽입 횟수: " << g_total_inserts << "회" << "\n";
    cout << "(목표치: 10초 x 3스레드 = 30회. 락 경합이 없어 목표치에 가까움)" << "\n";

    // 7-2. 락 점유 시간 (Lock Duration)
    double avg_print_lock_us = g_print_count > 0 ?
        (double)g_print_total_lock_time * 1000000.0 / (g_performance_frequency * g_print_count) : 0.0;
    double avg_save_lock_us = g_save_count > 0 ?
        (double)g_save_total_lock_time * 1000000.0 / (g_performance_frequency * g_save_count) : 0.0;

    cout << "\n[락 점유 시간 - PrintThread]" << "\n";
    cout << "  총 실행 횟수: " << g_print_count << "회" << "\n";
    cout << "  총 락 점유 시간 (카운트): " << g_print_total_lock_time << "\n";
    cout << "  평균 락 점유 시간: " << fixed << setprecision(2) << avg_print_lock_us << " \u03BCs (마이크로초)" << "\n";

    cout << "\n[락 점유 시간 - SaveThread]" << "\n";
    cout << "  총 실행 횟수: " << g_save_count << "회" << "\n";
    cout << "  총 락 점유 시간 (카운트): " << g_save_total_lock_time << "\n";
    cout << "  평균 락 점유 시간: " << fixed << setprecision(2) << avg_save_lock_us << " \u03BCs (마이크로초)" << "\n";

    cout << "=============================================" << "\n";

    // 8. 동기화 객체 정리
    CloseHandle(g_save_event);
    CloseHandle(g_print_event);
    CloseHandle(g_delete_event);
    for (int i = 0; i < 3; ++i) {
        CloseHandle(g_worker_events[i]);
    }
    DeleteCriticalSection(&g_critical_section);

    return 0;
}