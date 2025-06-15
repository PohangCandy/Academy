#pragma once

//1. iterator의 node 해제
// 어떻게 private 맴버 변수를 삭제해줄까
//get 역할을 하는 내부 함수를 새로 만들자.
//객체지향을 고려해서 iterator 맴버 함수로 node를 제거

//2. 리스트를 순회하면서 데이터 값을 찾아서 지우는 함수
//근데 데이터를 지우면 노드를 지우는 작업인데 iter 위치가 바뀌는게 맞나??
//중복된 데이터 모두 삭제하기 위함임.
//어차피 지역변수인 iter은 스택 소멸과 함께 사라짐.

//3. *iterator 연산자 오버로딩
// iterator의 노드 맴버 T _data 반환
//iterator 의 this 포인터는 &iterator 타입이므로 *this 하면 그냥 iterator타입

//4.소멸자만 정의되어 있고, 복사 생성자/대입 연산자가 없음 → 얕은 복사 시 문제 발생 가능


template <typename T>
class CList
{
public:
	struct Node
	{
		T  _data;
		Node* _Prev;
		Node* _Next;
	};

	class iterator
	{
	private:
		Node* _node;
	public:
		//인자로 들어온 Node 포인터를 저장
		iterator(Node* node = nullptr)
		{
			_node = node;
		}

		//------------------------------------------------
		// 동적 할당 받은 _node를 저장하고 있으므로
		// iter로 이를 해제하는 함수가 필요함.
		//------------------------------------------------
		void deleteNode()
		{
			_node->_Next->_Prev = _node->_Prev;
			_node->_Prev->_Next = _node->_Next;
			delete _node;
		}

		//전위 증가
		iterator& operator++()
		{
			_node = _node->_Next;
			return *this;
		}

		//후위 증가
		//현재 노드를 다음 노드로 이동
		iterator operator ++(int)
		{
			iterator temp = *this;
			++(*this);
			return temp;
		}

		iterator& operator--()
		{
			_node = _node->_Prev;
			return *this;
		}

		iterator operator--(int)
		{
			iterator temp = *this;
			--(*this);
			return temp;
		}

		//현재 노드의 데이터를 뽑음
		T& operator *()
		{
			return _node->_data;
		}

		//----------------------------------------------------
		// iterator를 복사 대입하는 = 연산자
		// 내부 노드 값 복사
		//----------------------------------------------------
		iterator& operator =(const iterator& other)
		{
			this->_node = other._node;
			return *this;
		}

		bool operator ==(const iterator& other)
		{
			return (this->_node == other._node);
		}
		bool operator !=(const iterator& other)
		{
			return !(*this == other);
		}
	};

public:
	CList()
	{
		_head._Next = &(_tail);
		_tail._Prev = &(_head);
	};
	~CList() 
	{
		//리스트 안에 새롭게 할당 받은 노드 지우기
		Node* cur = _head._Next;
		while (cur != &(_tail))
		{
			Node* next = cur->_Next;
			delete(cur);
			cur = next;
		}
	};

	//첫번째 데이터 노드를 가리키는 이터레이터 리턴
	iterator begin()
	{
		return (iterator::iterator(_head._Next));
	}

	/*	Tail 노드를 가리키는(데이터가 없는 진짜 더미 끝 노드) 이터레이터를 리턴
		또는 끝으로 인지할 수 있는 이터레이터를 리턴*/
	iterator end()
	{
		return (iterator::iterator(&_tail));
	}

	void push_front(T data)
	{
		Node* newNode = new Node;
		if (!newNode) return;
		newNode->_data = data;

		//항상 끝에 tail이 존재하므로 따로 널 체크 하지 않아도 될 듯
		Node* next = _head._Next;
		_head._Next = newNode;
		newNode->_Prev = &(_head);
		newNode->_Next = next;
		next->_Prev = newNode;
		_size++;
	}
	void push_back(T data)
	{
		Node* newNode = new Node;
		if (!newNode) return;
		newNode->_data = data;

		Node* prev = _tail._Prev;
		_tail._Prev = newNode;
		newNode->_Prev = prev;
		newNode->_Next = &(_tail);
		prev->_Next = newNode;
		_size++;
	}
	void pop_front()
	{
		if (_head._Next != &(_tail))
		{
			Node* next = _head._Next;
			_head._Next = next->_Next;
			_head._Next->_Prev = &(_head);
			delete(next);
			_size--;
		}
	}
	void pop_back()
	{
		if (_tail._Prev != &(_head))
		{
			Node* prev = _tail._Prev;
			_tail._Prev = prev->_Prev;
			_tail._Prev->_Next = &(_tail);
			delete(prev);
			_size--;
		}
	}
	void clear()
	{
		//리스트 안에 새롭게 할당 받은 노드 지우기
		Node* cur = _head._Next;
		while (cur != &(_tail))
		{
			Node* next = cur->_Next;
			delete(cur);
			cur = next;
		}

		_head._Next = &(_tail);
		_tail._Prev = &(_head);
		_size = 0;
	}
	int size() { return _size; };
	bool empty() { return _head._Next == &(_tail); };


