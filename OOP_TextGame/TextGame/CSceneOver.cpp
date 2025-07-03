#include "CSceneOver.h"

void CSceneOver::init()
{
	if (!bIsLoaded)
	{
		bool LoadOverData = false;
		if (!LoadOverData)
		{
			CParser parser;

			if (!parser.LoadFile("gameOver.txt"))
			{
				printf("게임 오버 파일 불러오기 실패");
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
				if (!parser.GetString("GameOvermessage", message))
				{
					break;
				}

				bSuccess = true;
			} while (0);

			if (!bSuccess)
			{
				printf("게임 오버 메시지 로딩 실패\n");
				return;
			}
			LoadOverData = true;
		}

		bIsLoaded = true;
	}
}

void CSceneOver::Update()
{
	//입력
	if (GetAsyncKeyState(VK_SPACE) & 0x8001)
	{
		CSceneManager::GetInstance()->LoadScene(Title);
	}


	//랜더
	if (!fu.Skip)
	{
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
