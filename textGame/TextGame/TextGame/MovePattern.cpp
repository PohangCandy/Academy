#include "MovePattern.h"

void LoadPattern(const char* patternName)
{
	static CParser Parser;
	int typeNum = 0;
	int stepNum = 0;

	bool bSuccess = false;

	if (!Parser.LoadFile("test.txt"))
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
		return;
	}
}
