#include <iostream>
#include <cstdlib> 
#include <ctime>   
#include <queue>   
#include <cmath>   
#include "JPS.h"   
#include "Dungeon.h" 
using namespace std;

// 이동 비용 상수 정의(Dungeon / JPS와 일치해야 함)
const float DIAGONAL_COST = 1.5f; // sqrt(2)
const float CARDINAL_COST = 1.0f;       // 상하좌우
#define TEST_GRID_HEIGHT 100 
#define TEST_GRID_WIDTH 100
#define NUM_STRESS_TESTS 1000 

struct GridComparer {
    bool operator()(const Grid* g1, const Grid* g2) const {
        return g1->g > g2->g; // G 값이 작은 것이 우선순위가 높다 (min-heap)
    }
};


float ComputeBFS(Dungeon& dungeon)
{
    dungeon.InitMap();

    // G Cost를 무한대로 초기화 (Dijkstra의 필수 단계)
    const float MAX_G_VALUE = 1000000.0f;
    for (int y = 0; y < dungeon.getheight(); ++y) {
        for (int x = 0; x < dungeon.getwidth(); ++x) {
            Grid* grid = dungeon.getGrid(y, x);
            grid->g = MAX_G_VALUE; // G Cost를 무한대로 설정
            grid->gparent = nullptr; // 부모 포인터 초기화
        }
    }
    // --------------------------------------------------------------------

    priority_queue<Grid*, vector<Grid*>, GridComparer> open_list;
    Grid* g_start = dungeon.getStart();
    Grid* g_goal = dungeon.getGoal();

    if (dungeon.IsObstacle(g_start->y, g_start->x) || dungeon.IsObstacle(g_goal->y, g_goal->x)) {
        return -1.0f;
    }

    g_start->g = 0.0f; // 시작 지점만 G=0
    open_list.push(g_start);


    int dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    while (!open_list.empty())
    {
        Grid* current = open_list.top();
        open_list.pop();

        if (current == g_goal) {
            return current->g;
        }

        // 현재 G cost가 이웃을 통해 갱신될 수 있는 최소 G cost보다 크다면 스킵
        // (같은 노드가 큐에 중복 삽입되는 문제를 일부 방지)
        // if (current->g > neighbor->g) continue; // 이웃과 비교는 복잡하므로 간단히 생략

        for (int i = 0; i < 8; ++i)
        {
            int ny = current->y + dy[i];
            int nx = current->x + dx[i];

            if (ny < 0 || nx < 0 || ny >= dungeon.getheight() || nx >= dungeon.getwidth()) continue;
            if (dungeon.IsObstacle(ny, nx)) continue;

            Grid* neighbor = dungeon.getGrid(ny, nx);
            float cost = (dy[i] != 0 && dx[i] != 0) ? DIAGONAL_COST : CARDINAL_COST;
            float new_g = current->g + cost;

            // 순수한 G Cost 비교 로직
            if (new_g < neighbor->g)
            {
                neighbor->g = new_g;
                neighbor->gparent = current;
                open_list.push(neighbor);
            }
        }
    }

    return -1.0f;
}
// ... (RunPathfindingTest 함수는 이전과 동일하게 유지) ...

//----------------------------------------------------
// 단일 테스트 실행 함수 (RunPathfindingTest 함수는 변경 없음)
//----------------------------------------------------
void RunPathfindingTest(Dungeon& dungeon, JPS& astar, const string& test_name)
{
    dungeon.InitMap();

    bool success = false;
    Grid* goal_grid = nullptr;
    float jps_cost = -1.0f;

    cout << "\n--- 테스트 실행: " << test_name << " ---" << endl;

    try
    {
        astar.findPath();
        goal_grid = dungeon.getGoal();

        success = (goal_grid != nullptr && goal_grid->gparent != nullptr);

        if (success)
        {
            jps_cost = goal_grid->g;
            cout << test_name << " 결과:  JPS 성공!" << endl;
            cout << "  > JPS 경로 길이 (G Cost): " << jps_cost << endl;
        }
        else
        {
            cout << test_name << " 결과:  JPS 실패 (경로를 찾지 못함)." << endl;
        }
    }
    catch (const exception& e)
    {
        cout << test_name << " 결과:  예외 발생 (Fatal Error)! 메시지: " << e.what() << endl;
        success = false;
    }
    catch (...)
    {
        cout << test_name << " 결과:  알 수 없는 예외 발생!" << endl;
        success = false;
    }

    cout << "----------------------------------------------" << endl;
}


