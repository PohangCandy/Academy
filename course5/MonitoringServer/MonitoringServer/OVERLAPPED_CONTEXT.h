#pragma once
#include "stdafx.h"

enum EIOCP_OPERATION
{
	ENone,
	ERecv,
	ESend,
};

struct OVERLAPPED_CONTEXT {
	OVERLAPPED Overlapped = {};
	EIOCP_OPERATION op;

	OVERLAPPED_CONTEXT();
	OVERLAPPED_CONTEXT(EIOCP_OPERATION operation);
};
