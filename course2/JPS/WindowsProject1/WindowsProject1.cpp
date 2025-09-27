 //WindowsProject1.cpp : 애플리케이션에 대한 진입점을 정의합니다.


#include "framework.h"
#include "WindowsProject1.h"
#include "windowsx.h"
#include "JPS.h"
#include "Dungeon.h"

#define MAX_LOADSTRING 100
#define GRID_SIZE 32
#define GRID_WIDTH 200
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

    int fontHeight = -(g_iGridSize / 6);
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
void RenderMap(HDC hdc, IMap& map)
{
    
    int iX = 0;
    int iY = 0;
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hStartBrush);

    g_Dungeon.mapUpdate();

    for (int iCntH = 0; iCntH < GRID_HEIGHT;iCntH++)
    {
        for (int  iCntW = 0; iCntW < GRID_WIDTH;iCntW++)
        {
            iX = iCntW * g_iGridSize;
            iY = iCntH * g_iGridSize;
            switch (g_Dungeon.CheckTile(iCntH, iCntW))
            {
            case none:
               SelectObject(hdc, g_hEmptyBrush);
                break;
            case start:
               SelectObject(hdc, g_hStartBrush);
                break;
            case end:
                SelectObject(hdc, g_hGoalBrush);
                break;
            case  obs:
                SelectObject(hdc, g_hTileBrush);
                break;
            case nodelist:
               SelectObject(hdc, g_hNodeListBrush);
                break;
            case visited:
            {
                switch (g_Dungeon.getRGB(iCntH, iCntW) % 10)
                {
                case 1:
                    SelectObject(hdc, g_hVisitedBrush1);
                    break;
                case 2:
                    SelectObject(hdc, g_hVisitedBrush2);
                    break;
                case 3:
                    SelectObject(hdc, g_hVisitedBrush3);
                    break;
                case 4:
                    SelectObject(hdc, g_hVisitedBrush4);
                    break;
                case 5:
                    SelectObject(hdc, g_hVisitedBrush5);
                    break;
                case 6:
                    SelectObject(hdc, g_hVisitedBrush6);
                    break;
                case 7:
                   SelectObject(hdc, g_hVisitedBrush7);
                    break;
                case 8:
                    SelectObject(hdc, g_hVisitedBrush8);
                    break;
                case 9:
                   SelectObject(hdc, g_hVisitedBrush9);
                    break;
                default:
                    SelectObject(hdc, g_hVisitedBrush);
                    break;
                }
            }
               
                break;
            case shortest:
                SelectObject(hdc, g_hAstarAnswerListBrush);
                break;
            default:
                break;
            }
            Rectangle(hdc, iX, iY, iX + g_iGridSize + 1, iY + g_iGridSize + 1);

            Grid* g = g_Dungeon.getGrid(iCntH, iCntW);
            if (g && g_iGridSize >= 64 && g->type != obs && g->type != none)
            {
                // 폰트 설정은 WM_CREATE와 WM_MOUSEWHEEL에서 미리 완료되었음.
                HFONT hOldFont = (HFONT)SelectObject(hdc, g_hDisplayFont);

                COLORREF oldTextColor = GetTextColor(hdc);
                int oldBkMode = GetBkMode(hdc);

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0, 0, 0));

                char buffer[64];

                // 1. g 값 출력 (타일 좌상단)
                sprintf_s(buffer, "g: %.1f", g->g);
                TextOutA(hdc, iX + 2, iY + 2, buffer, strlen(buffer));

                // 2. h 값 출력 (타일 중앙)
                sprintf_s(buffer, "h: %.1f", g->h);
                TextOutA(hdc, iX + 2, iY + g_iGridSize / 3 + 2, buffer, strlen(buffer));

                // 3. f 값 출력 (타일 중앙 하단)
                sprintf_s(buffer, "f: %.1f", g->f);
                TextOutA(hdc, iX + 2, iY + 2 * g_iGridSize / 3 + 2, buffer, strlen(buffer));


                SelectObject(hdc, hOldFont);

                SetBkMode(hdc, oldBkMode);
                SetTextColor(hdc, oldTextColor);
            }
        }
    }
    SelectObject(hdc, hOldBrush);
}

