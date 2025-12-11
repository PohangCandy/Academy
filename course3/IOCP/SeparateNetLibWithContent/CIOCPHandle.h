#pragma once

typedef void* HANDLE;

//------------------------------------------
// 모든 IOCP의 핸들 정보를 가지고 있는 전역 싱글톤 객체
//------------------------------------------
class IOCPHandle
{
public:
	static IOCPHandle* GetIOCPHandleInstance()
	{
		if (IOCPHandleInstance == nullptr)
		{
			IOCPHandleInstance = new IOCPHandle;
		}
		return IOCPHandleInstance;
	}

	HANDLE netHcp = {};
	HANDLE contentHcp = {};
private:
	static IOCPHandle* IOCPHandleInstance;

	IOCPHandle() {};
	~IOCPHandle() {};

};
IOCPHandle* IOCPHandle::IOCPHandleInstance = nullptr;