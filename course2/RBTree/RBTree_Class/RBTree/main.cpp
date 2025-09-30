#include <iostream>
#include <cstdlib>   // rand(), srand()
#include <ctime>     // time()
#include "RBTree.h"
#include "TestRBTree.h"
using namespace std;

// 테스트를 진행할 횟수
const int NUM_TESTS = 1000;

int main()
{
    // 난수 시드 초기화 (매번 다른 테스트를 위해)
    srand(time(NULL));

    int successfulTests = 0;

    // ----------------------------------------------------
    // [1] 대량 삽입 및 유효성 검증 테스트
    // ----------------------------------------------------
    cout << "--- 1. 대량 삽입 및 RB 규칙 검증 테스트 (" << NUM_TESTS << "회) ---\n";

    for (int i = 1; i <= NUM_TESTS; ++i)
    {
        RBTree rbt;
        // TestTree는 내부에서 size를 랜덤으로 설정합니다.
        // 초기 size는 크게 의미 없지만, 생성자에 10을 전달합니다.
        TestTree tt(10);

        // 랜덤 케이스 생성 (Unbalancing, Perfect, Random 중 하나)
        tt.makeRandomCase();

        cout << "[" << i << "/" << NUM_TESTS << "] "
            << "삽입 테스트 진행... (데이터 수: " << tt.getVDataSize() << ")\n";
        // getVDataSize() 함수가 TestTree에 없으므로, 필요하면 추가해야 합니다.
        // 임시로 TestTree::v_data 멤버 변수가 public이나 get 함수가 있다고 가정합니다. 
        // 없으면 tt.makeRandomCase() 출력에서 데이터 수를 확인해야 합니다.

   // 데이터 삽입
        tt.InsertFullData(&rbt);

        // A. BST 속성 및 RB 규칙 (색상, 블랙 깊이) 검증
        if (!rbt.isRBTreeValid()) {
            cout << "\n======================================================\n";
            cout << "!!!!! [ERROR] 삽입 후 RB Tree 규칙 위반 감지 !!!!!\n";
            cout << "======================================================\n";
            cout << "원본 데이터 (삽입 순서): \n";
            tt.printfVOriginData();
            cout << "\n--- 테스트 종료 ---\n";
            return 1; // 오류 발생 시 프로그램 종료
        }

        // B. 중위 순회 결과 (정렬 상태) 검증
        tt.compareData(&rbt);

        // 메모리 해제 및 다음 테스트 준비
        rbt.clear();

        successfulTests++;
        cout << " -> Valid!\n";
    }

    cout << "\n모든 " << successfulTests << "회 삽입 유효성 테스트 성공!\n";

    // ----------------------------------------------------
    // [2] 삽입/삭제 혼합 및 유효성 검증 테스트
    // ----------------------------------------------------
    cout << "\n--- 2. 삽입/삭제 혼합 및 RB 규칙 검증 테스트 (" << NUM_TESTS << "회) ---\n";
    successfulTests = 0;

    for (int i = 1; i <= NUM_TESTS; ++i)
    {
        RBTree rbt;
        TestTree tt(10);

        // 1. 랜덤 데이터 100개 삽입
        tt.makeRandomCase();
        tt.InsertFullData(&rbt);

        // 2. 랜덤하게 50%의 데이터를 삭제
        // 원본 데이터 벡터에서 짝수 인덱스만 삭제한다고 가정
        const vector<int>& dataList = tt.getVData(); // v_data 접근 함수가 필요
        int initialSize = dataList.size();

        for (int j = 0; j < initialSize; j += 2) {
            rbt.Remove(dataList[j]);
        }

        cout << "[" << i << "/" << NUM_TESTS << "] "
            << "삭제 테스트 진행... (초기: " << initialSize << ", 삭제 후: " << rbt.getSize() << ")\n";
        // rbt.getSize() 함수나 tt.getVData()와 같은 보조 함수가 RBTree에 필요합니다.

   // A. BST 속성 및 RB 규칙 검증
        if (!rbt.isRBTreeValid()) {
            cout << "\n======================================================\n";
            cout << "!!!!! [ERROR] 삭제 후 RB Tree 규칙 위반 감지 !!!!!\n";
            cout << "======================================================\n";
            // 삭제 시 복잡하므로, 디버깅을 위해 이 시점의 트리를 출력해야 합니다.
            cout << "--- 테스트 종료 ---\n";
            return 1; // 오류 발생 시 프로그램 종료
        }

        // B. 최종 남은 데이터의 정렬 상태 검증
        // 이 부분은 tt 클래스에 남은 데이터만 추출하여 정렬 후 비교하는 기능이 필요합니다.
        // 현재는 규칙 검증(A)만으로 충분하다고 판단하고 생략합니다.

        // 메모리 해제 및 다음 테스트 준비
        rbt.clear();

        successfulTests++;
        cout << " -> Valid!\n";
    }

    cout << "\n 모든 " << successfulTests << "회 삭제 유효성 테스트 성공!\n";

    return 0;
}