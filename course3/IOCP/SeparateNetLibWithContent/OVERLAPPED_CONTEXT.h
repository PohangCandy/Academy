#pragma once
#include "stdafx.h"

//-------------------------------
// 비동기 입출력 함수 종류
//-------------------------------
enum EIOCP_OPERATION
{
	ENone,
	ERecv,
	ESend,
	//EContents,
};


//-----------------------------------
// 완료된 비동기 함수를 나타내는 확장된 Overlapped
//-----------------------------------
struct OVERLAPPED_CONTEXT {
	OVERLAPPED Overlapped = {};
	EIOCP_OPERATION op;

	OVERLAPPED_CONTEXT();

	OVERLAPPED_CONTEXT(EIOCP_OPERATION operation);

};