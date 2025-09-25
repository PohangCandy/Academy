 //WindowsProject1.cpp : 애플리케이션에 대한 진입점을 정의합니다.


#include "framework.h"
#include "WindowsProject1.h"
#include "windowsx.h"
#include "JPS.h"
#include "Dungeon.h"

#define MAX_LOADSTRING 100
#define GRID_SIZE 32
#define GRID_WIDTH 100
#define GRID_HEIGHT 50

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
HPEN g_hGridPen;

bool g_bErase = false;
bool g_bStartMove = false;
bool g_bGoalMove = false;
bool g_bDrag = false;

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

    for (int iCntW = 0; iCntW < GRID_WIDTH;iCntW++)
    {
        for (int iCntH = 0;iCntH < GRID_HEIGHT;iCntH++)
        {
            iX = iCntW * g_iGridSize;
            iY = iCntH * g_iGridSize;
            switch (g_Dungeon.CheckTile(iCntH, iCntW))
            {
            case none:
                hOldBrush = (HBRUSH)SelectObject(hdc, g_hEmptyBrush);
                break;
            case start:
                hOldBrush = (HBRUSH)SelectObject(hdc, g_hStartBrush);
                break;
            case end:
                hOldBrush = (HBRUSH)SelectObject(hdc, g_hGoalBrush);
                break;
            case  obs:
                hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
                break;
            case nodelist:
                hOldBrush = (HBRUSH)SelectObject(hdc, g_hNodeListBrush);
                break;
            case visited:
                //if ((iCntW == g_Dungeon.getStart().x && iCntH == g_Dungeon.getStart().y)
                //    || (iCntW == g_Dungeon.getGoal().x && iCntH == g_Dungeon.getGoal().y))
                //{
                //    break;
                //}
                hOldBrush = (HBRUSH)SelectObject(hdc, g_hVisitedBrush);
                break;
            default:
                break;
            }
            //SelectObject(hdc, GetStockObject(BLACK_BRUSH));
            Rectangle(hdc, iX, iY, iX + g_iGridSize + 1, iY + g_iGridSize + 1);
        }
    }
    SelectObject(hdc, hOldBrush);
}

void RenderObstacle(HDC hdc)
{
    int iX = 0;
    int iY = 0;
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    // 사각형 테두리 부분을 보이게 하기위해 BLACK_BRUSH를 지정한다.
    // CreatPen으로 BLACK_BRUSH을 생성해도 되지만, 
    //GerStockObject를 사용해서 시스템에 미리 만들어진 고정 GDI 객체를 사용한다.
    // GetStock은 시스템의 고정적인 범용 GDI라서 삭제할 필요가 없다.
    //시스템 전역적인 GDI Object를 얻어서 사용한다는 개념

    for (int iCntW = 0; iCntW < GRID_WIDTH;iCntW++)
    {
        for (int iCntH = 0;iCntH < GRID_HEIGHT;iCntH++) 
        {
            if (g_Dungeon.CheckTile(iCntH,iCntW) == obs)
            {
                iX = iCntW * g_iGridSize;
                iY = iCntH * g_iGridSize;
                //테두리 크기가 있으므로 +2 한다.
                Rectangle(hdc, iX, iY, iX + g_iGridSize + 1, iY + g_iGridSize + 1);
            }
        }
    }
    SelectObject(hdc, hOldBrush);
}

void RenderStartGoal(HDC hdc)
{
    if (g_Dungeon._start.x != -1) {
        SelectObject(hdc, g_hStartBrush);
        //Rectangle(hdc, g_AStar._start.x * GRID_SIZE, g_AStar._start.y * GRID_SIZE,
        //    (g_AStar._start.x + 1) * GRID_SIZE, (g_AStar._start.y + 1) * GRID_SIZE);
        int iX = g_Dungeon._start.x * g_iGridSize;
        int iY = g_Dungeon._start.y * g_iGridSize;
        Rectangle(hdc, iX, iY, iX + g_iGridSize + 1, iY + g_iGridSize + 1);
    }
    if (g_Dungeon._goal.x != -1) {
        SelectObject(hdc, g_hGoalBrush);
        int iX = g_Dungeon._goal.x * g_iGridSize;
        int iY = g_Dungeon._goal.y* g_iGridSize;
        Rectangle(hdc, iX, iY, iX + g_iGridSize + 1, iY + g_iGridSize + 1);
    }
}