	/*- 이터레이터의 그 노드를 지움.
		- 그리고 지운 노드의 다음 노드를 카리키는 이터레이터 리턴*/
	iterator erase(iterator iter)
	{
		iterator cur = iter;
		iterator next = ++iter;
		cur.deleteNode();
		return next;
	}

	//----------------------------------------------------------
	//리스트를 순회하면서 데이터 값을 찾아서 지우는 함수
	//근데 데이터를 지우면 노드를 지우는 작업인데 iter 위치가 바뀌는게 맞나??
	//----------------------------------------------------------
	void remove(T Data)
	{
		CList<T>::iterator iter;
		for (iter = this->begin(); iter != this->end();)
		{
			if (*iter == Data)
			{
				iter = erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

private:
	int _size = 0;
	Node _head;
	Node _tail;
};

/////////////////// 순회 샘플 코드 /////////////////////////
//
//CList<int> ListInt;
//
//
//CList<int>::iterator iter;
//for (iter = ListInt.begin(); iter != ListInt.end(); ++iter)
//{
//	printf("%d", *iter);
//}
//
//
//
//
//
//
//
/////////////////// 객체 삭제 샘플 코드 /////////////////////////
//
//list.begin()    Head 다음 첫 요소의 iter
//list.end()      더미 Tail iter
//
//
//CList<CPlayer*> ListPlayer;
//ListPlayer.push_back(new CPlayer);
//ListPlayer.push_back(new CPlayer);
//
//
//for (CList<CPlayer*>::iterator iter = List.begin(); iter != List.end(); ++iter)
//{
//	CPlayer* p = *iter;
//}
//
//
//
//CList<CPlayer*>::iterator iter;
//
//for (iter = List.begin(); iter != List.end(); )
//{
//	data = *iter;
//
//	if ((*iter)->GetID() == 삭제대상)
//	{
//		delete* iter;
//		iter = List.erase(iter);
//	}
//	else
//	{
//		++iter;
//	}
//}

///////////////////////////////////////////////////////////////////////////////

//#pragma once
//
////1. iterator의 node 해제
//// 어떻게 private 맴버 변수를 삭제해줄까
////get 역할을 하는 내부 함수를 새로 만들자.
////객체지향을 고려해서 iterator 맴버 함수로 node를 제거
//
////2. 리스트를 순회하면서 데이터 값을 찾아서 지우는 함수
////근데 데이터를 지우면 노드를 지우는 작업인데 iter 위치가 바뀌는게 맞나??
////중복된 데이터 모두 삭제하기 위함임.
////어차피 지역변수인 iter은 스택 소멸과 함께 사라짐.
//
////3. *iterator 연산자 오버로딩
//// iterator의 노드 맴버 T _data 반환
////iterator 의 this 포인터는 &iterator 타입이므로 *this 하면 그냥 iterator타입
//
////4.얕은 복사 시 문제 발생 가능
//// 일단 생성자랑 소멸자에서 특별히 복사 생성자 해주지 않고 있음.
//// iterator 대입 연산자를 만들었는데 
//// 노드를 그냥 복사하고 있어서 얕은 복사 문제가 발생하고 있음.
//// 노드의 내용만 복사되게 만들어줘야함. 
//// 근데 결국엔 복사 생성자를 처리해줘야함.
//// 초기화하면서 생성하는 경우를 대비해줘야함.
////friend 함수를 통해서 private 맴버에 접근 할 수 있도록 해줌.
////friend 함수 선언
////1. 먼저 friend 함수가 될 함수의 시그니쳐 전방에 선언
////2. private 맴버에 접근하고 싶은 클래스에 friend 함수 선언
////3. 클래스 외부에 해당 함수 정의
//
//
//
//template <typename T>
//class CList
//{
//public:
//	struct Node
//	{
//		T  _data;
//		Node* _Prev;
//		Node* _Next;
//	};
//
//
//	class iterator;
//	iterator erase(iterator iter);
//
//	class iterator
//	{
//
//	//node에 접근하기 위해 friend 함수 사용
//	friend iterator CList<T>::erase(iterator iter);
//
//	private:
//		Node* _node;
//	public:
//
//		//인자로 들어온 Node 포인터를 저장
//		iterator(Node* node = nullptr)
//		{
//			_node = node;
//		}
//
//		//전위 증가
//		iterator& operator++()
//		{
//			_node = _node->_Next;
//			return *this;
//		}
//
//		//후위 증가
//		//현재 노드를 다음 노드로 이동
//		iterator operator ++(int)
//		{
//			iterator temp = *this;
//			++(*this);
//			return temp;
//		}
//
//		iterator& operator--()
//		{
//			_node = _node->_Prev;
//			return *this;
//		}
//
//		iterator operator--(int)
//		{
//			iterator temp = *this;
//			--(*this);
//			return temp;
//		}
//
//		//현재 노드의 데이터를 뽑음
//		T& operator *()
//		{
//			return _node->_data;
//		}
//
//		//----------------------------------------------------
//		// iterator를 복사 대입하는 = 연산자
//		// 내부 노드 값 복사
//		//----------------------------------------------------
//		iterator& operator =(const iterator& other)
//		{
//			//그냥 포인터 복사해서 넘겨주셈
//			this->_node = other._node;
//
//			return *this;
//		}
//
//		bool operator ==(const iterator& other)
//		{
//			return (this->_node == other._node);
//		}
//		bool operator !=(const iterator& other)
//		{
//			return !(*this == other);
//		}
//	};
//
//public:
//	CList()
//	{
//		_head._Next = &(_tail);
//		_tail._Prev = &(_head);
//	};
//	~CList() 
//	{
//		//리스트 안에 새롭게 할당 받은 노드 지우기
//		Node* cur = _head._Next;
//		while (cur != &(_tail))
//		{
//			Node* next = cur->_Next;
//			delete(cur);
//			cur = next;
//		}
//	};
//
//	//첫번째 데이터 노드를 가리키는 이터레이터 리턴
//	iterator begin()
//	{
//		return (iterator::iterator(_head._Next));
//	}
//
//	/*	Tail 노드를 가리키는(데이터가 없는 진짜 더미 끝 노드) 이터레이터를 리턴
//		또는 끝으로 인지할 수 있는 이터레이터를 리턴*/
//	iterator end()
//	{
//		return (iterator::iterator(&_tail));
//	}
//
//	void push_front(T data)
//	{
//		Node* newNode = new Node;
//		if (!newNode) return;
//		newNode->_data = data;
//
//		//항상 끝에 tail이 존재하므로 따로 널 체크 하지 않아도 될 듯
//		Node* next = _head._Next;
//		_head._Next = newNode;
//		newNode->_Prev = &(_head);
//		newNode->_Next = next;
//		next->_Prev = newNode;
//		_size++;
//	}
//	void push_back(T data)
//	{
//		Node* newNode = new Node;
//		if (!newNode) return;
//		newNode->_data = data;
//
//		Node* prev = _tail._Prev;
//		_tail._Prev = newNode;
//		newNode->_Prev = prev;
//		newNode->_Next = &(_tail);
//		prev->_Next = newNode;
//		_size++;
//	}
//	void pop_front()
//	{
//		if (_head._Next != &(_tail))
//		{
//			Node* next = _head._Next;
//			_head._Next = next->_Next;
//			_head._Next->_Prev = &(_head);
//			delete(next);
//			_size--;
//		}
//	}
//	void pop_back()
//	{
//		if (_tail._Prev != &(_head))
//		{
//			Node* prev = _tail._Prev;
//			_tail._Prev = prev->_Prev;
//			_tail._Prev->_Next = &(_tail);
//			delete(prev);
//			_size--;
//		}
//	}
//	void clear()
//	{
//		//리스트 안에 새롭게 할당 받은 노드 지우기
//		Node* cur = _head._Next;
//		while (cur != &(_tail))
//		{
//			Node* next = cur->_Next;
//			delete(cur);
//			cur = next;
//		}
//
//		_head._Next = &(_tail);
//		_tail._Prev = &(_head);
//		_size = 0;
//	}
//	int size() { return _size; };
//	bool empty() { return _head._Next == &(_tail); };
//
//	//----------------------------------------------------------
//	//리스트를 순회하면서 데이터 값을 찾아서 지우는 함수
//	//근데 데이터를 지우면 노드를 지우는 작업인데 iter 위치가 바뀌는게 맞나??
//	//----------------------------------------------------------
//	void remove(T Data)
//	{
//		CList<T>::iterator iter;
//		for (iter = this->begin(); iter != this->end();)
//		{
//			if (*iter == Data)
//			{
//				iter = erase(iter);
//			}
//			else
//			{
//				++iter;
//			}
//		}
//	}
//
//private:
//	int _size = 0;
//	Node _head;
//	Node _tail;
//};
//
//template<typename T>
//inline typename CList<T>::iterator CList<T>::erase(iterator iter)
//{
//	iterator cur = iter;
//	iterator next = ++iter;
//
//	Node* nodeToDelete = cur._node;
//	nodeToDelete->_Next->_Prev = nodeToDelete->_Prev;
//	nodeToDelete->_Prev->_Next = nodeToDelete->_Next;
//	delete nodeToDelete;
//	return next;
//}
//
//
//
///*- 이터레이터의 그 노드를 지움.
//	- 그리고 지운 노드의 다음 노드를 카리키는 이터레이터 리턴*/
////template<typename T>
////typename CList<T>::iterator CList<T>::erase(iterator iter)
////{
////	iterator cur = iter;
////	iterator next = ++iter;
////	delete(cur._node);
////	//cur.deleteNode();
////	return next;
////}
//
//
//













