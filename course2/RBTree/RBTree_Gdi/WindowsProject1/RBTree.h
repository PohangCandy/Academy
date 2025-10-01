#pragma once
#include <vector>

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