//방문 가능성 노드, 방문 한 노드 파랗게 랜더링
void RenderAStarList(HDC hdc)
{

    //for (auto node : g_AStar._openlist)
    //{
    //    if (node->pos.y < GRID_HEIGHT)
    //    {
    //        SelectObject(hdc, g_hNodeListBrush);
    //        Rectangle(hdc, node->pos.x * g_iGridSize, node->pos.y * g_iGridSize,
    //            (node->pos.x + 1) * g_iGridSize, (node->pos.y + 1) * g_iGridSize);
    //    }
    //}

    //for (auto node : g_AStar._closelist)
    //{
    //    if (node->pos.y < GRID_HEIGHT)
    //    {
    //        SelectObject(hdc, g_hNodeListBrush);
    //        Rectangle(hdc, node->pos.x * g_iGridSize, node->pos.y * g_iGridSize,
    //            (node->pos.x + 1) * g_iGridSize, (node->pos.y + 1) * g_iGridSize);
    //    }
    //}


    for (auto node : g_AStar._shortestRoutelist)
    {
        if (node->pos.x != -1)
        {
            SelectObject(hdc, g_hAstarAnswerListBrush);
            Rectangle(hdc, node->pos.x * g_iGridSize, node->pos.y * g_iGridSize,
                (node->pos.x + 1) * g_iGridSize, (node->pos.y + 1) * g_iGridSize);
        }
    }
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
                if (iTileX == g_Dungeon.getStart().x && iTileY == g_Dungeon.getStart().y)
                {
                    //g_Dungeon.ChangeTile(iTileY, iTileX, none);
                    g_bErase = false;
                    g_bStartMove = true;
                    g_bGoalMove = false;
                }
                else if (iTileX == g_Dungeon.getGoal().x && iTileY == g_Dungeon.getGoal().y)
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
        g_AStar.findPath();
        InvalidateRect(hWnd, NULL, false);
        break;
    case WM_MOUSEMOVE:
        if (g_bDrag)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            int iTileX = xPos / g_iGridSize;
            int iTileY = yPos / g_iGridSize;
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
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
        g_hStartBrush = CreateSolidBrush(RGB(0, 200, 0));
        g_hGoalBrush = CreateSolidBrush(RGB(200, 0, 0));
        g_hNodeListBrush = CreateSolidBrush(RGB(0, 0, 200));
        g_hVisitedBrush = CreateSolidBrush(RGB(2000, 2000, 2000));
        g_hEmptyBrush = CreateSolidBrush(RGB(500, 500, 500));

        //정답인 노드 덧칠
        g_hAstarAnswerListBrush = CreateSolidBrush(RGB(200, 200, 0));

        //메모리DC 생성 코드
        //윈도우 생성 시 현 윈도우 크기와 동일한 메모리 DC 생성
        HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDC_Rect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDC_Rect.right, g_MemDC_Rect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
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
         RenderAStarList(g_hMemDC);
         //RenderObstacle(g_hMemDC);
         //RenderStartGoal(g_hMemDC);
         RenderMap(g_hMemDC,g_Dungeon);

         //매모리 DC의 이미지를 윈도우 DC에 출력
         hdc = BeginPaint(hWnd, &ps);
         BitBlt(hdc, 0, 0, g_MemDC_Rect.right, g_MemDC_Rect.bottom, g_hMemDC, g_originX, g_originY, SRCCOPY);
         EndPaint(hWnd, &ps);

         break;
     }
     break;
     case WM_SIZE:
     {
         SelectObject(g_hMemDC, g_hMemDCBitmap_old);
         DeleteObject(g_hMemDCBitmap);
         DeleteDC(g_hMemDC);

         HDC hdc = GetDC(hWnd);
         GetClientRect(hWnd, &g_MemDC_Rect);
         g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDC_Rect.right, g_MemDC_Rect.bottom);
         g_hMemDC = CreateCompatibleDC(hdc);
         ReleaseDC(hWnd, hdc);

         g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
     }
     break;
     case WM_MOUSEWHEEL:
     {
         int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

         if (zDelta > 0) g_iGridSize = (int)(g_iGridSize * 1.3f);  // 확대
         else            g_iGridSize = (int)(g_iGridSize * 0.9f);  // 축소

         if (g_iGridSize < 4)   g_iGridSize = 4;   // 최소 크기 제한
         if (g_iGridSize > 128) g_iGridSize = 128; // 최대 크기 제한

         InvalidateRect(hWnd, NULL, TRUE);
     }
     break;
     case WM_KEYDOWN:
         switch (wParam)
         {
         case VK_LEFT:  g_originX -= g_iGridSize; break;  // 화면 오른쪽으로 이동
         case VK_RIGHT: g_originX += g_iGridSize; break;  // 화면 왼쪽으로 이동
         case VK_UP:    g_originY -= g_iGridSize; break;  // 화면 아래로 이동
         case VK_DOWN:   g_originY += g_iGridSize; break;  // 화면 위로 이동
         }
         InvalidateRect(hWnd, NULL, TRUE);
         break;
    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDCBitmap);
        //DeleteObject(g_hMemDC);
        DeleteDC(g_hMemDC);
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

