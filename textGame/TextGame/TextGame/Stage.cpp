#include "Stage.h"

//--------------------------------------------------------------------
//파일에 있는 스테이지 정보를 불러와 저장한다.
//--------------------------------------------------------------------
bool LoadStageInfo()
{
	static CParser Parser;
	int stagewidth = 0;
	int stageheight = 0;
	int stagenum = 0;

	bool bSuccess = false;

	if (!Parser.LoadFile("StageInfo.txt"))
	{
		printf("스테이지 정보 파일 로딩 실패\n");
	};

	do {
		if (!Parser.GetValue("stagewidth", &stagewidth))
		{
			break;
		}
		if (!Parser.GetValue("stageheight", &stageheight))
		{
			break;
		}
		if (!Parser.GetValue("stagenum", &stagenum))
		{
			break;
		}

		bSuccess = true;
	} while (0);

	if (!bSuccess)
	{
		printf("스테이지 데이터 값 로딩 실패\n");
		return false;
	}

	if (stagenum > MAXSTAGENUM)
	{
		printf("최개 스테이지 개수 초과\n");
		return false;
	}

	//불러온 파일 이름으로 파일을 호출한다.
	for (int i = 0; i < stagenum; i++)
	{
		char filename[256];
		char* chpBuff, chWord[256];
		int iLength;
		if (Parser.GetNextWord(&chpBuff, &iLength))
		{
			memset(chWord, 0, 256);
			memcpy(chWord, chpBuff, iLength);
			chWord[iLength] = '\0';
			sprintf_s(filename, sizeof(filename), "%s.txt", chWord);

			//지금은 그냥 스테이지 파일을 읽어서 바로 반영하므로 가장  마지막 스테이지가 반영될 것
			//이걸 먼저 파일에 있는 데이터를 읽어서 스테이지 배열에 저장하고
			//스테이지가 바뀌면 반영되도록 만들자.
			//LoadStage(filename, stagewidth, stageheight);

			sprintf_s(StagePool[i].filename, sizeof(StagePool[i].filename), "%s", filename);
			StagePool[i].width = stagewidth;
			StagePool[i].height = stageheight;
		}
	}


	return true;
}

//bool LoadStage(const char* filename, int width, int height)
bool LoadStage(tag_Stage* s)
{
	static CParser Parser;
	int stagewidth = s->width;
	int stageheight = s->height;

	bool bSuccess = false;

	if (!Parser.LoadFile(s->filename))
	{
		printf("스테이지 파일 로딩 실패\n");
	};

	//파일을 다 읽고 메모리에서 데이터를 분석해야 시간이 적게 걸림.
	for (int i = 0; i < stageheight; i++)
	{
		for (int j = 0; j < stagewidth - 1; j++)
		{
			char buff;
			if (Parser.GetOneByte(&buff))
			{
				if (isspace(buff) || buff == 'w' || buff == '.')
				{
					continue;
				}
				tag_Enemy* e = SearchEnemyType(buff);
				LoadEnemyPool(e, j, i);
			}
		}
	}


	return false;
}

//--------------------------------------------------------------------
//적을 찾아서 적 파일 데이터에서 모양으로 비교
//위치는 스테이지 파일을 기반으로, 나머지 스펙은 적 데이터 파일을 기반으로 작성한 후
//메모리 풀에 넣어준다.
//--------------------------------------------------------------------
tag_Enemy* SearchEnemyType(const char eShape)
{
	for (int i = 0; i < MAXENEMYTYPE; i++)
	{
		if (EnemyType[i].shape == eShape)
		{
			return &EnemyType[i];
		}
	}
	printf("일치하는 모양을 가진 Enemy가 없습니다.\n");
	return nullptr;
}

//--------------------------------------------------------------------
//스테이지에 있는 적을 적 메모리 풀에 저장
//--------------------------------------------------------------------
bool LoadEnemyPool(tag_Enemy* ep, int posx, int posy)
{
	if (ep == nullptr) return false;

	ep->x = posx;
	ep->y = posy;
	for (int i = 0; i < MAXENEMYNUM; i++)
	{
		if (!EP[i].Active)
		{
			EP[i] = *ep;
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------
// 스테이지 종료 조건 체크
//--------------------------------------------------------------------
bool Stage_End()
{
	//모든 스크린에서 적 타입이 있는지 체크
	//그냥 적 메모리풀만 체크해도 되겠는데?
	for (int i = 0; i < MAXENEMYNUM;i++)
	{
		if (EP[i].Active) return false;
	}

	return true;
}
