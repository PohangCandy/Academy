 //WindowsProject1.cpp : 애플리케이션에 대한 진입점을 정의합니다.


#include "framework.h"
#include "WindowsProject1.h"
#include "windowsx.h"
#include "JPS.h"
#include "Dungeon.h"

#define MAX_LOADSTRING 100
#define GRID_SIZE 32
#define GRID_WIDTH 150
#define GRID_HEIGHT 100

int g_iGridSize = GRID_SIZE;

//나중에 출발지와 목적지를 겹치게 두면 출발지를 먼저 옮길 수 있는 예외처리도 해줘야 할 듯
Dungeon g_Dungeon(GRID_HEIGHT,GRID_WIDTH);
JPS g_AStar(&g_Dungeon);

HBRUSH g_hEmptyBrush;
HBRUSH g_hTileBrush;
HBRUSH g_hStartBrush;
HBRUSH g_hGoalBrush;
HBRUSH g_hNodeListBrush;
HBRUSH g_hAstarAnswerListBrush;
HBRUSH g_hVisitedBrush;

HBRUSH g_hVisitedBrush1;
HBRUSH g_hVisitedBrush2;
HBRUSH g_hVisitedBrush3;
HBRUSH g_hVisitedBrush4;
HBRUSH g_hVisitedBrush5;
HBRUSH g_hVisitedBrush6;
HBRUSH g_hVisitedBrush7;
HBRUSH g_hVisitedBrush8;
HBRUSH g_hVisitedBrush9;

HPEN g_hGridPen;
HPEN g_hPathLinePen; // 경로 선분용 펜

HFONT g_hDisplayFont = NULL; // 전역 폰트 핸들
HFONT g_hOldFont = NULL;

bool g_bErase = false;
bool g_bStartMove = false;
bool g_bGoalMove = false;
bool g_bDrag = false;

//선분을 길찾기 시행때만 찾는다.
bool g_bFindPath = false;

//메모리DC 관련 변수들
HDC g_hMemDC;
HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
RECT g_MemDC_Rect;

// 초기 배율 (100%)
float g_fScale = 1.0f;
POINT g_pan{ 0,0 }; // (옵션) 패닝용

int g_originX = 0;
int g_originY = 0;

void RecreateFont(HWND hWnd)
{
    if (g_hDisplayFont) DeleteObject(g_hDisplayFont);

    // 현재 셀 크기
    int desiredCellFraction = 4;
    int fontHeight = -(g_iGridSize / desiredCellFraction);

    // 폰트 크기
    if (fontHeight > -12) fontHeight = -12;

    g_hDisplayFont = CreateFont(
        fontHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial"
    );
}

void RenderGrid(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
   //그리드의 마지막 선을 추가로 그리기위한 <= 반복 조건
    for (int iCntW = 0; iCntW <= GRID_WIDTH;iCntW++)
    {
        MoveToEx(hdc, iX, 0, NULL);
        LineTo(hdc, iX, GRID_HEIGHT * g_iGridSize);
        iX += g_iGridSize;
    }
    for (int iCntH = 0;iCntH <= GRID_HEIGHT;iCntH++)
    {
        MoveToEx(hdc, 0, iY, NULL);
        LineTo(hdc, GRID_WIDTH * g_iGridSize, iY);
        iY += g_iGridSize;
    }
    SelectObject(hdc, hOldBrush);
}

