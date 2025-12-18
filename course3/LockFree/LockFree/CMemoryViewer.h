//---------------------------------------------------------------------------------------------
// 프로젝트명: 시작 주소로부터 일정한 크기 간격만큼을 담아줄 자료구조
// 
// 목적: 멀티스레드에 의해 시시각각 변하는 자료구조의 흔적을 남기기위한 객체
// 
// 방법 : 시작 주소로 부터 3MB크기의 메모리를 copy한 후 담아둔다.
// 3MB 공간 2개를 활용해 스택의 이전 상태와 현재 상태를 보관한다.
// 
// 결론 : 
// 
// 
//---------------------------------------------------------------------------------------------

#pragma once

enum FuncName {
	epush,
	epop,
};

class CMemoryViewer {
public:
	CMemoryViewer();
	~CMemoryViewer();
	void copy(char* src1, int size1, char* src2, int size2, FuncName f);
private:
	char* buf[2];
	bool recentMem = 0;
};