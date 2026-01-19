#include "OVERLAPPED_CONTEXT.h"


OVERLAPPED_CONTEXT :: OVERLAPPED_CONTEXT(EIOCP_OPERATION operation)
	:op{ operation }
{
	//ZeroMemory(&Overlapped, 0);
}

OVERLAPPED_CONTEXT::OVERLAPPED_CONTEXT()
	:op{ ENone }
{
	//ZeroMemory(&Overlapped, 0);
}
