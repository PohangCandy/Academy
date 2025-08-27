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
		if (grandparent->pLeft == parent)
		{
			uncle = grandparent->pRight;
		}
		else
		{
			uncle = grandparent->pLeft;
		}

		//삼촌의 색이 black인 경우
		if (uncle->Color == BLACK)
		{
			//내가 왼쪽자식일 경우
			if (bIsMyPosisLeft)
			{
				//조부모 노드를 우회전 시킨다.
				makeRightRotate(&grandparent);
				//부모의 색을 검은색으로 치환
				//조부모의 색을 빨간색으로 치환한다.
				ChangeColor(&parent, BLACK);
				ChangeColor(&grandparent, RED);
			}
			//오른쪽 자식일 경우
			else
			{
				//부모노드를 기준으로 좌회전
				makeLeftRotate(&parent);
				//이후 다시 조부모 노드를 기준으로 우회전
				makeRightRotate(&grandparent);
				//새로운 노드의 색은 검정이 되고
				ChangeColor(curNode, BLACK);
				//조부모 노드의 색은 빨간색이 된다.
				ChangeColor(&grandparent, RED);
			}
		}
		//삼촌의 색이 Red인 경우
		else
		{
			//조부모는 빨간색
			ChangeColor(&grandparent, RED);
			//부모와 삼촌은 검은색이 된다.
			ChangeColor(&parent, BLACK);
			ChangeColor(&uncle, BLACK);
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

void RBTree::makeRightRotate(stNODE** curNode)
{
	//부모, 왼쪽 자식, 왼쪽 자식의 오른쪽 자식을 구해준다.
	stNODE** parent = &(*curNode)->pParent;
	stNODE** lc = &(*curNode)->pLeft;
	stNODE** lrc = &(*lc)->pRight;
	//현재 노드의 왼쪽 자식이 현재 노드의 부모가 됨.
	//왼쪽 자식의 부모 노드는 현재 노드의 부모가 됨.
	//부모 입장에선 자식이 교체됨.
	(*curNode)->pParent = *lc;
	(*lc)->pParent = *parent;
	if (parent != nullptr)
	{
		if ((*parent)->pLeft == (*curNode))
		{
			(*parent)->pLeft = *lc;
		}
		else
		{
			(*parent)->pRight = *lc;
		}
	}
	//왼쪽 자식의 오른쪽 자식은,
	//현재 노드의 왼쪽 자식이 된다.
	(*curNode)->pLeft = *lrc;
	(*lrc)->pParent = *curNode;
	//현재 노드는 원래 왼쪽 자식의 오른쪽 자식이 됨.
	(*lc)->pRight = *curNode;
}

void RBTree::makeLeftRotate(stNODE** curNode)
{
	//현재 노드의 부모. 오른쪽 자식, 오른쪽 자식의 왼쪽 자식을 확보한다.



	//현재 노드의 왼쪽 자식이 현재 노드의 부모 자식이 됨.
	//현재노드의 부모의 자식이 현재 노드 왼쪽 자식이 됨.
	//현재 노드 왼쪽 자식의 부모가 현재 노드의 부모가 됨.

	//오른쪽 자식의 왼쪽 자식이 현재 노드의 오른쪽 자식이 되고
	//오른쪽 자식의 부모가 현재 노드가 됨.
	
	//오른쪽 자식의 왼쪽 자식이 현재 노드가 됨. 
}

void RBTree::ChangeColor(stNODE** curNode, NODE_COLOR color)
{
	(*curNode)->Color = color;
}

void RBTree::RemoveData(stNODE** curNode, int d)
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
			if ((*curNode)->pLeft == nullptr && (*curNode)->pRight == nullptr)
			{
				delete (*curNode);
				*curNode = nullptr;
			}
			else if ((*curNode)->pLeft == nullptr)
			{
				stNODE* tmp = *curNode;
				*curNode = (*curNode)->pRight;
				delete(tmp);
				tmp = nullptr;
			}
			else if ((*curNode)->pRight == nullptr)
			{
				stNODE* tmp = *curNode;
				*curNode = (*curNode)->pLeft;
				delete(tmp);
				tmp = nullptr;
			}
			//자식이 2개있는 경우
			//해당 자리를 대체할 수 있는 노드를 찾아야 함.
			//왼쪽 자식의 가장 오른쪽 노드 or 오른쪽 자식의 가장 왼쪽 노드
			//왼-오 노드로 선택
			else {
				stNODE* LeftChild = (*curNode)->pLeft;
				while (LeftChild->pRight != nullptr)
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

}

void RBTree::inorderTraversal(vector<int>& vout, stNODE* curNode)
{
	if (curNode == nullptr) return;

	inorderTraversal(vout, curNode->pLeft);
	cout << curNode->iData << " ";
	vout.push_back(curNode->iData);
	inorderTraversal(vout, curNode->pRight);
}

void RBTree::destroyTree(stNODE** curNode)
{
	if (*curNode == nullptr) return;

	destroyTree(&(*curNode)->pLeft);
	destroyTree(&(*curNode)->pRight);
	delete* curNode;
	*curNode = nullptr;
}