//이제 맵정보를 바탕으로 랜더링하도록 만들어본다.
//none = 0,//빈칸 nothing
//start,//스타트 지점 start
//end,//끝 지점 end
//obs,//장애물 obstacle
//n,//JPS로 만들어진 노드 nodelist
//v,//JPS로 탐색한 타일 visited
void RenderMap(HDC hdc)
{
    int iX = 0;
    int iY = 0;

    // 1. GDI Object Selection (펜을 한번 선택)
    // Pen을 한번 선택하여 Rectangle 호출시 불필요한 SelectObject를 방지
    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);

    HBRUSH hCurrentBrush = NULL;
    HBRUSH hOldBrush = NULL; // 첫 SelectObject 호출 시 이전 브러시를 저장할 변수

    // 2. 텍스트 GDI 상태
    // 텍스트 관련 GDI 상태 설정/복구를 루프 외부로 이동시켜 SelectObject, SetBkMode 등의 비용을 최소화
    HFONT hOldFont = NULL;
    COLORREF oldTextColor = 0;
    int oldBkMode = 0;
    bool bTextEnabled = (g_iGridSize >= 64);

    if (bTextEnabled)
    {
        hOldFont = (HFONT)SelectObject(hdc, g_hDisplayFont);
        oldTextColor = GetTextColor(hdc);
        oldBkMode = GetBkMode(hdc);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
    }

    g_Dungeon.mapUpdate();

    for (int iCntH = 0; iCntH < GRID_HEIGHT; iCntH++)
    {
        for (int iCntW = 0; iCntW < GRID_WIDTH; iCntW++)
        {
            iX = iCntW * g_iGridSize;
            iY = iCntH * g_iGridSize;
            HBRUSH hNewBrush = NULL;

            // 브러시 결정 로직
            switch (g_Dungeon.CheckTile(iCntH, iCntW))
            {
            case none: hNewBrush = g_hEmptyBrush; break;
            case start: hNewBrush = g_hStartBrush; break;
            case goal: hNewBrush = g_hGoalBrush; break;
            case obs: hNewBrush = g_hTileBrush; break;
            case nodelist: hNewBrush = g_hNodeListBrush; break;
            case shortest: hNewBrush = g_hAstarAnswerListBrush; break;
            case visited:
            {
                // Visited 브러시 선택 
                switch (g_Dungeon.getRGB(iCntH, iCntW) % 10)
                {
                case 1: hNewBrush = g_hVisitedBrush1; break;
                case 2: hNewBrush = g_hVisitedBrush2; break;
                case 3: hNewBrush = g_hVisitedBrush3; break;
                case 4: hNewBrush = g_hVisitedBrush4; break;
                case 5: hNewBrush = g_hVisitedBrush5; break;
                case 6: hNewBrush = g_hVisitedBrush6; break;
                case 7: hNewBrush = g_hVisitedBrush7; break;
                case 8: hNewBrush = g_hVisitedBrush8; break;
                case 9: hNewBrush = g_hVisitedBrush9; break;
                default: hNewBrush = g_hVisitedBrush; break;
                }
                break;
            }
            default: hNewBrush = g_hEmptyBrush; break;
            }

            //Tile Drawing
            if (hNewBrush != hCurrentBrush)
            {
                if (hCurrentBrush == NULL) {
                    hOldBrush = (HBRUSH)SelectObject(hdc, hNewBrush);
                }
                else {
                    SelectObject(hdc, hNewBrush);
                }
                hCurrentBrush = hNewBrush;
            }

            Rectangle(hdc, iX, iY, iX + g_iGridSize + 1, iY + g_iGridSize + 1);

            //Text Overlay (상태는 이미 설정됨)
            if (bTextEnabled)
            {
                Grid* g = g_Dungeon.getGrid(iCntH, iCntW);
                if (g && g->type != obs && g->type != none)
                {
                    char buffer[64];

                    sprintf_s(buffer, "g: %.1f", g->g);
                    TextOutA(hdc, iX + 2, iY + 2, buffer, strlen(buffer));

                    sprintf_s(buffer, "h: %.1f", g->h);
                    TextOutA(hdc, iX + 2, iY + g_iGridSize / 3 + 2, buffer, strlen(buffer));

                    sprintf_s(buffer, "f: %.1f", g->f);
                    TextOutA(hdc, iX + 2, iY + 2 * g_iGridSize / 3 + 2, buffer, strlen(buffer));
                }
            }
        }
    }

    // 3. GDI 상태 복구
    if (hOldBrush) SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);

    if (bTextEnabled)
    {
        SelectObject(hdc, hOldFont);
        SetBkMode(hdc, oldBkMode);
        SetTextColor(hdc, oldTextColor);
    }
}

