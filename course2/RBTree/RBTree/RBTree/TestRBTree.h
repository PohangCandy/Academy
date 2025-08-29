#pragma once
#include "RBTree.h"

//트리 검증을 위한 프로그램
//여러가지 랜덤한 데이터를 트리에 넣고 트리에 있는 결과와 원본 데이터 비교
//데이터 종류
//1.언밸런싱 -> 한쪽으로 쏠린 형태
// 계속 이전 데이터보다 큰 데이터 삽입하거나
// 이전 데이터보다 작은 데이터 삽입
// 오름차순, 내림 차순 으로 정렬 후 삽입
// 
//2.밸런싱 -> 완전 이진 트리 형태
// 등비 수열공식에 따라 완전 이진 트리 총 노드의 개수는 2^n - 1개의 데이터 
// 데이터 정렬 한 후
// 중앙에 있는 2^(n-1)번 째 항 부터 -1,+1한 인덱스 번호를 큐에 담아 front를 출력해가면서 해당 원소를 트리에 삽입 
// 
//3.랜덤
//걍 데이터 랜덤하게 생성한 후 삽입
//원본데이터 정렬 결과를 트리와 비교

class TestTree {

public:
	TestTree(int size)
	{
		_size = size;
	}

	//한쪽으로 쏠린 형태의 데이터
	void makeUnBalancing();



	void makePerfectBinaryTree();



	void makeListWithRand(int s, std::vector<int>& v);



	void InsertTree(RBTree* rbt);



	void compareData(RBTree* rbt);



	void printfData();


private:
	//삽입 삭제 테스트 하려면 리스트가 나을것으로 보임.
	std::vector <int> v_data;
	int _size;
};