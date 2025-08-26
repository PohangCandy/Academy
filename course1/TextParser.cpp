#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE
#undef _UNICODE

#include <iostream>
#include <Windows.h>
using namespace std;

class CParser
{
	//int Version = 0;
	//int 	ServerID = 0;
	//char* ServerBindIP = nullptr;
	//int ServerBindPort = 0;
	//int WorkerThread = 0;
	//int MaxUser = 0;
	char* filedata = (char*)malloc(sizeof(char));
	int* filesize = (int*)malloc(sizeof(int));
	char* current = nullptr;

public: CParser() {}

	  void LoadFile(const char* filename);

	  bool GetNextWord(char** buf, int* length);

	  BOOL GetValue(const char* szName, int* ipValue);
};



	  void CParser::LoadFile(const char* filename)
	  {
		  FILE* f;
		  f = fopen(filename, "rb");

		  fseek(f, 0, SEEK_END);
		  *filesize = ftell(f);
		  fseek(f, 0, SEEK_SET);
		  //원본을 가리킬 버퍼와 문자를 저장할 버퍼
		  //저장한 문자를 담을 버퍼가 있어야함.
		  filedata = (char*)malloc(*filesize + 1);
		  char* buf = (char*)malloc(1);
		  for (int i = 0; i < *filesize; i++)
		  {
			  fread(buf, 1, 1, f);
			  filedata[i] = *buf;
		  }
		  filedata[*filesize] = '/0';
		  fclose(f);
		  for (int i = 0; i < *filesize; i++)
		  {
			  printf("%c", filedata[i]);
		  }

		  current = filedata;
	  }

	  //bool CParser::GetNextWord(char** buf, int* length)
	  //{
		 // char** pfiledata;
		 // *length = 0;
		 // if (buf == nullptr)
		 // {
			//  *buf = filedata;
		 // }

		 // pfiledata = buf;

		 // if (isalpha(**pfiledata))
		 // {
			//  while (isalpha(**pfiledata++))
			//  {
			//	  *length++;
			//  }
		 //}
		 // else if (isdigit(**pfiledata))
		 // {
			//  while (isdigit(**pfiledata++))
			//  {
			//	  *length++;
			//  }
		 // }
		 // else if (**pfiledata == '=')
		 // {

		 // }
		 // else
		 // {

		 // }

		 // return false;
	  //}

	  bool CParser::GetNextWord(char** buf, int* length)
	  {
		  // 공백 문자 건너뛰기
		  while (*current && isspace(*current))
		  {
			  current++;
		  }

		  if (*current == '\0') return false;

		  *buf = current;
		  *length = 0;

		  if (isalpha(*current)) {
			  while (*current != '\0' && isalpha(*current)) {
				  (*length)++;
				  current++;
			  }
		  }
		  else if (isdigit(*current)) {
			  while (*current != '\0' && isdigit(*current)) {
				  (*length)++;
				  current++;
			  }
		  }
		  else if (*current == '=') {
			  *length = 1;
			  current++;
		  }
		  else {
			  // 기타 문자 하나
			  *length = 1;
			  current++;
		  }

		  return true;
	  }


	  BOOL CParser::GetValue(const char* szName, int* ipValue)
	  {
		  //filedata 안에서 단어를 찾는다.
		 //찾은 단어를 저장할 버퍼
		  char* chpBuff, chWord[256];
		  int	iLength;
		  // 찾고자 하는 단어가 나올때까지 계속 찾을 것이므로 while 문으로 검사.
		  while (GetNextWord(&chpBuff, &iLength))
		  {
			  // Word 버퍼에 찾은 단어를 저장한다.
			  memset(chWord, 0, 256);
			  memcpy(chWord, chpBuff, iLength);
			  // 인자로 입력 받은 단어와 같은지 검사한다.
			  if (0 == strcmp(szName, chWord))
			  {
				  // 맞다면 바로 뒤에 = 을 찾자.
				  if (GetNextWord(&chpBuff, &iLength))
				  {
					  memset(chWord, 0, 256);
					  memcpy(chWord, chpBuff, iLength);
					  if (0 == strcmp(chWord, "="))
					  {
						  // = 다음의 데이터 부분을 얻자.
						  if (GetNextWord(&chpBuff, &iLength))
						  {
							  memset(chWord, 0, 256);
							  memcpy(chWord, chpBuff, iLength);
							  *ipValue = atoi(chWord);
							  return TRUE;
						  }
						  return FALSE;
					  }
				  }
				  return FALSE;
			  }
		  }
	  }