void RenderLine(HDC hdc, IMap& map)
{
    int iX = 0;
    int iY = 0;

    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hPathLinePen);


    for (int iCntH = 0; iCntH < GRID_HEIGHT;iCntH++)
    {
        for (int iCntW = 0; iCntW < GRID_WIDTH;iCntW++)
        {
            iX = iCntW * g_iGridSize;
            iY = iCntH * g_iGridSize;

            Grid* g = g_Dungeon.getGrid(iCntH, iCntW);

            if (g && g->gparent)
            {
                // 경로 표시가 필요한 타일 유형인지 확인
                if (g->type == shortest || g->type == goal || g->type == nodelist)
                {
                    // 1. 현재 타일의 중심 좌표 계산
                    int iCenterX = iX + g_iGridSize / 2;
                    int iCenterY = iY + g_iGridSize / 2;

                    // 2. 부모 타일의 인덱스와 중심 좌표 계산
                    int iParentX = g->gparent->x;
                    int iParentY = g->gparent->y;
                    int iParentCenterX = iParentX * g_iGridSize + g_iGridSize / 2;
                    int iParentCenterY = iParentY * g_iGridSize + g_iGridSize / 2;

                    // 4. 선 그리기: 펜은 이미 루프 밖에서 선택되었습니다.
                    MoveToEx(hdc, iCenterX, iCenterY, NULL);

                    if (g->type == nodelist)
                    {
                        // (JPS 노드 표시 로직은 그대로 유지)
                        int dx = g->gparent->x - g->x;
                        int dy = g->gparent->y - g->y;
                        if (dx < 0) dx = -1;
                        else if (dx == 0) dx = 0;
                        else dx = 1;
                        if (dy < 0) dy = -1;
                        else if (dy == 0) dy = 0;
                        else dy = 1;

                        const float HALF_GRID_SIZE = (float)g_iGridSize / 2.0f;
                        const float DIAG_RATIO = 0.707f;

                        float fEndShortX = (float)iCenterX;
                        float fEndShortY = (float)iCenterY;

                        if (dx != 0 && dy != 0)
                        {
                            fEndShortX += (float)dx * HALF_GRID_SIZE * DIAG_RATIO;
                            fEndShortY += (float)dy * HALF_GRID_SIZE * DIAG_RATIO;
                        }
                        else
                        {
                            fEndShortX += (float)dx * HALF_GRID_SIZE;
                            fEndShortY += (float)dy * HALF_GRID_SIZE;
                        }

                        LineTo(hdc, (int)fEndShortX, (int)fEndShortY);
                    }
                    else
                    {
                        LineTo(hdc, iParentCenterX, iParentCenterY);
                    }
                }
            }
        }
    }

    SelectObject(hdc, hOldPen);

    g_bFindPath = false;
}


//-----------------------------
//메모리 DC 크기 계산을 위한 헬퍼 함수
void RecreateMemDC(HWND hWnd)
{
    // 기존 메모리 DC 자원 정리
    if (g_hMemDCBitmap) SelectObject(g_hMemDC, g_hMemDCBitmap_old);
    if (g_hMemDCBitmap) DeleteObject(g_hMemDCBitmap);
    if (g_hMemDC) DeleteDC(g_hMemDC);

    // 맵의 논리적 크기 (g_iGridSize 기준)
    const int worldW = GRID_WIDTH * g_iGridSize;
    const int worldH = GRID_HEIGHT * g_iGridSize;

    // 배율(g_fScale)을 적용한 최종 메모리 DC의 픽셀 크기
    int memDC_Width = (int)(worldW * g_fScale);
    int memDC_Height = (int)(worldH * g_fScale);

    // 최소 크기를 윈도우 크기로 설정 (최소한 윈도우 크기 이상이어야 함)
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);

    // 메모리 DC 크기는 맵 전체 크기 또는 윈도우 크기 중 더 큰 값으로 설정
    // 맵이 윈도우보다 작더라도 맵 크기(g_MemDC_Rect는 맵 크기 용도로 사용)로 설정.
    g_MemDC_Rect.right = memDC_Width;
    g_MemDC_Rect.bottom = memDC_Height;

    HDC hdc = GetDC(hWnd);
    g_hMemDC = CreateCompatibleDC(hdc);

    // 맵 전체 크기로 비트맵 생성 (여기서 g_MemDC_Rect는 맵 크기로 사용)
    g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDC_Rect.right, g_MemDC_Rect.bottom);
    g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
    ReleaseDC(hWnd, hdc);
}
//-----------------------------

