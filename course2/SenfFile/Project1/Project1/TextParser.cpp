//TextParser.cpp
#include "TextParser.h"

CParser::~CParser()
{
	free(filedata);
}

bool CParser::LoadFile(const WCHAR* filename)
	  {
		  FILE* f;
		  
		  //f = fopen(filename, "rb");
		  f = _wfopen(filename, L"rb");
		  if (!f)
		  {
			  printf("%s 파일 열기 실패\n", filename);
			  return false;
		  }

		  fseek(f, 0, SEEK_END);
		  filesize = ftell(f);
		  fseek(f, 0, SEEK_SET);

		  if (filesize < 0)
		  {
			  printf("%s 파일 사이즈 읽기 실패\n", filename);
			  return false;
		  }

		  //원본을 가리킬 버퍼와 문자를 저장할 버퍼
		  //저장한 문자를 담을 버퍼가 있어야함.
		  filedata = (char*)malloc(filesize + 1);
		  if (!filedata)
		  {
			  fclose(f);
			  return false;
		  }

		  fread(filedata, 1, filesize, f);
		  filedata[filesize] = '\0';
		  fclose(f);

		  //탐색을 시작할 커서를 파일 데이터 가장 앞으로 초기화
		  current = filedata;

		  //for (int i = 0; i < *filesize; i++)
		  //{
			 // printf("%c", filedata[i]);
		  //}
		  return true;
	  }

	  bool CParser::RemoveSpace()
	  {
		  //커서가 가리키는 단어가 널 문자일 경우
		  if (!IsValid()) return false;
		  // 공백 문자 건너뛰기
		  while (IsValid() && isspace(*current))
		  {
			  current++;
		  }
		  if (!IsValid()) return false;

		  return true;
	  }

	  bool CParser::GetNextWord(char** buf, int* length)
	  {
		  if (!RemoveSpace()) return false;

		  ////커서가 가리키는 단어가 널 문자일 경우
		  //if (!IsValid()) return false;
		  //// 공백 문자 건너뛰기
		  //while (IsValid() && isspace(*current))
		  //{
			 // current++;
		  //}
		  //if (!IsValid()) return false;

		  *buf = current;
		  *length = 0;

		  bool bIsSymbol = true;

		  do {
			  if (*current == '=') {
				  break;
			  }
			  else if (*current == '(') {
				  break;
			  }
			  else if (*current == ')') {
				  break;
			  }
			  else if (*current == ',') {
				  break;
			  }

			  bIsSymbol = false;
		  } while (0);
		
		  if (bIsSymbol)
		 {
			  *length = 1;
			  current++;
			  return true;
     	 }

		  while (IsValid() && (isalpha(*current) || isdigit(*current) || *current == '-'))
		  {
			  (*length)++;
			  current++;
		  }

		  return true;
	  }


	  bool CParser::GetValue(const char* szName, int* ipValue)
	  {
		  //filedata 안에서 단어를 찾는다.
		 //찾은 단어를 저장할 버퍼
		  char* chpBuff, chWord[256];
		  int	iLength;

		  //탐색을 시작할 커서를 파일 데이터 가장 앞으로 초기화
		  current = filedata;

		  // 찾고자 하는 단어가 나올때까지 계속 찾을 것이므로 while 문으로 검사.
		  while (GetNextWord(&chpBuff, &iLength))
		  {
			  // Word 버퍼에 찾은 단어를 저장한다.
			  memset(chWord, 0, 256);
			  memcpy(chWord, chpBuff, iLength);
			  chWord[iLength] = '\0';
			  // 인자로 입력 받은 단어와 같은지 검사한다.
			  if (strcmp(szName, chWord) == 0)
			  {
				  // 맞다면 바로 뒤에 = 을 찾자.
				  if (GetNextWord(&chpBuff, &iLength))
				  {
					  memset(chWord, 0, 256);
					  memcpy(chWord, chpBuff, iLength);
					  if (strcmp(chWord, "=") == 0)
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
					  return FALSE;
				  }
			  }
		  }
		  return FALSE;
	  }

	  bool CParser::GetCharacter(const char* szName, char* cdata)
	  {
		  //filedata 안에서 단어를 찾는다.
	  //찾은 단어를 저장할 버퍼
		  char* chpBuff, chWord[256];
		  int	iLength;

		  //커서 초기화
		  current = filedata;

		  // 찾고자 하는 단어가 나올때까지 계속 찾을 것이므로 while 문으로 검사.
		  while (GetNextWord(&chpBuff, &iLength))
		  {
			  // Word 버퍼에 찾은 단어를 저장한다.
			  memset(chWord, 0, 256);
			  memcpy(chWord, chpBuff, iLength);
			  // 인자로 입력 받은 단어와 같은지 검사한다.
			  chWord[iLength] = '\0';
			  if (strcmp(szName, chWord) == 0)
			  {
				  // 맞다면 바로 뒤에 = 을 찾자.
				  if (GetNextWord(&chpBuff, &iLength))
				  {
					  memset(chWord, 0, 256);
					  memcpy(chWord, chpBuff, iLength);
					  if (strcmp(chWord, "=") == 0)
					  {
						  // = 다음의 데이터 부분을 얻자.
						  if (GetNextWord(&chpBuff, &iLength))
						  {
							  memset(chWord, 0, 256);
							  memcpy(chWord, chpBuff, iLength);
							  memcpy(cdata, chWord, iLength);
							 // *cdata = *chWord;
							  return TRUE;
						  }
						  return FALSE;
					  }
					  return FALSE;
				  }
			  }
		  }

		  return FALSE;
	  }

	  bool CParser::GetString(const char* szName, char* cdata)
	  {
		  //filedata 안에서 단어를 찾는다.
	  //찾은 단어를 저장할 버퍼
		  char* chpBuff, chWord[256];
		  int	iLength;

		  //커서 초기화
		  current = filedata;

		  // 찾고자 하는 단어가 나올때까지 계속 찾을 것이므로 while 문으로 검사.
		  while (GetNextWord(&chpBuff, &iLength))
		  {
			  // Word 버퍼에 찾은 단어를 저장한다.
			  memset(chWord, 0, 256);
			  memcpy(chWord, chpBuff, iLength);
			  // 인자로 입력 받은 단어와 같은지 검사한다.
			  chWord[iLength] = '\0';
			  if ( strcmp(szName, chWord) == 0)
			  {
				  // 맞다면 바로 뒤에 = 을 찾자.
				  if (GetNextWord(&chpBuff, &iLength))
				  {
					  memset(chWord, 0, 256);
					  memcpy(chWord, chpBuff, iLength);
					  if (0 == strcmp(chWord, "="))
					  {
						  // = 다음의 데이터 부분을 얻자.
						  if (GetStringWord(&chpBuff, &iLength))
						  {
							  memset(chWord, 0, 256);
							  memcpy(chWord, chpBuff, iLength);
							  memcpy(cdata, chWord, iLength);
							  // *cdata = *chWord;
							  return TRUE;
						  }
						  return FALSE;
					  }
					  return FALSE;
				  }
			  }
		  }

		  return FALSE;
	  }

	  bool CParser::GetStringWord(char** buf, int* length)
	  {
		  if (!RemoveSpace()) return false;

		  if(*current != '"') return false;

		  current++;
		  *buf = current;
		  *length = 0;
		  while (*current != '"')
		  {
			  (*length)++;
			  current++;
		  }

		  return true;
	  }

	  bool CParser::GetOneByte(char* buf)
	  {
		  //커서가 가리키는 단어가 널 문자일 경우
		  if (!RemoveSpace()) return false;

		  *buf = *current;
		  current++;
		  return true;
	  }

	  bool CParser::IsValid()
	  {
			return current && *current != '\0';
	  }


