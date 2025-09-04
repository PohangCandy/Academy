#include "RBTree.h"
#include <iostream>
using namespace std;

void RBTree::InsertData(stNODE** curNode, int d, stNODE* parent)
{

	//현재 위치가 비어있다면 대입
	if (*curNode == &Nil)
	{
		*curNode = new stNODE;
		(*curNode)->iData = d;
		(*curNode)->pParent = parent;
		(*curNode)->pLeft = &Nil;
		(*curNode)->pRight = &Nil;
		(*curNode)->Color = RED;
		//데이터 삽입에 성공하면 삽입 후 
		// 삽입된 노드를 중심으로 밸런싱이 일어난다.
		MakeBalacingAfterInsert(curNode);
		//밸런싱 후 루트노드와 닐 노드는 항상 Black으로 만들어줘야 함.
		makeRootandNilBecomeBlack();

		return;
	}
	//비어있지 않은 경우
	else {
		if ((*curNode)->iData > d)
		{
			InsertData(&(*curNode)->pLeft, d, *curNode);
		}
		else if ((*curNode)->iData < d)
		{
			InsertData(&(*curNode)->pRight, d, *curNode);
		}
		//중복된 데이터가 있다면 여기서 거른다.
		else {
			cout << "--------중복 데이터 감지--------" << "\n";
			return;
		}
	}
}

//단일 포인터로도 충분할 것으로 보임.
//직접 회전하면서 curnode를 갱신하진 않음.
void RBTree::MakeBalacingAfterInsert(stNODE** curNode)
{
	//부모가 없다면 밸런싱 필요없음.
	if ((*curNode)->pParent == nullptr) return;

	//부모가 블랙이라면 밸런싱 할 필요없음.
	if ((*curNode)->pParent->Color == BLACK) return;
	//부모가 레드일 경우
	else if ((*curNode)->pParent->Color == RED)
	{
		//형제, 부모, 조부모, 삼촌을 확보한다.
		//없을 경우도 고려? -> ㄴㄴ
		//부모가 레드라는건 조부모는 무조건 블랙
		//삼촌은 닐 노드라도 존대할 것

		stNODE* parent = (*curNode)->pParent;
		stNODE* sibling;
		//이때 내가 어느 위치의 자식으로 들어갈지도 알아두자.
		bool bIsMyPosisLeft = true;
		if (*curNode == parent->pLeft)
		{
			sibling = parent->pRight;
		}
		else
		{
			bIsMyPosisLeft = false;
			sibling = parent->pLeft;
		}

		stNODE* grandparent = parent->pParent;
		stNODE* uncle;
		//조부모 nullptr오류 -> RR오류가 나면 조부모가 nullptr이 될 수 없음.
		//조부모를 기준으로 현재 노드의 부모 노드가 왼 자식인지 오른 자식인지도 알아야 함.
		bool bIsMyParentisLeftChild = true;
		if (grandparent->pLeft == parent)
		{
			uncle = grandparent->pRight;
		}
		else
		{
			bIsMyParentisLeftChild = false;
			uncle = grandparent->pLeft;
		}

		//삼촌의 색이 black인 경우
		if (uncle->Color == BLACK)
		{
			//부모가 왼쪽 자식일 경우
			if (bIsMyParentisLeftChild)
			{
				//내가 왼쪽자식일 경우
				if (bIsMyPosisLeft)
				{
					//조부모 노드를 우회전 시킨다.
					makeRightRotate(grandparent);
					//부모의 색을 검은색으로 치환
					//조부모의 색을 빨간색으로 치환한다.
					ChangeColor(parent, BLACK);
					ChangeColor(grandparent, RED);
				}
				//오른쪽 자식일 경우
				else
				{
					//부모노드를 기준으로 좌회전
					makeLeftRotate(parent);
					//이후 다시 조부모 노드를 기준으로 우회전
					makeRightRotate(grandparent);
					//새로운 노드의 색은 검정이 되고
					ChangeColor(*curNode, BLACK);
					//조부모 노드의 색은 빨간색이 된다.
					ChangeColor(grandparent, RED);
				}
			}
			//부모가 오른쪽 자식일 경우
			else
			{
				//내가 왼쪽자식일 경우
				if (bIsMyPosisLeft)
				{
					//부모노드를 기준으로 우회전
					makeRightRotate(parent);
					//이후 다시 조부모 노드를 기준으로 좌회전
					makeLeftRotate(grandparent);
					//새로운 노드의 색은 검정이 되고
					ChangeColor(*curNode, BLACK);
					//조부모 노드의 색은 빨간색이 된다.
					ChangeColor(grandparent, RED);
				}
				//오른쪽 자식일 경우
				else
				{
					//조부모 노드를 좌회전 시킨다.
					makeLeftRotate(grandparent);
					//부모의 색을 검은색으로 치환
					//조부모의 색을 빨간색으로 치환한다.
					ChangeColor(parent, BLACK);
					ChangeColor(grandparent, RED);
				}
			}
		}
		//삼촌의 색이 Red인 경우
		else
		{
			//조부모는 빨간색
			ChangeColor(grandparent, RED);
			//부모와 삼촌은 검은색이 된다.
			ChangeColor(parent, BLACK);
			ChangeColor(uncle, BLACK);
			//이때 조부모의 부모노드가 R이면 다시 RR문제가 발생함.
			//해당 조건을 체크해준다.
			if (grandparent->pParent != nullptr)
			{
				MakeBalacingAfterInsert(&grandparent);
			}
		}

	}
	//이상한 값일 경우
	else
	{
		cout << "detacted worng num in parent's color" << "\n";
	}
	
}