int main()
{
	CParser Parser;
	int iValue1, iValue2;
	Parser.LoadFile("test.txt");
	Parser.GetValue("Version", &iValue1);


	return 0;
}

//#define _CRT_SECURE_NO_WARNINGS
//#undef UNICODE
//#undef _UNICODE
//
//#include <iostream>
//#include <Windows.h>
//using namespace std;
//
//class CParser
//{
//    char* filedata = nullptr;
//    int filesize = 0;
//    char* current = nullptr;
//
//public:
//    CParser() {}
//    ~CParser() {
//        if (filedata) free(filedata);
//    }
//
//    void LoadFile(const char* filename);
//    bool GetNextWord(char** buf, int* length);
//    BOOL GetValue(const char* szName, int* ipValue);
//};
//
//void CParser::LoadFile(const char* filename)
//{
//    FILE* f = fopen(filename, "rb");
//    if (!f) {
//        cout << "파일 열기 실패" << endl;
//        return;
//    }
//
//    fseek(f, 0, SEEK_END);
//    filesize = ftell(f);
//    fseek(f, 0, SEEK_SET);
//
//    filedata = (char*)malloc(filesize + 1);  // +1 for null terminator
//    fread(filedata, 1, filesize, f);
//    filedata[filesize] = '\0';  // null terminator 추가
//
//    fclose(f);
//
//    printf("파일 내용:\n%s\n", filedata);  // 디버깅용 출력
//    current = filedata;
//}
//
//bool CParser::GetNextWord(char** buf, int* length)
//{
//    // 공백 문자 건너뛰기
//    while (*current && isspace(*current)) {
//        current++;
//    }
//
//    if (*current == '\0') return false;
//
//    *buf = current;
//    *length = 0;
//
//    if (isalpha(*current)) {
//        while (*current != '\0' && isalpha(*current)) {
//            (*length)++;
//            current++;
//        }
//    }
//    else if (isdigit(*current)) {
//        while (*current != '\0' && isdigit(*current)) {
//            (*length)++;
//            current++;
//        }
//    }
//    else if (*current == '=') {
//        *length = 1;
//        current++;
//    }
//    else {
//        *length = 1;
//        current++;
//    }
//
//    return true;
//}
//
//BOOL CParser::GetValue(const char* szName, int* ipValue)
//{
//    char* chpBuff, chWord[256];
//    int iLength;
//
//    while (GetNextWord(&chpBuff, &iLength))
//    {
//        memset(chWord, 0, 256);
//        memcpy(chWord, chpBuff, iLength);
//
//        if (strcmp(szName, chWord) == 0)
//        {
//            if (GetNextWord(&chpBuff, &iLength))
//            {
//                memset(chWord, 0, 256);
//                memcpy(chWord, chpBuff, iLength);
//
//                if (strcmp(chWord, "=") == 0)
//                {
//                    if (GetNextWord(&chpBuff, &iLength))
//                    {
//                        memset(chWord, 0, 256);
//                        memcpy(chWord, chpBuff, iLength);
//                        *ipValue = atoi(chWord);
//                        return TRUE;
//                    }
//                }
//            }
//            return FALSE;
//        }
//    }
//    return FALSE;
//}
//
//int main()
//{
//    CParser Parser;
//    int iValue1 = 0;
//
//    Parser.LoadFile("test.txt");
//
//    if (Parser.GetValue("Version", &iValue1))
//    {
//        cout << "Version 값: " << iValue1 << endl;
//    }
//    else
//    {
//        cout << "Version 키를 찾을 수 없습니다." << endl;
//    }
//
//    return 0;
//}
