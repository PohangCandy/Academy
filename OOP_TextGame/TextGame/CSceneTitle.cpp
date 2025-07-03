#include "CSceneTitle.h"
#include "windows.h"


void CSceneTitle::init()
{
	if (!bIsLoaded)
	{
		bool LoadTitleData = false;
		if (!LoadTitleData)
		{
			CParser parser;

			if (!parser.LoadFile("gameTitle.txt"))
			{
				printf("게임 타이틀 파일 불러오기 실패");
				return;
			}

			bool bSuccess = false;
			do {
				if (!parser.GetValue("xpos", &xpos))
				{
					break;
				}
				if (!parser.GetValue("ypos", &ypos))
				{
					break;
				}
				if (!parser.GetString("GameTitlemessage", message))
				{
					break;
				}

				bSuccess = true;
			} while (0);

			if (!bSuccess)
			{
				printf("게임 타이틀 메시지 로딩 실패\n");
				return;
			}

			LoadTitleData = true;
		}

		bIsLoaded = true;
	}

}

void CSceneTitle::Update()
{
	//입력
	if (GetAsyncKeyState(VK_SPACE) & 0x8001)
	{
		CSceneManager::GetInstance()->LoadScene(Game);
	}

	if (!fu.Skip)
	{
		//랜더
		CurScreen->Buffer_Clear();
		for (int i = 0; i < (signed int)strlen(message); i++)
		{
			CurScreen->Sprite_Draw(xpos + i, ypos, message[i]);
		}
		fu.Render();
		for (int i = 0; i < (signed int)strlen(fu.Framemessage); i++)
		{
			CurScreen->Sprite_Draw(30 + i, 0, fu.Framemessage[i]);
		}
		CurScreen->Buffer_Flip();
	}
}