// 화면 픽셀 -> 그리드 타일 좌표 변환 (전역으로 추가)
void ScreenToTile(int screenX, int screenY, int& outTileX, int& outTileY)
{

    float px = (float)screenX - (float)g_pan.x;
    float py = (float)screenY - (float)g_pan.y;


    float worldX = px / g_fScale;
    float worldY = py / g_fScale;

    outTileX = (int)(worldX) / g_iGridSize;
    outTileY = (int)(worldY) / g_iGridSize;
}

//// 전역 변수:
//HINSTANCE hInst;                                // 현재 인스턴스입니다.
//WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
//WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
//
// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // 2. 수정: 모든 보고서 모드를 디버그 출력으로 강제 설정

    // ERROR와 ASSERT는 디버그 창으로 바로 보냅니다.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    // 경고(WARN) 메시지(여기에 누수 보고서가 포함됨)도 디버그 창으로 보냅니다.
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);


    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    //g_AStar.makeInitList();

    //g_AStar.~JPS();
    //g_Dungeon.~Dungeon();

    _CrtDumpMemoryLeaks(); // 이 함수는 누수 감지 플래그가 설정되어 있다면 바로 출력합니다.



    return (int)msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
    wcex.lpszClassName = L"Test Windows";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    //hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    /*HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);*/

    HWND hWnd = CreateWindowW(L"Test Windows", L"szTitle", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_LBUTTONDOWN:
        g_bDrag = true;
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            int iTileX = (xPos + g_originX) / g_iGridSize;
            int iTileY = (yPos + g_originY) / g_iGridSize;

            //선택 타일 우선 순위
            //첫 선택 타일이 장애물이면 지우기 모드 아니면 장애물 넣기 모드
            if (iTileX >= 0 && iTileX < GRID_WIDTH && iTileY >= 0 && iTileY < GRID_HEIGHT) 
            {
                if (iTileX == g_Dungeon.getStart()->x && iTileY == g_Dungeon.getStart()->y)
                {
                    g_bErase = false;
                    g_bStartMove = true;
                    g_bGoalMove = false;
                }
                else if (iTileX == g_Dungeon.getGoal()->x && iTileY == g_Dungeon.getGoal()->y)
                {
                    g_bErase = false;
                    g_bStartMove = false;
                    g_bGoalMove = true;
                }

                else if (g_Dungeon.CheckTile(iTileY, iTileX) == obs)
                {
                    g_bErase = true;
                    g_bStartMove = false;
                    g_bGoalMove = false;
                }
                else if (g_Dungeon.CheckTile(iTileY, iTileX) != obs && g_Dungeon.CheckTile(iTileY, iTileX) != start && g_Dungeon.CheckTile(iTileY, iTileX) != goal)
                {
                    g_bErase = false;
                    g_bStartMove = false;
                    g_bGoalMove = false;
                }
                else
                {
                    g_bDrag = false;
                }
            }
        }
        break;
    case WM_LBUTTONUP:
        g_bDrag = false;
        g_bStartMove = false;
        g_bGoalMove = false;
        break;
    case WM_RBUTTONDOWN:
        g_AStar.findPathwithRender();
        g_bFindPath = true;
        InvalidateRect(hWnd, NULL, false);
        break;
    case WM_MOUSEMOVE:
        if (g_bDrag)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            int iTileX = (xPos + g_originX) / g_iGridSize;
            int iTileY = (yPos + g_originY) / g_iGridSize;



            g_AStar.bfirst = true;

            if (g_bStartMove)
            {
                if (iTileX >= 0 && iTileX < GRID_WIDTH && iTileY >= 0 && iTileY < GRID_HEIGHT)
                {
                    //그냥 원래 이전 값으로 복원해준다.
                    int e = g_Dungeon.CheckTile(g_Dungeon._start.y, g_Dungeon._start.x);

                    g_Dungeon.ChangeTile(g_Dungeon._start.y, g_Dungeon._start.x, none);
                    g_Dungeon._start.y = iTileY;
                    g_Dungeon._start.x = iTileX;
                    g_Dungeon.mapUpdate();
                    g_AStar.updateNode();
                }
            }
            else if (g_bGoalMove)
            {
                if (iTileX >= 0 && iTileX < GRID_WIDTH && iTileY >= 0 && iTileY < GRID_HEIGHT)
                {
                    g_Dungeon.ChangeTile(g_Dungeon._goal.y, g_Dungeon._goal.x, none);
                    g_Dungeon._goal.y = iTileY;
                    g_Dungeon._goal.x = iTileX;
                    g_Dungeon.mapUpdate();
                    g_AStar.updateNode();
                }
            }
            else
            {
                if (iTileX >= 0 && iTileX < GRID_WIDTH && iTileY >= 0 && iTileY < GRID_HEIGHT)
                {
                    if (g_bErase)
                    {
                        if (g_Dungeon.CheckTile(iTileY, iTileX) == obs)
                        {
                            g_Dungeon.ChangeTile(iTileY, iTileX, none);
                        }
                    }
                    else
                    {
                        if (g_Dungeon.CheckTile(iTileY, iTileX) != start && g_Dungeon.CheckTile(iTileY, iTileX) != goal)
                        {
                            g_Dungeon.ChangeTile(iTileY, iTileX, obs);
                        }
                    }
                }
            }
            InvalidateRect(hWnd, NULL, false);
        }
        break;
    case WM_CREATE:
    {
        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hPathLinePen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255)); // 파란색, 두께 2

        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
        g_hStartBrush = CreateSolidBrush(RGB(0, 200, 0));
        g_hGoalBrush = CreateSolidBrush(RGB(200, 0, 0));
        g_hNodeListBrush = CreateSolidBrush(RGB(100, 100, 200));

        g_hVisitedBrush = CreateSolidBrush(RGB(180, 210, 250)); 
        g_hVisitedBrush1 = CreateSolidBrush(RGB(250, 210, 170)); 
        g_hVisitedBrush2 = CreateSolidBrush(RGB(190, 250, 180));
        g_hVisitedBrush3 = CreateSolidBrush(RGB(250, 190, 200));
        g_hVisitedBrush4 = CreateSolidBrush(RGB(175, 240, 230));
        g_hVisitedBrush5 = CreateSolidBrush(RGB(250, 250, 180));
        g_hVisitedBrush6 = CreateSolidBrush(RGB(220, 210, 240));
        g_hVisitedBrush7 = CreateSolidBrush(RGB(250, 230, 190)); 
        g_hVisitedBrush8 = CreateSolidBrush(RGB(200, 220, 245));
        g_hVisitedBrush9 = CreateSolidBrush(RGB(250, 200, 180)); 

        g_hEmptyBrush = CreateSolidBrush(RGB(255, 255, 255));

        //정답인 노드 덧칠
        g_hAstarAnswerListBrush = CreateSolidBrush(RGB(200, 200, 0));

        RecreateFont(hWnd);

        RecreateMemDC(hWnd);
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RenderMap(g_hMemDC);

        if (g_bFindPath)
        {
            RenderLine(g_hMemDC, g_Dungeon); // 부모 노드 및 최단 거리 선분 그리기
        }

        BitBlt(hdc, 0, 0, g_MemDC_Rect.right, g_MemDC_Rect.bottom, g_hMemDC, g_originX, g_originY, SRCCOPY);

        EndPaint(hWnd, &ps);
    }
    break;
     case WM_SIZE:
     {
         RecreateMemDC(hWnd);
     }
     break;
     case WM_MOUSEWHEEL:
     {
         int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
         int oldGridSize = g_iGridSize; // 이전 크기를 저장합니다.

         // 1. 새 그리드 크기 계산
         if (zDelta > 0) {
             g_iGridSize = (int)(g_iGridSize * 2.0f);
         }
         else
         {
             g_iGridSize = (int)(g_iGridSize * 0.5f);
         }

         if (g_iGridSize < GRID_SIZE / 2) g_iGridSize = GRID_SIZE / 2;
         if (g_iGridSize > 64) g_iGridSize = 64;

         if (oldGridSize != g_iGridSize)
         {
             float fRatio = (float)g_iGridSize / (float)oldGridSize; // 스케일 비율

             g_originX = (int)(g_originX * fRatio);
             g_originY = (int)(g_originY * fRatio);

             RecreateFont(hWnd);
         }
         RecreateMemDC(hWnd);
         InvalidateRect(hWnd, NULL, TRUE);
     }
     break;
     case WM_KEYDOWN:
     {
         // 1. 맵의 총 논리적 픽셀 크기
         const int WORLD_WIDTH = GRID_WIDTH * g_iGridSize;
         const int WORLD_HEIGHT = GRID_HEIGHT * g_iGridSize;

         // 2. 클라이언트 윈도우 크기 (현재 화면에 보이는 영역)
         RECT clientRect;
         GetClientRect(hWnd, &clientRect);
         const int VIEW_WIDTH = clientRect.right;
         const int VIEW_HEIGHT = clientRect.bottom;

         // 3. 허용 가능한 최대 오프셋 (Max Origin)
         // 맵이 윈도우보다 클 때만 패닝을 허용합니다.
         const int MAX_ORIGIN_X = max(0, WORLD_WIDTH - VIEW_WIDTH);
         const int MAX_ORIGIN_Y = max(0, WORLD_HEIGHT - VIEW_HEIGHT);


         switch (wParam)
         {
         case 'M':
         {
             const float MIN_OBS_RATIO = 0.10f;
             const float MAX_OBS_RATIO = 0.40f;
             float random_ratio = MIN_OBS_RATIO + (float)rand() / RAND_MAX * (MAX_OBS_RATIO - MIN_OBS_RATIO);
             g_Dungeon.GenerateRandomMap(random_ratio);
             break;
         }
         case 'R':
         {
             g_Dungeon.resetObs();
             break;
         }
         case VK_SPACE:
         {
             g_AStar.findPath();
             g_bFindPath = true;
             break;
         }
         case VK_LEFT: {
             g_originX = max(0, g_originX - g_iGridSize); 
             break;
         } 
         case VK_RIGHT: {
             g_originX = min(MAX_ORIGIN_X, g_originX + g_iGridSize);
             break;
         }
         case VK_UP: {
             g_originY = max(0, g_originY - g_iGridSize); 
             break;
         }
         case VK_DOWN: {
             g_originY = min(MAX_ORIGIN_Y, g_originY + g_iGridSize);
             break;
         } 
         }

         InvalidateRect(hWnd, NULL, FALSE);
     }
     break;
    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDCBitmap);
        //DeleteObject(g_hMemDC);
        DeleteDC(g_hMemDC);

        DeleteObject(g_hGridPen);
        DeleteObject(g_hPathLinePen);

        DeleteObject(g_hTileBrush);
        DeleteObject(g_hStartBrush);
        DeleteObject(g_hGoalBrush);
        DeleteObject(g_hNodeListBrush);

        DeleteObject(g_hVisitedBrush);
        DeleteObject(g_hVisitedBrush1);
        DeleteObject(g_hVisitedBrush2);
        DeleteObject(g_hVisitedBrush3);
        DeleteObject(g_hVisitedBrush4);
        DeleteObject(g_hVisitedBrush5);
        DeleteObject(g_hVisitedBrush6);
        DeleteObject(g_hVisitedBrush7);
        DeleteObject(g_hVisitedBrush8);
        DeleteObject(g_hVisitedBrush9);

        DeleteObject(g_hEmptyBrush);
        DeleteObject(g_hAstarAnswerListBrush);

        DeleteObject(g_hDisplayFont);

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (((WORD)(((DWORD_PTR)(wParam)) & 0xffff)) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

