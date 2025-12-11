#include "OVERLAPPED_CONTEXT.h"

#include <Windows.h>
OVERLAPPED_CONTEXT :: OVERLAPPED_CONTEXT(EIOCP_OPERATION operation)
	:op{ operation }
{
}

OVERLAPPED_CONTEXT::OVERLAPPED_CONTEXT()
	:op{ ENone }
{
}
