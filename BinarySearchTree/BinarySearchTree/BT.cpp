#include <iostream>
using namespace std;

//클래스로 만드는게 더 나아보임.

struct Node {
	int data;
	Node* Parent;
	Node* LeftChild;
	Node* RightChild;
};

void InsertData(Node** curNode, int d, Node* parent)
{

	//현재 위치가 비어있다면 대입
	if (curNode == nullptr)
	{
		*curNode = new Node;
		(*curNode)->data = d;
		(*curNode)->Parent = parent;
	}
	//비어있지 않은 경우
	else {
		if ((*curNode)->data > d)
		{
			InsertData(&(*curNode)->LeftChild, d,*curNode);
		}
		else if((*curNode)->data < d)
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

void RemoveData(Node* curNode, int d)
{
	//데이터를 찾아 지우자

	//삭제할 데이터 찾지 못한 경우 예외처리내며 반환
	if (curNode == nullptr)
	{
		cout << "--------없는 데이터 삭제 시도--------" << "\n";
		return;
	}
	else
	{
		if (curNode->data > d)
		{
			RemoveData(curNode->LeftChild, d);
		}
		else if (curNode->data < d)
		{
			RemoveData(curNode->RightChild, d);
		}
		//삭제할 데이터를 찾은 경우
		else
		{
			//해당 데이터의 자식 노드 유무에 따라 동작이 달라짐.
			//자식이 없는 노드일 경우 걍 삭제
			if (curNode->LeftChild == nullptr && curNode->RightChild == nullptr)
			{
				delete curNode;
				curNode = nullptr;
			}
			else if (curNode->LeftChild == nullptr)
			{
				curNode = curNode->RightChild;
				delete(curNode->RightChild);
				curNode->RightChild = nullptr;
			}
			else if (curNode->RightChild == nullptr)
			{
				curNode = curNode->LeftChild;
				delete(curNode->LeftChild);
				curNode->LeftChild = nullptr;
			}
			//자식이 2개있는 경우
			//해당 자리를 대체할 수 있는 노드를 찾아야 함.
			else {

			}
		}
	}

}

void inorderTraversal(Node * curNode)
{
	if (curNode == nullptr) return;

	inorderTraversal(curNode->LeftChild);
	cout << curNode->data << " ";
	inorderTraversal(curNode->RightChild);
}

int main()
{
	Node* root = nullptr;

	bool loop = true;
	while (loop)
	{
		cout << "번호를 입력하세요." << "\n";
		cout << "1 : 삽입 " << "\n";
		cout << "2 : 삭제" << "\n";
		cout << "3 : 종료" << "\n";

		int input;
		cin >> input;
		switch (input)
		{
		case 1:
		{
			cout << "삽입할 데이터를 입력하세요. : " << "\n";
			int data;
			cin >> data;
			InsertData(&root, data, nullptr);

			//자동으로 중위순회 결과 트리 출력
			inorderTraversal(root);
			break;
		}
		case 2:
		{
			cout << "삭제할 데이터를 입력하세요. : " << "\n";
			int data;
			cin >> data;
			RemoveData(root, data);
			break;
		}
		case 3:
		{
			cout << "프로그램을 종료합니다." << "\n";
			loop = false;
			break;
		}
		default:
		{
			cout << "잘못된 번호입니다." << "\n";
			cout << "다시 입력하세요." << "\n";
			break;
		}

		}
	}


	return 0;
}