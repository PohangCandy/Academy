#include "RBTree.h"
#include <iostream>
using namespace std;

void RBTree::InsertData(stNODE** curNode, int d, stNODE* parent)
{

	if (d < 0 || d >= 10000) return;

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
		this->iSize++;
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
void RBTree::MakeBalacingAfterInsert(stNODE** pCurNode)
{
	stNODE* curNode = *pCurNode; // 편의상 포인터를 가져옴

	//부모가 없다면 밸런싱 필요없음.
	if (curNode->pParent == nullptr) return;

	//부모가 블랙이라면 밸런싱 할 필요없음.
	if (curNode->pParent->Color == BLACK) return;
	//부모가 레드일 경우
	else if (curNode->pParent->Color == RED)
	{
		//형제, 부모, 조부모, 삼촌을 확보한다.
		//없을 경우도 고려? -> ㄴㄴ
		//부모가 레드라는건 조부모는 무조건 블랙
		//삼촌은 닐 노드라도 존대할 것

		stNODE* parent = curNode->pParent;
		stNODE* sibling;
		//이때 내가 어느 위치의 자식으로 들어갈지도 알아두자.
		bool bIsMyPosisLeft = true;
		if (curNode == parent->pLeft)
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

					stNODE* temp = parent;
					parent = curNode;
					curNode = temp;

					makeRightRotate(grandparent);
					//새로운 노드의 색은 검정이 되고
					ChangeColor(parent, BLACK);
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

					// 포인터 갱신 (위와 동일)
					stNODE* temp = parent;
					parent = curNode;
					curNode = temp;

					//이후 다시 조부모 노드를 기준으로 좌회전
					makeLeftRotate(grandparent);
					//새로운 노드의 색은 검정이 되고
					ChangeColor(parent, BLACK);
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
				stNODE* newCurNode = grandparent;
				MakeBalacingAfterInsert(&newCurNode);
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

void RBTree::RemoveData(stNODE** pCurNode, int d)
{
	stNODE* z = *pCurNode;

	// 1. 삭제할 노드(z) 찾기
	while (z != &Nil && z->iData != d) {
		if (d < z->iData) {
			z = z->pLeft;
		}
		else {
			z = z->pRight;
		}
	}

	if (z == &Nil) {
		cout << "--------없는 데이터 삭제 시도--------" << "\n";
		return;
	}

	// 2. 실제로 삭제되거나 이동될 노드 y 결정
	stNODE* y = z;
	stNODE* x; // y를 대체할 노드
	NODE_COLOR y_original_color = y->Color;

	if (z->pLeft == &Nil) { // 자식이 0개 또는 1개(오른쪽만)
		x = z->pRight;
		RBTransplant(z, z->pRight);
	}
	else if (z->pRight == &Nil) { // 자식이 1개(왼쪽만)
		x = z->pLeft;
		RBTransplant(z, z->pLeft);
	}
	else { // 자식이 2개
		y = TreeMinimum(z->pRight); // 후속자 (successor)
		y_original_color = y->Color;
		x = y->pRight; // y는 왼쪽 자식이 없으므로 x는 항상 y의 오른쪽 자식(Nil 포함)

		// y의 원래 부모 저장 (y가 z의 바로 아래 자식인지 확인)
		stNODE* y_parent = y->pParent;

		// 1. y가 z의 바로 아래 자식이 아닐 경우에만 y의 원래 자리를 x로 교체 (RBTransplant)
		if (y_parent != z) {
			RBTransplant(y, y->pRight);
			y->pRight = z->pRight;
			y->pRight->pParent = y;
		}
		// y가 z의 바로 아래 자식일 때는 y의 자리를 x로 교체하는 Transplant를 수행하지 않음.

		// 2. z를 y로 교체 (y가 z의 자리를 차지)
		RBTransplant(z, y);
		y->pLeft = z->pLeft;
		y->pLeft->pParent = y;
		y->Color = z->Color;

		// 3. x의 부모 포인터 정리
		// y가 z의 바로 아래 자식일 때 (y_parent == z), x는 y의 자식으로 남아있으므로 x의 부모는 y가 되어야 함.
		// y가 z의 깊은 곳에 있을 때 (y_parent != z), x의 부모는 y의 원래 부모(y_parent)가 되어야 함.

		// y_parent != z 일 때, RBTransplant(y, y->pRight)에서 x->pParent는 이미 y_parent로 설정됨.
		// y_parent == z 일 때, x의 부모는 y가 되어야 함.
		if (y_parent == z && x != &Nil) {
			x->pParent = y; // y가 z의 자리를 차지했으므로 x의 부모를 y로 명확하게 설정
		}
		else if (x == &Nil) {
			// x가 Nil 노드일 때, Nil.pParent는 y의 원래 부모를 가리켜야 합니다. 
			// y_parent != z 일 때는 y_parent를, y_parent == z 일 때는 y를 가리켜야 합니다.
			// RBTransplant(z, y) 후 Nil.pParent가 잘못되었을 수 있으므로 이 블록은 필요합니다.
			Nil.pParent = (y_parent == z) ? y : y_parent;
		}
	}

	this->iSize--;

	// 3. 메모리 해제
	delete z;

	// 4. 밸런싱 (실제로 제거된 노드 y의 색이 Black이었을 경우에만)
	if (y_original_color == BLACK) {
		MakeBalacingAfterRemove(&x); // x는 이제 Double Black 상태의 시작점
	}

	// 5. 루트 노드 색상 정리 (makeRootandNilBecomeBlack에서 처리)
}

//실제로 삭제될 노드의 색에 따라 밸런싱 작업을 함.
//근데 어차피 삭제될 노드의 색은 검은색일때만 밸런싱 작업이 필요함.
//삭제될 노드가 블랙이고 자식 노드 입장에서 원래 부모 위치 입장의 검은 노드가 사라진 것이므로
//해당하는 자식 노드를 인자로 넣자.
//어차피 삭제될 노드의 자식은 하나 이하임.
void RBTree::MakeBalacingAfterRemove(stNODE** pX)
{
	stNODE* x = *pX;

	// x는 삭제된 Black 노드를 대체한 노드 (Nil 노드일 수도 있음)
	// x가 Double Black 상태이므로, x가 Red가 되거나 root가 될 때까지 반복
	while (x != root && x->Color == BLACK) {
		stNODE* parent = x->pParent;
		stNODE* w; // 형제 노드

		// x가 왼쪽 자식일 경우 (대칭 Case 1, 2, 3, 4)
		if (x == parent->pLeft) {
			w = parent->pRight;

			// Case 1: 형제 w가 RED인 경우
			if (w->Color == RED) {
				ChangeColor(w, BLACK);
				ChangeColor(parent, RED);
				makeLeftRotate(parent);
				w = parent->pRight; // 새로운 형제 지정
			}

			// Case 2: 형제 w가 BLACK이고 w의 두 자식이 모두 BLACK인 경우
			if (w->pLeft->Color == BLACK && w->pRight->Color == BLACK) {
				ChangeColor(w, RED);
				x = parent; // Double Black을 부모로 옮김
			}
			// Case 3 & 4: 형제 w가 BLACK이고 자식 중 하나가 RED인 경우
			else {
				// Case 3: 형제 w의 오른쪽 자식이 BLACK인 경우 (Zig-Zag)
				if (w->pRight->Color == BLACK) {
					//ChangeColor(w->pLeft, BLACK);
					ChangeColor(w, RED);
					makeRightRotate(w);
					w = parent->pRight; // 새로운 형제 지정
				}
				// Case 4: 형제 w의 오른쪽 자식이 RED인 경우 (Zig-Zig)
				w->Color = parent->Color;
				ChangeColor(parent, BLACK);
				ChangeColor(w->pRight, BLACK);
				makeLeftRotate(parent);
				x = root; // 밸런싱 종료 (루프 탈출)
			}
		}
		// x가 오른쪽 자식일 경우 (대칭 Case 1, 2, 3, 4)
		else {
			w = parent->pLeft;

			// Case 1: 형제 w가 RED인 경우
			if (w->Color == RED) {
				ChangeColor(w, BLACK);
				ChangeColor(parent, RED);
				makeRightRotate(parent);
				w = parent->pLeft; // 새로운 형제 지정
			}

			// Case 2: 형제 w가 BLACK이고 w의 두 자식이 모두 BLACK인 경우
			if (w->pLeft->Color == BLACK && w->pRight->Color == BLACK) {
				ChangeColor(w, RED);
				x = parent; // Double Black을 부모로 옮김
			}
			// Case 3 & 4: 형제 w가 BLACK이고 자식 중 하나가 RED인 경우
			else {
				// Case 3: 형제 w의 왼쪽 자식이 BLACK인 경우 (Zig-Zag)
				if (w->pLeft->Color == BLACK) {
					//ChangeColor(w->pRight, BLACK);
					ChangeColor(w, RED);
					makeLeftRotate(w);
					w = parent->pLeft; // // 새로운 형제 지정
				}
				// Case 4: 형제 w의 왼쪽 자식이 RED인 경우 (Zig-Zig)
				w->Color = parent->Color;
				ChangeColor(parent, BLACK);
				ChangeColor(w->pLeft, BLACK);
				makeRightRotate(parent);
				x = root; // 밸런싱 종료 (루프 탈출)
			}
		}
	}
	// x가 RED였다면 (Case 2로 인해 RED가 된 부모 등), BLACK으로 바꿈
	ChangeColor(x, BLACK);

	// 최종적으로 x를 MakeBalacingAfterRemove 호출 위치로 전달하기 위해 *pX 갱신
	*pX = x;
}

void RBTree::RBTransplant(stNODE* u, stNODE* v)
{
	if (u->pParent == nullptr) {
		root = v;
	}
	else if (u == u->pParent->pLeft) {
		u->pParent->pLeft = v;
	}
	else {
		u->pParent->pRight = v;
	}
	v->pParent = u->pParent;

}

stNODE* RBTree::TreeMinimum(stNODE* node)
{
	while (node->pLeft != &Nil) {
		node = node->pLeft;
	}
	return node;
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

// 1. 메인 검증 함수 (public)
bool RBTree::isRBTreeValid()
{
	// 규칙 1: 모든 노드는 Red 또는 Black이다. (stNODE 구조체에서 이미 보장)

	// 규칙 2: Root 노드는 Black이어야 한다.
	if (root != &Nil && root->Color != BLACK) {
		std::cout << "Validation Failed: Rule 2 (Root must be Black)\n";
		return false;
	}

	// 규칙 3: Nil 노드는 Black이어야 한다. (Nil은 이미 BLACK으로 초기화됨을 가정)
	if (Nil.Color != BLACK) {
		std::cout << "Validation Failed: Rule 3 (Nil must be Black)\n";
		return false;
	}

	// 규칙 4: Red 노드의 자식은 모두 Black이어야 한다. (재귀 함수에서 체크)
	// 규칙 5: 모든 경로의 Black Node 개수는 동일해야 한다. (checkBlackHeight에서 체크)

	// 규칙 0: BST 속성을 만족해야 한다.
	if (!checkBSTProperty(root, INT_MIN, INT_MAX)) {
		std::cout << "Validation Failed: Rule 0 (Must satisfy BST property)\n";
		return false;
	}

	// 규칙 5와 4 검사 시작
	int blackHeight = checkBlackHeight(root);
	if (blackHeight == -1) {
		// Black Height이 일관되지 않거나, Rule 4 (RR 위반)가 발생했을 경우
		return false;
	}

	// 모든 검증 통과
	return true;
}

void RBTree::clear()
{
	// 1. 재귀적으로 모든 노드의 메모리 해제
	// 이 호출이 끝나면 root는 nullptr이 됩니다.
	destroyTree(&root);

	// 2. 핵심: RBTree의 특징에 맞게 root를 Nil 노드로 재설정
	root = &Nil;

	// 3. iSize 초기화
	this->iSize = 0;
}

// 2. 이진 탐색 트리 속성 검증 함수 (private)
bool RBTree::checkBSTProperty(stNODE* node, int minVal, int maxVal)
{
	if (node == &Nil) {
		return true;
	}

	// 현재 노드의 값이 범위 내에 있는지 확인
	if (node->iData <= minVal || node->iData >= maxVal) {
		std::cout << "Validation Failed: BST Order Violated at " << node->iData << "\n";
		return false;
	}

	// 왼쪽 서브트리는 maxVal보다 작아야 함
	if (!checkBSTProperty(node->pLeft, minVal, node->iData)) {
		return false;
	}

	// 오른쪽 서브트리는 minVal보다 커야 함
	if (!checkBSTProperty(node->pRight, node->iData, maxVal)) {
		return false;
	}

	return true;
}

// 3. 블랙 깊이 및 Red-Red 규칙 검증 함수 (private)
// 반환값: 해당 노드까지의 Black Height. 불일치 또는 규칙 위반 시 -1 반환.
int RBTree::checkBlackHeight(stNODE* node)
{
	if (node == &Nil) {
		return 1; // Nil 노드는 Black이므로 Black Height은 1
	}

	// 규칙 4: Red 노드의 자식은 모두 Black이어야 한다 (RR 규칙 검사)
	if (node->Color == RED) {
		if (node->pLeft->Color == RED || node->pRight->Color == RED) {
			std::cout << "Validation Failed: Rule 4 (Red node has Red child) at " << node->iData << "\n";
			return -1;
		}
	}

	// 재귀적으로 왼쪽과 오른쪽 자식의 Black Height 계산
	int leftBH = checkBlackHeight(node->pLeft);
	int rightBH = checkBlackHeight(node->pRight);

	// 규칙 5: 왼쪽/오른쪽 서브트리의 Black Height이 일치해야 함
	if (leftBH == -1 || rightBH == -1) {
		return -1; // 이미 하위 트리에서 오류 발생
	}

	if (leftBH != rightBH) {
		std::cout << "Validation Failed: Rule 5 (Black Height mismatch) at " << node->iData
			<< ". Left BH: " << leftBH << ", Right BH: " << rightBH << "\n";
		return -1;
	}

	// 현재 노드가 Black이면 Height에 1을 더함
	// 현재 노드가 Red이면 Height을 그대로 반환
	return leftBH + (node->Color == BLACK ? 1 : 0);
}

void RBTree::makeRootandNilBecomeBlack()
{
	if (root != nullptr && root != &Nil) { // root가 Nil 노드를 가리킬 때도 예외 처리
		root->Color = BLACK;
	}
	Nil.Color = BLACK;
}

