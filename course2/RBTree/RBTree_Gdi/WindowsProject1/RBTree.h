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

	NODE_COLOR Color;

	int iData;   // Key , Value
};

class RBTree {
public:
	RBTree() {
		Nil.Color = BLACK;
		Nil.pParent = NULL;
		Nil.pLeft = NULL;
		Nil.pRight = NULL;

		root = &Nil;
	}
	~RBTree() {
		destroyTree(&root);
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

private:
	stNODE* root;

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

	void destroyTree(stNODE** curNode);

	void makeRootandNilBecomeBlack();
};