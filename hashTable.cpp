//hash table
//1. id 추가
//2. 전체 id 보기

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//해쉬 테이블
//리스트 배열

struct Node {
	int data; //id
	Node* next;
};

struct List {
	Node* head;
};

//리스트 초기화
void initList(List* list);
//리스트 오르쪽에 추가
void InsertRightList(Node* n, int d);
//리스트 추가
void AppendList(List* list, int d);
//리스트 배열의 모든 값 확인하기
void showAllList(List* al, int alSize);
//리스트의 모든 노드 값 확인하기
void showListAll(List* al);
//모든 리스트 해제시키기
void freeList(List* a);
//리스트 배열 해제시키기
void freeAllList(List* a , int alSize);



void main()
{
	List al[5];
	int alLength = sizeof(al) / sizeof(al[0]);
	for (int i = 0; i < alLength;i++)
	{
		initList(&al[i]);
	}

	/*for (int i = 0; i < alLength;i++)
	{
		AppendList(&al[i],i);
	}

	showAllList(al, alLength);*/
	int id;
	while (1)
	{
		printf("정수 ID를 입력하시오 : \n");
		scanf("%d", &id);
		int hash = id % 5;

		switch (hash)
		{
		case 0:
			AppendList(&al[0],id);
			break;
		case 1:
			AppendList(&al[1], id);
			break;
		case 2:
			AppendList(&al[2], id);
			break;
		case 3:
			AppendList(&al[3], id);
			break;
		case 4:
			AppendList(&al[4], id);
			break;
		}

		showAllList(al, alLength);

	}

	freeAllList(al, alLength);
}

//리스트 배열 해제시키기
void freeAllList(List* al, int alSize)
{
	for (int i = 0; i < alSize;i++)
	{
		freeList(&al[i]);
	}
}

//모든 리스트 해제시키기
void freeList(List* l)
{
	Node* h = l->head;
	if (h != nullptr)
	{
		for (h = h->next;h;)
		{
			Node* next = h->next;
			free(h);
			h = next;
		}
		free(h);
	}
}

//리스트의 모든 노드 값 확인하기
void showListAll(List* al)
{
	Node* h = al->head;
	for (h = h->next; h;h = h->next)
	{
		printf("%d", h->data);
		printf(" >>>> ");
	}
}

//리스트 배열의 모든 값 확인하기
void showAllList(List* al, int alSize)
{
	for (int i = 0; i < alSize;i++)
	{
		printf("%d : ",i);
		showListAll(&al[i]);
		printf("\n");
	}
}

//리스트 초기화
void initList(List* list)
{
	//list = (List*)malloc(sizeof(List));
	
	//do {
	//	if (list == nullptr) break;
	//	list->head = (Node*)malloc(sizeof(Node));
	//	Node* h = list->head;

	//	if (h == nullptr) break;
	//	h->data = -1;
	//	h->next = nullptr;
	//	return;
	//} while (0);

	//printf("-----initList_error------n\n");


	if (list != nullptr)
	{
		list->head = (Node*)malloc(sizeof(Node));
		Node* h = list->head;
		if (h == nullptr)
		{
			printf("-----initList_error------n\n");
		}
		else
		{
			h->data = -1;
			h->next = nullptr;
		}
	}
	
}
//리스트 오른쪽에 삽입
void InsertRightList(Node* n, int d)
{
	Node* newNode = (Node*)malloc(sizeof(Node));
	if (newNode != nullptr)
	{
		newNode->data = d;
		newNode->next = n->next;
		n->next = newNode;
	}
	else
	{
		printf("-----InsertRightList_error------n\n");
	}
}

//리스트 가장 뒤쪽에 삽입
void AppendList(List* list, int d)
{
	Node* h;
	for(h = list->head;h->next;h = h->next){}
	InsertRightList(h, d);
}