void RenderLine(HDC hdc, IMap& map)
{
    int iX = 0;
    int iY = 0;

    for (int iCntH = 0; iCntH < GRID_HEIGHT;iCntH++)
    {
        for (int iCntW = 0; iCntW < GRID_WIDTH;iCntW++)
        {
            iX = iCntW * g_iGridSize;
            iY = iCntH * g_iGridSize;

            Grid* g = g_Dungeon.getGrid(iCntH, iCntW);

            if (g && g->gparent && (g->type == shortest || g->type == end || g->type == nodelist))
            {
                // 1. 현재 타일의 중심 좌표 계산 (불필요한 오프셋 제거)
                int iCenterX = iX + g_iGridSize / 2;
                int iCenterY = iY + g_iGridSize / 2;

                // 2. 부모 타일의 인덱스
                int iParentX = g->gparent->x;
                int iParentY = g->gparent->y;

                // 3. 부모 타일의 월드 좌표 중심점
                int iParentCenterX = iParentX * g_iGridSize + g_iGridSize / 2;
                int iParentCenterY = iParentY * g_iGridSize + g_iGridSize / 2;

                // 4. 선 그리기
                HPEN hOldPen = (HPEN)SelectObject(hdc, g_hPathLinePen);
                // 시작점을 정확한 중심 좌표로 설정
                MoveToEx(hdc, iCenterX, iCenterY, NULL);        // 현재 노드의 중심
                if (g->type == nodelist)
                {
                    // 1. 방향 벡터 (부모 -> 자식의 반대 방향, 즉 자식 -> 부모 방향)
                       // dx, dy는 -1, 0, 1 중 하나가 됩니다.
                    int dx = g->gparent->x - g->x;
                    int dy = g->gparent->y - g->y;
                    if (dx < 0) dx = -1;
                    else if (dx == 0) dx = 0;
                    else dx = 1;
                    if (dy < 0) dy = -1;
                    else if (dy == 0) dy = 0;
                    else dy = 1;

                    // 2. 고정 길이 설정
                    const float HALF_GRID_SIZE = (float)g_iGridSize / 2.0f;
                    // 대각선 거리 비율: 1/sqrt(2) 또는 0.707. (타일 중심에서 모서리 방향으로의 길이)
                    const float DIAG_RATIO = 0.707f;

                    // 3. 선분 끝점 좌표 초기화
                    float fEndShortX = (float)iCenterX;
                    float fEndShortY = (float)iCenterY;

                    // 4. 방향에 따른 끝점 계산
                    if (dx != 0 && dy != 0) // 대각선 방향 (4방향)
                    {
                        // 대각선 이동 (dx, dy 모두 0이 아님)
                        fEndShortX += (float)dx * HALF_GRID_SIZE * DIAG_RATIO;
                        fEndShortY += (float)dy * HALF_GRID_SIZE * DIAG_RATIO;
                    }
                    else // 직교 방향 (상하좌우 4방향)
                    {
                        // 상하좌우 이동 (dx 또는 dy 중 하나만 0이 아님)
                        fEndShortX += (float)dx * HALF_GRID_SIZE;
                        fEndShortY += (float)dy * HALF_GRID_SIZE;
                    }

                    // 5. 선 그리기 (float 좌표를 int로 변환하여 사용)
                    LineTo(hdc, (int)fEndShortX, (int)fEndShortY);
                }
                else
                {
                    LineTo(hdc, iParentCenterX, iParentCenterY);    // 부모 노드의 중심까지 선분 연결
                }

                // 5. GDI 자원 복구
                SelectObject(hdc, hOldPen);
            }
        }
    }
    g_bFindPath = false;
}


