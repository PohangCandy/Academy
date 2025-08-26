#include "BinarySearchTree.h"
#include <iostream>
using namespace std;

void BT::InsertData(Node** curNode, int d, Node* parent)
{

	//현재 위치가 비어있다면 대입
	if (*curNode == nullptr)
	{
		*curNode = new Node;
		(*curNode)->data = d;
		(*curNode)->Parent = parent;
		(*curNode)->LeftChild = nullptr;
		(*curNode)->RightChild = nullptr;
	}
	//비어있지 않은 경우
	else {
		if ((*curNode)->data > d)
		{
			InsertData(&(*curNode)->LeftChild, d, *curNode);
		}
		else if ((*curNode)->data < d)
		{
			InsertData(&(*curNode)->RightChild, d, *curNode);
		}
		//중복된 데이터가 있다면 여기서 거른다.
		else {
			cout << "--------중복 데이터 감지--------" << "\n";
			return;
		}
	}
}

void BT::RemoveData(Node** curNode, int d)
{
	//데이터를 찾아 지우자

	//삭제할 데이터 찾지 못한 경우 예외처리내며 반환
	if (*curNode == nullptr)
	{
		cout << "--------없는 데이터 삭제 시도--------" << "\n";
		return;
	}
	else
	{
		if ((*curNode)->data > d)
		{
			RemoveData(&((*curNode)->LeftChild), d);
		}
		else if ((*curNode)->data < d)
		{
			RemoveData(&((*curNode)->RightChild), d);
		}
		//삭제할 데이터를 찾은 경우
		else
		{
			//해당 데이터의 자식 노드 유무에 따라 동작이 달라짐.
			//자식이 없는 노드일 경우 걍 삭제
			if ((*curNode)->LeftChild == nullptr && (*curNode)->RightChild == nullptr)
			{
				delete (*curNode);
				*curNode = nullptr;
			}
			else if ((*curNode)->LeftChild == nullptr)
			{
				Node* tmp = *curNode;
				*curNode = (*curNode)->RightChild;
				delete(tmp);
				tmp = nullptr;
			}
			else if ((*curNode)->RightChild == nullptr)
			{
				Node* tmp = *curNode;
				*curNode = (*curNode)->LeftChild;
				delete(tmp);
				tmp = nullptr;
			}
			//자식이 2개있는 경우
			//해당 자리를 대체할 수 있는 노드를 찾아야 함.
			//왼쪽 자식의 가장 오른쪽 노드 or 오른쪽 자식의 가장 왼쪽 노드
			//왼-오 노드로 선택
			else {
				Node* LeftChild = (*curNode)->LeftChild;
				while (LeftChild->RightChild != nullptr)
				{
					Node* RightestChild = LeftChild->RightChild;
					LeftChild = RightestChild;
				}
				//왼오 노드의 데이터를 현재 위치에 대입한 후,
				//왼오 노드는 삭제한다.
				(*curNode)->data = LeftChild->data;
				RemoveData(&(*curNode)->LeftChild, LeftChild->data);
			}
		}
	}

}

void BT::inorderTraversal(vector<int>& vout, Node* curNode)
{
	if (curNode == nullptr) return;

	inorderTraversal(vout, curNode->LeftChild);
	cout << curNode->data << " ";
	vout.push_back(curNode->data);
	inorderTraversal(vout, curNode->RightChild);
}

void BT::destroyTree(Node** curNode)
{
	if (*curNode == nullptr) return;

	destroyTree(&(*curNode)->LeftChild);
	destroyTree(&(*curNode)->RightChild);
	delete* curNode;
	*curNode = nullptr;
}