void RBTree::makeRightRotate(stNODE* curNode)
{
	//부모, 왼쪽 자식, 왼쪽 자식의 오른쪽 자식을 구해준다.
	stNODE* parent = curNode->pParent;
	stNODE* lc = curNode->pLeft;
	stNODE* lrc = lc->pRight;
	//현재 노드의 왼쪽 자식이 현재 노드의 부모가 됨.
	//왼쪽 자식의 부모 노드는 현재 노드의 부모가 됨.
	//부모 입장에선 자식이 교체됨.
	lc->pParent = parent;
	curNode->pParent = lc;

	if (parent != nullptr)
	{
		if (parent->pLeft == curNode)
		{
		  parent->pLeft = lc;
		}
		else
		{
			parent->pRight = lc;
		}
	}
	else {
		//현재 회전하는 노드가 root 노드였다면, root 멤버 변수가 lc를 가리키게 만들어준다.
		root = lc;
	}

	//왼쪽 자식의 오른쪽 자식은,
	//현재 노드의 왼쪽 자식이 된다.
	curNode->pLeft = lrc;
	lrc->pParent = curNode;
	//현재 노드는 원래 왼쪽 자식의 오른쪽 자식이 됨.
	lc->pRight = curNode;
}

void RBTree::makeLeftRotate(stNODE* curNode)
{
	//현재 노드의 부모. 오른쪽 자식, 오른쪽 자식의 왼쪽 자식을 확보한다.
	stNODE* parent = curNode->pParent;
	stNODE* rc = curNode->pRight;
	stNODE* rlc = rc->pLeft;

	//현재 노드 오른쪽 자식의 부모가 현재 노드의 부모가 됨.
	rc->pParent = parent;
	//현재 노드의 오른쪽 자식이 현재 노드의 부모의 자식이 됨.
	curNode->pParent = rc;

	//현재 노드 부모의 자식이 현재 노드 오른쪽 자식이 됨.
	if (parent == nullptr)
	{
		//현재 회전하는 노드가 root 노드였다면, root 멤버 변수가 lc를 가리키게 만들어준다.
		root = rc;
	}
	else {
		if (parent->pLeft == curNode)
		{
			parent->pLeft = rc;
		}
		else
		{
			parent->pRight = rc;
		}
	}

	//오른쪽 자식의 왼 자식은,
	//현재 노드의 오른쪽 자식이 된다.
	curNode->pRight = rlc;
	rlc->pParent = curNode;
	//현재 노드는 오른쪽 자식의 왼 자식이 됨.
	rc->pLeft = curNode;
}

