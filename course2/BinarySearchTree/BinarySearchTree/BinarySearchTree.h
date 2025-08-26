#pragma once
#include <vector>

struct Node {
	int data;
	Node* Parent;
	Node* LeftChild;
	Node* RightChild;
};

class BT {
public:
	BT() {
		root = nullptr;
	}
	~BT() {
		destroyTree(&root);
	}

	void Insert(int data) {
		InsertData(&root, data, nullptr);
	}

	void Remove(int data) {
		RemoveData(&root, data);
	}

	void InOrder(std::vector<int>& vout){
		inorderTraversal(vout, root);
	}

private:
	Node* root;

	void InsertData(Node** curNode, int d, Node* parent);

	void RemoveData(Node** curNode, int d);

	void inorderTraversal(std::vector<int>& vout, Node* curNode);

	void destroyTree(Node** curNode);
};