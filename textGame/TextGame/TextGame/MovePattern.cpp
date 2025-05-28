#include "MovePattern.h"

bool LoadPattern(tag_Pattern pattern[])
{
	static CParser Parser;
	int typeNum = 0;
	int stepNum = 0;

	bool bSuccess = false;

	if (!Parser.LoadFile("movepattern.txt"))
	{
		printf("패턴 파일 로딩 실패\n");
	};

	do {
		if (!Parser.GetValue("PatternTypeNum", &typeNum))
		{
			break;
		}
		if (!Parser.GetValue("PatternStepNum", &stepNum))
		{
			break;
		}

		bSuccess = true;
	} while (0);

	if (!bSuccess)
	{
		printf("패턴 데이터 값 로딩 실패\n");
		return false;
	}

	for (int i = 0; i < typeNum; i++)
	{
		char p[12];
		sprintf(p, "pattern%d", i);
		strcpy(pattern[i].name, p);
		pattern[i].stepCount = stepNum;
		if (!GetPattern(Parser, p, &pattern[i], stepNum))
		{
			return false;
		}
	}
	return true;
}

//아직 패턴을 추가했을때 오류 잡는부분이 부족함
//ex) (-1-,1)오류 안나고 정상작동
bool GetPattern(CParser parser, const char* patternName, tag_Pattern* pattern, int patternSize)
{
	//filedata 안에서 단어를 찾는다.
		//찾은 단어를 저장할 버퍼
	char* chpBuff, chWord[256];
	int	iLength;

	//탐색을 시작할 커서를 파일 데이터 가장 앞으로 초기화
	parser.current = parser.filedata;

	// 찾고자 하는 단어가 나올때까지 계속 찾을 것이므로 while 문으로 검사.
	while (parser.GetNextWord(&chpBuff, &iLength))
	{
		// Word 버퍼에 찾은 단어를 저장한다.
		memset(chWord, 0, 256);
		memcpy(chWord, chpBuff, iLength);
		// 인자로 입력 받은 단어와 같은지 검사한다.
		if (0 == strcmp(patternName, chWord))
		{
			// 맞다면 바로 뒤에 = 을 찾자.
			while (parser.GetNextWord(&chpBuff, &iLength))
			{
				memset(chWord, 0, 256);
				memcpy(chWord, chpBuff, iLength);
				if (0 == strcmp(chWord, "="))
				{
					// = 다음의 패턴 데이터 부분을 얻자.
					int i = 0;
					while (parser.GetNextWord(&chpBuff, &iLength) && i != patternSize)
					{
						memset(chWord, 0, 256);
						memcpy(chWord, chpBuff, iLength);
						//바로 뒤의 (를 찾자.
						if (0 == strcmp(chWord, "("))
						{
							//패턴의 x좌표를 찾는다.
							if (parser.GetNextWord(&chpBuff, &iLength))
							{
								memset(chWord, 0, 256);
								memcpy(chWord, chpBuff, iLength);
								if (!isdigit(*chWord)) {
									if (!(*chWord == '-' && isdigit(*(chWord + 1))))
									{
										printf("잘못된 행동 패턴 양식\n");
										return false;
									}
								}
								pattern->Steps[i].x = atoi(chWord);
							}
							//,를 찾는다
							if (parser.GetNextWord(&chpBuff, &iLength))
							{
								memset(chWord, 0, 256);
								memcpy(chWord, chpBuff, iLength);
								if (0 == strcmp(chWord, ","))
								{
									//패턴의 y좌표를 찾는다.
									if (parser.GetNextWord(&chpBuff, &iLength))
									{
										memset(chWord, 0, 256);
										memcpy(chWord, chpBuff, iLength);
										if (!isdigit(*chWord)) {
											if (!(*chWord == '-' && isdigit(*(chWord + 1))))
											{
												printf("잘못된 행동 패턴 양식\n");
												return false;
											}
										}
										pattern->Steps[i].y = atoi(chWord);
										i++;
									}
								}
							}
						}
					}
					if (i == patternSize)
					{
						return true;
					}
				}
			}
			printf("잘못된 행동 패턴 양식\n");
			return false;
		}
	}
	printf("잘못된 행동 패턴 양식\n");
	return false;
}