//-----------------------------
// // 1. 메모리 DC 크기 계산을 위한 헬퍼 함수
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
    // 1) 화면 픽셀 -> 뷰포트 기준(패닝 보정)
    //    g_pan 는 WM_PAINT에서 SetViewportOrgEx로 사용되는 값이다.
    float px = (float)screenX - (float)g_pan.x;
    float py = (float)screenY - (float)g_pan.y;

    // 2) 뷰포트 픽셀 -> 논리(월드) 좌표(스케일 보정)
    //    WM_PAINT에서 SetWindowExtEx(worldW, worldH) 와 SetViewportExtEx(vp.cx, vp.cy)
    //    을 통해서 실제 출력은 g_fScale에 의해 변환된다.
    //    간단히, world = px / scale
    float worldX = px / g_fScale;
    float worldY = py / g_fScale;

    // 3) 논리 좌표 -> 타일 인덱스
    //    g_iGridSize는 한 타일의 논리(원래) 픽셀 크기
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

    //안씀, 쓰는척
    //UNREFERENCED_PARAMETER(hPrevInstance);
    //UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);


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
                    //g_Dungeon.ChangeTile(iTileY, iTileX, none);
                    g_bErase = false;
                    g_bStartMove = true;
                    g_bGoalMove = false;
                }
                else if (iTileX == g_Dungeon.getGoal()->x && iTileY == g_Dungeon.getGoal()->y)
                {
                    //g_Dungeon.ChangeTile(iTileY, iTileX, none);
                    g_bErase = false;
                    g_bStartMove = false;
                    g_bGoalMove = true;
                }
                //if (g_Tile[iTileY][iTileX] == TILE_START)
                //{
                //    g_bErase = false;
                //    g_bStartMove = true;
                //}
                else if (g_Dungeon.CheckTile(iTileY, iTileX) == obs)
                {
                    g_bErase = true;
                    g_bStartMove = false;
                    g_bGoalMove = false;
                }
                else if (g_Dungeon.CheckTile(iTileY, iTileX) != obs && g_Dungeon.CheckTile(iTileY, iTileX) != start && g_Dungeon.CheckTile(iTileY, iTileX) != end)
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

            //int iTileX, iTileY;
            //ScreenToTile(xPos, yPos, iTileX, iTileY);

            g_AStar.bfirst = true;

            if (g_bStartMove)
            {
                //g_Tile[g_AStar._start.y][g_AStar._start.x] = TILE_EMPTY;
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
                //g_Tile[g_AStar._start.y][g_AStar._start.x] = TILE_EMPTY;
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
                        if (g_Dungeon.CheckTile(iTileY, iTileX) != start && g_Dungeon.CheckTile(iTileY, iTileX) != end)
                        {
                            g_Dungeon.ChangeTile(iTileY, iTileX, obs);
                        }
                    }
                }
            }
            //마우스 드래그로 데이터가 변경되어 갱신을 요청 할 시 마지막 Erase 플래그를 false로 하여 화면 깜박임을 없앤다.
            //WM_PAINT에서는 윈도우 전체를 덮어쓰기 때문에 지우지 않아도 된다.
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

        //----------------------------------------------------
        //메모리DC 생성 코드
        //윈도우 생성 시 현 윈도우 크기와 동일한 메모리 DC 생성
       /* HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDC_Rect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDC_Rect.right, g_MemDC_Rect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);*/
        //----------------------------------------------------
        RecreateMemDC(hWnd);
    }
    break;
     case WM_PAINT:
     {
         //기존에는 윈도우 DC,hdc를 대상으로 출력했으나 이제 메모리 DC를 대상으로 출력한다.

         //메모리 DC를 클리어 하고
         //메모리DC 이미지 클리어
         PatBlt(g_hMemDC, 0, 0, g_MemDC_Rect.right, g_MemDC_Rect.bottom, WHITENESS);

         // ====== 스케일 적용: 매핑 모드 방식 ======
           // 논리 좌표계(window): 월드의 "원래 크기"
         const int worldW = GRID_WIDTH * g_iGridSize;
         const int worldH = GRID_HEIGHT * g_iGridSize;

         SetMapMode(g_hMemDC, MM_ANISOTROPIC);

         // 논리창(window) 크기(고정)
         SetWindowOrgEx(g_hMemDC, 0, 0, nullptr);
         SetWindowExtEx(g_hMemDC, worldW, worldH, nullptr);

         // 뷰포트(viewport): 화면으로 나갈 크기(배율 반영)
         // (옵션) g_pan.x/y 로 패닝 가능
         SetViewportOrgEx(g_hMemDC, g_pan.x, g_pan.y, nullptr);
         SIZE vp = { (int)(worldW * g_fScale), (int)(worldH * g_fScale) };
         SetViewportExtEx(g_hMemDC, vp.cx, vp.cy, nullptr);
         // =========================================
          
         
         //RenderObstacle,RenderGrid를 메모리 DC에 출력
         RenderGrid(g_hMemDC);
         //RenderObstacle(g_hMemDC);
         //RenderStartGoal(g_hMemDC);
         RenderMap(g_hMemDC,g_Dungeon);
         //RenderAStarList(g_hMemDC);
         if (g_bFindPath)
         {
             RenderLine(g_hMemDC, g_Dungeon);
         }

         //매모리 DC의 이미지를 윈도우 DC에 출력
         hdc = BeginPaint(hWnd, &ps);
         BitBlt(hdc, 0, 0, g_MemDC_Rect.right, g_MemDC_Rect.bottom, g_hMemDC, g_originX, g_originY, SRCCOPY);
         EndPaint(hWnd, &ps);

         break;
     }
     break;
     case WM_SIZE:
     {

         //------------------------------------------------------------------
         //SelectObject(g_hMemDC, g_hMemDCBitmap_old);
         //DeleteObject(g_hMemDCBitmap);
         //DeleteDC(g_hMemDC);

         //HDC hdc = GetDC(hWnd);
         //GetClientRect(hWnd, &g_MemDC_Rect);
         //g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDC_Rect.right, g_MemDC_Rect.bottom);
         //g_hMemDC = CreateCompatibleDC(hdc);
         //ReleaseDC(hWnd, hdc);

         //g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
         //------------------------------------------------------------------
         RecreateMemDC(hWnd);
     }
     break;
     case WM_MOUSEWHEEL:
     {
         int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

         if (zDelta > 0) {
             g_iGridSize = (int)(g_iGridSize * 2.0f);  // 확대
             //g_GRID_WIDTH *= 2;
             //g_GRID_HEIGHT *= 2;
         } 
         else
         {
             g_iGridSize = (int)(g_iGridSize * 0.5f);  // 축소
             //g_GRID_WIDTH /= 2;
             //g_GRID_HEIGHT /= 2;
         }

         if (g_iGridSize < GRID_SIZE / 2)   g_iGridSize = GRID_SIZE/2;   // 최소 크기 제한
         if (g_iGridSize > 128) g_iGridSize = 128; // 최대 크기 제한

         //InvalidateRect(hWnd, NULL, TRUE);
         //int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
         RecreateFont(hWnd);
         RecreateMemDC(hWnd);
         InvalidateRect(hWnd, NULL, TRUE);
     }
     break;
     case WM_KEYDOWN:
        {
         bool path = true;
         switch (wParam)
         {
         case VK_SPACE:
         {
             path = false;
             g_AStar.findPath();
             g_bFindPath = true;
             break;
         }
         case VK_LEFT:  g_originX -= g_iGridSize; break;  // 화면 오른쪽으로 이동
         case VK_RIGHT: g_originX += g_iGridSize; break;  // 화면 왼쪽으로 이동
         case VK_UP:    g_originY -= g_iGridSize; break;  // 화면 아래로 이동
         case VK_DOWN:   g_originY += g_iGridSize; break;  // 화면 위로 이동
         }
         InvalidateRect(hWnd, NULL, path);
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

