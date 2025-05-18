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
		  filedata[*filesize + 1] = '/0';
		  fclose(f);
		  for (int i = 0; i < *filesize; i++)
		  {
			  printf("%c", filedata[i]);
		  }
	  }

	  bool CParser::GetNextWord(char** buf, int* length)
	  {
		  char** pfiledata;
		  if (buf == nullptr)
		  {
			  buf = &filedata;
		  }

		  pfiledata = buf;

		  if (isalpha(**pfiledata))
		  {

		 }
		  else if (isdigit(**pfiledata))
		  {

		  }
		  else if (isdigit(**pfiledata))
		  {

		  }
		  else
		  {

		  }

		  return false;
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