//----------------------------------------------------
// 1000회 반복 스트레스 테스트 함수
//----------------------------------------------------
void RunStressTests(Dungeon& dungeon, JPS& astar)
{
    cout << "\n==============================================" << endl;
    cout << "   JPS 알고리즘 대규모 스트레스 테스트 시작 (1000회)" << endl;
    cout << "  [JPS vs BFS 최단 경로/성공 여부 비교 검증 포함]" << endl;
    cout << "==============================================" << endl;

    int success_count = 0;
    int failure_count = 0;
    int error_count = 0; // JPS 예외 횟수
    int mismatch_count = 0; // JPS 결과 불일치 횟수

    // 무작위 장애물 비율 (10% ~ 40% 사이)
    const float MIN_OBS_RATIO = 0.10f;
    const float MAX_OBS_RATIO = 0.40f;
    const float EPSILON = 0.001f; // 부동 소수점 비교 오차

    for (int i = 1; i <= NUM_STRESS_TESTS; ++i)
    {
        // 1. 무작위 맵 생성
        float random_ratio = MIN_OBS_RATIO + (float)rand() / RAND_MAX * (MAX_OBS_RATIO - MIN_OBS_RATIO);
        dungeon.GenerateRandomMap(random_ratio);



        // 3. JPS 실행
        dungeon.InitMap(); // JPS 실행 전 맵 초기화 (BFS 흔적 제거)
        float jps_cost = -1.0f;
        bool jps_success = false;
        bool exception_occurred = false;

        bool Bfsexception_occurred = false;


        try
        {
            astar.findPath();
            Grid* goal_grid = dungeon.getGoal();
            jps_success = (goal_grid != nullptr && goal_grid->gparent != nullptr);
            if (jps_success) {
                jps_cost = goal_grid->g;
            }
        }
        catch (...)
        {
            exception_occurred = true;
        }


        float bfs_cost = -1.0f;
        bool bfs_success = false;

        try
        {
            // 2. BFS 실행 (JPS 비교 기준값)
            // BFS는 JPS와 무관하게 맵을 초기화하고 독립적으로 최단 경로 보장
            bfs_cost = ComputeBFS(dungeon);
            bfs_success = (bfs_cost > 0.0f);
        }
        catch (...)
        {
            Bfsexception_occurred = true;
        }

        // 4. 결과 비교 및 검증
        bool result_mismatch = false;

        if (exception_occurred)
        {
            error_count++;
            result_mismatch = true; // 예외는 불일치로 간주
            cerr << " FATAL (Test #" << i << "): JPS 예외 발생!" << endl;
        }
        else if (Bfsexception_occurred)
        {
            result_mismatch = true; // 예외는 불일치로 간주
            cerr << " FATAL (Test #" << i << "): BFS 예외 발생!" << endl;
        }
        else if (jps_success != bfs_success)
        {
            // 성공/실패 여부가 불일치: JPS가 경로를 찾았는데 BFS가 못 찾았거나 그 반대인 경우
            result_mismatch = true;
            cerr << " FAIL (Test #" << i << "): 성공 여부 불일치! JPS:" << jps_success << ", BFS:" << bfs_success << endl;
        }
        else if (jps_success && bfs_success)
        {
            // 둘 다 성공: 최단 경로 비용 비교
            if (std::abs(jps_cost - bfs_cost) > EPSILON)
            {
                result_mismatch = true;
                cerr << " FAIL (Test #" << i << "): 최단 경로 비용 불일치! JPS:" << jps_cost << ", BFS:" << bfs_cost << endl;
            }
        }
        // else: 둘 다 실패한 경우 (길이 없는게 맞는지 확인 완료) -> 정상

        // 5. 결과 집계 및 로그 출력
        if (result_mismatch) {
            mismatch_count++;

            cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
            cerr << " FAIL (Test #" << i << "): 불일치 발생! 디버깅을 위해 루프를 중단합니다." << endl;
            cerr << "   - Ratio: " << random_ratio << endl;
            cerr << "   - JPS Cost: " << jps_cost << ", BFS Cost: " << bfs_cost << endl;
            cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
        }
        else if (jps_success) {
            success_count++;
        }
        else {
            failure_count++;
        }

        if (i % 100 == 0 || i == NUM_STRESS_TESTS) {
            cout << "[LOG] " << i << "회 테스트 완료. 성공: " << success_count
                << ", 실패: " << failure_count << ", 불일치: " << mismatch_count << ", 예외: " << error_count << endl;
        }
    }

    cout << "\n==============================================" << endl;
    cout << "  대규모 테스트 최종 결과" << endl;
    cout << "==============================================" << endl;
    cout << "총 테스트 횟수: " << NUM_STRESS_TESTS << endl;
    cout << "--- 검증 결과 ---" << endl;
    cout << "정상 성공 횟수: " << success_count << endl;
    cout << "정상 실패 횟수: " << failure_count << endl;
    cout << "--- 오류 결과 ---" << endl;
    cout << "알고리즘 예외 발생 횟수: " << error_count << endl;
    cout << "BFS 결과 불일치 횟수: " << mismatch_count << " ( 0이어야 함!)" << endl;
}

//----------------------------------------------------
// 전체 테스트 실행 함수 (RunStressTests 추가)
//----------------------------------------------------
void RunAllTests()
{
    // rand() 시드를 고정하여 매번 동일한 무작위 맵이 생성되도록 함.
    srand(static_cast<unsigned int>(time(0)));

    Dungeon dungeon(TEST_GRID_HEIGHT, TEST_GRID_WIDTH);
    JPS astar(&dungeon);

    // 대규모 스트레스 테스트 실행
    RunStressTests(dungeon, astar);

}


//----------------------------------------------------
// main 함수
//----------------------------------------------------
int main()
{
    RunAllTests();

    cout << "\n모든 테스트 완료. 엔터 키를 눌러 종료하십시오...";
    cin.get();

    return 0;
}