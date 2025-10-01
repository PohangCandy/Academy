#pragma once
#include <vector>

// 시각화 드로잉 상수 (g_iGridSize는 노드 크기 결정에만 사용)
extern const int NODE_RADIUS; // 고정된 노드 반지름 (또는 g_iGridSize/2)
//const int H_SPACE_INITIAL = 1000; // 루트 레벨의 초기 수평 간격 (트리의 너비 결정)

//새로 추가: 노드 중심 간 최소 수평 간격
extern const int H_NODE_DISTANCE; // 노드 반지름의 4배 (겹침 방지)
//새로 추가: 다음 노드의 X 좌표를 추적하는 변수
extern int g_nextNodeX;

enum NODE_COLOR
{
	BLACK = 0,
	RED
};

struct stNODE
{
	stNODE* pParent;
	stNODE* pLeft;
	stNODE* pRight;

	int calculatedX;

	NODE_COLOR Color;

	int iData;   // Key , Value
};

class RBTree {
public:
	RBTree() :root(&Nil), iSize(0) {
		Nil.Color = BLACK;
		Nil.pParent = &Nil;
		Nil.pLeft = &Nil;
		Nil.pRight = &Nil;
	}

	~RBTree() {
		clear();
	}

	void Insert(int data) {
		InsertData(&root, data, nullptr);
	}

	void Remove(int data) {
		RemoveData(&root, data);
		makeRootandNilBecomeBlack();
	}

	void InOrder(std::vector<int>& vout) {
		inorderTraversal(vout, root);
	}

	stNODE* getRoot(){ return root; }
	stNODE* getNill(){ return &Nil; }

	stNODE* root;

	bool isRBTreeValid();

	size_t getSize() const { return iSize; } // 현재 트리의 노드 개수를 반환

	void clear();

	// RBTree.h (클래스 선언에 추가)
	void calculateXCoordinates();
	void calculateXRecursive(stNODE* curNode); // 재귀 도우미 함수

private:

	void destroyTree(stNODE** curNode);

	size_t iSize;

	bool checkBSTProperty(stNODE* node, int minVal, int maxVal);
	int checkBlackHeight(stNODE* node);

	stNODE Nil;	// 끝 리프노드. 무조건 블랙 / 데이터 무 / NULL 의 같은 용도.

	void InsertData(stNODE** curNode, int d, stNODE* parent);

	void MakeBalacingAfterInsert(stNODE** curNode);

	void makeRightRotate(stNODE* curNode);

	void makeLeftRotate(stNODE* curNode);

	void ChangeColor(stNODE* curNode, NODE_COLOR color);

	void RemoveData(stNODE** curNode, int d);

	void MakeBalacingAfterRemove(stNODE** curNode);

	void RBTransplant(stNODE* u, stNODE* v);

	stNODE* TreeMinimum(stNODE* node);

	void inorderTraversal(std::vector<int>& vout, stNODE* curNode);

	void makeRootandNilBecomeBlack();
};