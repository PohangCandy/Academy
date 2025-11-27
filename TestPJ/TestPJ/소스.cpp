//-----------------------------------------------
// 프로젝트 명 : string 객체를 대상으로 memcpy, strcpy_s하기
// 
// 방법 : strcpy_s, memcpy를 이용해 15바이트 이하, 15 바이트 초과하는 문자열을 복사하는 경우 결과 관찰
// 
// 결론 :
// string 객체는 문자열을 저장하는 공간과 메타 데이터 정보를 가짐.
// 이때 문자열을 저장하는 공간의 최대 크기는 15바이트.
// 이를 초과하는 문자열의 저장은 4바이트 주소를 저장해 참조하는 방식
// 
// 그렇기에 string에 문자열을 저장하려면 
// 반드시 string의 문자열 복사,대입 방식을 따라줘야 함.
// ( =연산자 , assign()매서드)
//
// 기존의 memcpy,strcpy_s를 string 객체에 사용하게되면
// 15바이트 문자열 이하에 대해 우연히 문자열이 잘 복사된 것처럼 보일 수 있으나
// 이는 잘못된 방식임.
// 이마저도 Debug 빌드에선 string의 가장 상단에 안전장치를 두어,
// string 객체의 주소로 memcpy가 시도되면 무조건 에러남.
//-----------------------------------------------

#include <iostream>
using namespace std;


//----------------------------------------
// 15byte이하
//----------------------------------------
string s1 = "s1sasdqwds1sasd";
string s2 = "s2sasdqwds1sasd";
//char c1[] = "c1sasdqwds1sasd";
char c2[] = "c2sasdqwds1sasd";
//----------------------------------------
// 15byte 초과
//----------------------------------------
//string s1 = "s1sasdqwds1sasds";
//string s2 = "s2sasdqwds1sasds";
//char c1[] = "c1sasdqwds1sasds";
//char c2[] = "c2sasdqwds1sasds";
//----------------------------------------
// 24byte
//----------------------------------------
//string s1 = "s1sasdqwdsc1sasdqwdsqwer";
//string s2 = "s2sasdqwds1sasds";
char c1[] = "c1sasdqwdsc1sasdqwdsqwera";
//char c2[] = "c2sasdqwds1sasds";

int main()
{
	//======================
	// memcpy
	//======================
	
	// ------------------------------------
	//char[]에 char[] memcpy
	//-----------------------------------
	//memcpy((char*)&c1, (char*)&c2, sizeof(c1));
	//cout << c1;
	
	//-----------------------------------
	//char[]에 string memcpy
	//-----------------------------------
	//memcpy((char*)&c1, (char*)&s2, sizeof(s2));
	//cout << c1;
	
    //-----------------------------------
	//string에 char[] memcpy
	//-----------------------------------
	//memcpy((char*)&s1, (char*)c1, sizeof(s1)); //-> 빌드 에러
	memcpy((char*)&s1, (char*)c1, s1.size());
	//s1 = c1;
	cout << s1;
	
	//-----------------------------------
	//string에 string memcpy
	//-----------------------------------
	//memcpy((char*)&s1, (char*)&s2, sizeof(s1)); 
	 //memcpy((char*)&s1, (char*)&s2, s2.size());
	 //cout << s1;
    //-----------------------------------
	

	//==========================
	// strcpy_s
	//==========================
	
	// ------------------------------------
	//char[]에 char[] strcpy_s
	//-----------------------------------
	//strcpy_s((char*)&c1, sizeof(c1), (char*)&c2);
	//cout << c1;

	//-----------------------------------
	//char[]에 string strcpy_s
	//-----------------------------------
	//strcpy_s((char*)&c1,sizeof(c1), (char*)&s2);
	//cout << c1;

	//-----------------------------------
	//string에 char[] strcpy_s
	//-----------------------------------
	////strcpy_s((char*)&s1, s1.size(), (char*)c1); //-> 빌드 에러
	//strcpy_s((char*)&s1, sizeof(s1), (char*)c1);
	// cout << s1;
	//-----------------------------------
	//string에 string strcpy_s
	//-----------------------------------
	//s1 += 'a';
	//s2 += 'a';
	 //strcpy_s((char*)&s1, sizeof(s1), (char*)&s2);
	 //cout << s1 << "\n";
	 //cout << s2 << "\n";
	//-----------------------------------
	
	return 0;
}