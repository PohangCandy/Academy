#include "CMemoryViewer.h"
#include "stdafx.h"

#define BUFSIZE (1024)

CMemoryViewer::CMemoryViewer()
{
	buf[0] = new char[BUFSIZE];
	buf[1] = new char[BUFSIZE];
}

CMemoryViewer::~CMemoryViewer()
{
	delete[] buf[0];
	delete[] buf[1];
}

void CMemoryViewer::copy(char* src1, int size1, char* src2, int size2, FuncName f)
{
	if (src1 == nullptr)
	{
		memset(buf[recentMem], 0, size1);
	}
	else
	{
		memcpy(buf[recentMem], &src1, size1);
	}
	
	if (src2 == nullptr)
	{
		memset(buf[recentMem] + size1, 0, size2);
	}
	else
	{
		memcpy(buf[recentMem] + size1, &src2, size2);
	}

	memcpy(buf[recentMem] + size1 + size2, &f, sizeof(FuncName));

	recentMem = !recentMem;
}