void RBTree::ChangeColor(stNODE* curNode, NODE_COLOR color)
{
	curNode->Color = color;
}

//삭제할 노드를 찾는 함수와 실제 삭제가 이뤄질 노드의 색에 따라 밸런싱을 취할 함수를 구분해야 할 듯.

void RBTree::RemoveData(stNODE** curNode, int d)
{
	//데이터를 찾아 지우자

	//삭제할 데이터 찾지 못한 경우 예외처리 내며 반환
	if (*curNode == &Nil)
	{
		cout << "--------없는 데이터 삭제 시도--------" << "\n";
		return;
	}


	if ((*curNode)->iData > d)
	{
		RemoveData(&((*curNode)->pLeft), d);
	}
	else if ((*curNode)->iData < d)
	{
		RemoveData(&((*curNode)->pRight), d);
	}
	
	//삭제할 데이터를 찾은 경우
	else
	{
		//해당 데이터의 자식 노드 유무에 따라 동작이 달라짐.
		   //자식이 없는 노드일 경우 걍 삭제
		if ((*curNode)->pLeft == &Nil && (*curNode)->pRight == &Nil)
		{
			//부모가 닐을 가리키도록 해줌.
			stNODE* parent = (*curNode)->pParent;
			if (parent->pLeft == *curNode)
			{
				parent->pLeft = &Nil;
			}
			else
			{
				parent->pRight = &Nil;
			}
			delete (*curNode);
			*curNode = nullptr;
			//삭제 후 벨런싱 작업
			MakeBalacingAfterRemove(curNode);
		}
		//자식이 하나인 경우
		//해당 자식과 부모를 연결 -> 삭제된 노드가 R이므로 부모와 자식은 반드시 B 
		//오른쪽 자식만 있는 경우
		else if ((*curNode)->pLeft == &Nil)
		{
			stNODE* right = (*curNode)->pRight;
			//부모는 자식을 가리키고
			//자식은 부모를 가리키도록 함.
			stNODE* parent = (*curNode)->pParent;
			if (parent->pLeft == *curNode)
			{
				parent->pLeft = right;
			}
			else
			{
				parent->pRight = right;
			}
			right->pParent = parent;

			MakeBalacingAfterRemove(curNode);
			delete (*curNode);
			*curNode = nullptr;
		}
		else if ((*curNode)->pRight == &Nil)
		{
			stNODE* left = (*curNode)->pRight;
			//부모는 자식을 가리키고
			//자식은 부모를 가리키도록 함.
			stNODE* parent = (*curNode)->pParent;
			if (parent->pLeft == *curNode)
			{
				parent->pLeft = left;
			}
			else
			{
				parent->pRight = left;
			}
			left->pParent = parent;

			MakeBalacingAfterRemove(curNode);
			delete (*curNode);
			*curNode = nullptr;
		}
		//자식이 2개있는 경우
		//해당 자리를 대체할 수 있는 노드를 찾아야 함.
		//왼쪽 자식의 가장 오른쪽 노드 or 오른쪽 자식의 가장 왼쪽 노드
		//왼-오 노드로 선택
		else {
			stNODE* LeftChild = (*curNode)->pLeft;
			while (LeftChild->pRight != &Nil)
			{
				stNODE* RightestChild = LeftChild->pRight;
				LeftChild = RightestChild;
			}
			//왼오 노드의 데이터를 현재 위치에 대입한 후,
			//왼오 노드는 삭제한다.
			(*curNode)->iData = LeftChild->iData;
			RemoveData(&(*curNode)->pLeft, LeftChild->iData);
		}
	}
}

