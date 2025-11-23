#pragma once
class Test {
public:
	Test() {
		std::cout << "Test 생성" << "\n";
	}
	int a = 0;
};

void addTestArray();

//-------------------
// 헤더 파일 vs 소스 파일 vs 소스파일 static 비교
//-------------------

//static Test t[10];