//실제로 삭제될 노드의 색에 따라 밸런싱 작업을 함.
//근데 어차피 삭제될 노드의 색은 검은색일때만 밸런싱 작업이 필요함.
//삭제될 노드가 블랙이고 자식 노드 입장에서 원래 부모 위치 입장의 검은 노드가 사라진 것이므로
//해당하는 자식 노드를 인자로 넣자.
//어차피 삭제될 노드의 자식은 하나 이하임.
void RBTree::MakeBalacingAfterRemove(stNODE** curNode)
{
	//삭제할 노드가 Red인 경우
	//이미 삭제 함수에서 부모와 자식 연결등의 작업은 다 됨.
	//if ((*curNode)->Color == RED)
	//{
	//	(*curNode)->Color = BLACK;
	//}
	
	//밸런싱은 삭제할 노드가 검정색일때만 하면 됨.
	if ((*curNode)->Color == BLACK)
	{
		stNODE* child = &Nil;
		(*curNode)->pLeft;
		//실질적으로 삭제되는 노드의 자식은 하나이거나 없는 경우임.
		//만약 자식이 있고 레드라면 해당 자식을 블랙으로 만들어주기만 하면 됨.
		if ((*curNode)->pLeft != &Nil)
		{
			child = (*curNode)->pLeft;
		}
		else if ((*curNode)->pRight != &Nil)
		{
			child = (*curNode)->pRight;
		}
		stNODE* parent = (*curNode)->pParent;

		//자식이 red인 경우
		if (child->Color == RED)
		{
			ChangeColor(child, BLACK);
		}
		//형제가 레드인 경우
		//부모가 존재하는지 여부를 파악해야 할까?
		//루트 노드의 삭제가 이뤄지는지 파악하려면 그렇게 해야 할 듯
		else if (parent != nullptr)
		{
			//루트 노드만 아니라면 형제가 nill로라도 존재함.
			stNODE* siblian;
			if (parent->pLeft == *curNode)
			{
				siblian = parent->pRight;
			}
			else
			{
				siblian = parent->pLeft;
			}

			//형제가 Red인 경우
			if (siblian->Color == RED)
			{
				ChangeColor(siblian, BLACK);
				makeLeftRotate(parent);
				ChangeColor(parent, RED);
				//child를 기준으로 다시한번 밸런싱을 맞춘다.
				//child는 red가 아닌 경우를 지났으므로  child는 black
				//child를 기준으로 밸런싱을 다시 해준다.
				//child는 삭제된 노드는 아니지만 black인 경우 이중 블랙처럼 처리 할 수 있음.
				MakeBalacingAfterRemove(&child);
			}
			//형제가 Black인 경우
			else
			{
				//형제의 양쪽 자식이 블랙인 경우
				//형제 쪽에서 노드를 끌고오더라도 형제 쪽의 밸런싱이 무너짐
				//전체적인 블랙 노드의 개수를 1개 줄이는 방향으로 해야 함. 
				if (siblian->pLeft->Color == BLACK && siblian->pRight->Color == BLACK)
				{
					siblian->Color = RED;
				}

			}

		}
	}
}

void RBTree::inorderTraversal(vector<int>& vout, stNODE* curNode)
{
	if (curNode == &Nil) return;
	//curNode nullptr 오류
	inorderTraversal(vout, curNode->pLeft);
	cout << curNode->iData << " ";
	vout.push_back(curNode->iData);
	inorderTraversal(vout, curNode->pRight);
}

void RBTree::destroyTree(stNODE** curNode)
{
	if (*curNode == &Nil) return;

	destroyTree(&(*curNode)->pLeft);
	destroyTree(&(*curNode)->pRight);
	delete* curNode;
	*curNode = nullptr;
}

void RBTree::makeRootandNilBecomeBlack()
{
	root->Color = BLACK;
	Nil.Color = BLACK;
}

