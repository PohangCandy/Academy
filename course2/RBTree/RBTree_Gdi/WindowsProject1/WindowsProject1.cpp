 //WindowsProject1.cpp : 애플리케이션에 대한 진입점을 정의합니다.


//누수 확인
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "framework.h"
#include "WindowsProject1.h"
#include "windowsx.h"
#include "RBTree.h"

#define MAX_LOADSTRING 100
#define GRID_SIZE 64
#define GRID_WIDTH 100
#define GRID_HEIGHT 100

//#define IDD_INPUTDIALOG                 1000
//#define IDC_INPUT_EDIT                  1001
//#define IDC_INSERT_BUTTON               1002 // 사용하지 않더라도 정의해두면 좋음
//#define IDC_REMOVE_BUTTON               1003 // 사용하지 않더라도 정의해두면 좋음
//#define IDC_STATIC                      -1   

int g_iGridSize = GRID_SIZE;

// 시각화 드로잉 상수 (g_iGridSize는 노드 크기 결정에만 사용)
const int NODE_RADIUS = 20; // 고정된 노드 반지름 (또는 g_iGridSize/2)
const int V_SPACE = 60;     // 수직 간격
const int H_SPACE_INITIAL = 500; // 루트 레벨의 초기 수평 간격 (트리의 너비 결정)

RBTree g_rbt;

//나중에 출발지와 목적지를 겹치게 두면 출발지를 먼저 옮길 수 있는 예외처리도 해줘야 할 듯

HBRUSH g_hEmptyBrush;
HBRUSH g_hTileBrush;

HPEN g_hGridPen;

HFONT g_hDisplayFont = NULL; // 전역 폰트 핸들
HFONT g_hOldFont = NULL;


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

INT_PTR CALLBACK InputDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void RecreateFont(HWND hWnd)
{
    if (g_hDisplayFont) DeleteObject(g_hDisplayFont);

    int fontHeight = -14;
    g_hDisplayFont = CreateFont(
        fontHeight, 0, 0, 0, 0, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial"
    );
}

// WindowsProject1.cpp에 추가

// 재귀적으로 노드를 그리고 위치를 결정하는 함수
void DrawNodeRecursive(HDC hdc, stNODE* curNode, int x, int y, int xOffset)
{
    // Nil 노드 검사: g_rbt.getNill()을 사용하여 Nil 노드인지 확인
    if (curNode == nullptr || curNode == g_rbt.getNill())
        return;

    // --- 1. 간선 그리기 ---
    int childY = y + V_SPACE;
    // XOffset 계산 수정: 다음 레벨의 간격을 2/3로 줄입니다. (더 느리게 좁아짐)
  // 또한, 노드 반지름보다 작아지지 않도록 최소값을 보장합니다.
    int nextXOffset = xOffset / 2;

    // 최소 X 간격(노드 반지름의 3배)을 보장하여 겹침을 방지
    const int MIN_H_SPACE = NODE_RADIUS * 3;
    if (nextXOffset < MIN_H_SPACE) {
        nextXOffset = MIN_H_SPACE;
    }

    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100)); // 간선 펜
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // 왼쪽 자식 간선
    if (curNode->pLeft != g_rbt.getNill())
    {
        int leftChildX = x - nextXOffset;
        MoveToEx(hdc, x, y + NODE_RADIUS, NULL);
        LineTo(hdc, leftChildX, childY - NODE_RADIUS);
        // 재귀 호출 (먼저 자식을 호출하여 배경에 선이 그려지도록 합니다)
        DrawNodeRecursive(hdc, curNode->pLeft, leftChildX, childY, nextXOffset);
    }

    // 오른쪽 자식 간선
    if (curNode->pRight != g_rbt.getNill())
    {
        int rightChildX = x + nextXOffset;
        MoveToEx(hdc, x, y + NODE_RADIUS, NULL);
        LineTo(hdc, rightChildX, childY - NODE_RADIUS);
        // 재귀 호출
        DrawNodeRecursive(hdc, curNode->pRight, rightChildX, childY, nextXOffset);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    // --- 2. 현재 노드 그리기 (노드 색상/데이터 표시) ---

    // 노드 색상 설정 (RED 또는 BLACK)
    COLORREF color = (curNode->Color == RED) ? RGB(255, 0, 0) : RGB(0, 0, 0);

    HBRUSH hBrush = CreateSolidBrush(color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    // 원 그리기
    Ellipse(hdc, x - NODE_RADIUS, y - NODE_RADIUS, x + NODE_RADIUS, y + NODE_RADIUS);

    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);

    // 3. 노드 데이터 텍스트 그리기
    HFONT hOldFont = (HFONT)SelectObject(hdc, g_hDisplayFont);
    SetTextColor(hdc, (curNode->Color == BLACK) ? RGB(255, 255, 255) : RGB(255, 255, 255)); // 텍스트 색상
    SetBkMode(hdc, TRANSPARENT);

    char buffer[16];
    _itoa_s(curNode->iData, buffer, 16, 10);

    RECT rect = { x - NODE_RADIUS, y - NODE_RADIUS, x + NODE_RADIUS, y + NODE_RADIUS };
    DrawTextA(hdc, buffer, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
}

// 맵 테두리를 그리고 트리를 그리는 메인 함수
void DrawRBTree(HDC hdc, RBTree* tree)
{
    // (RenderMapBoundary 함수가 WM_PAINT에서 호출된다고 가정하고 여기서는 트리만 그립니다)

    stNODE* root = tree->getRoot();

    // 루트 노드가 Nil 노드가 아니면 트리를 그립니다.
    if (root != tree->getNill())
    {
        // 맵의 중앙을 루트 노드의 X 시작점으로 설정
        const int worldW = GRID_WIDTH * g_iGridSize;
        int mapCenterX = worldW / 2;
        int startY = 100;     // 루트 노드의 Y 좌표

        // 재귀 드로잉 시작
        DrawNodeRecursive(hdc, root, mapCenterX, startY, H_SPACE_INITIAL);
    }
}

void RenderMapBoundary(HDC hdc)
{
    // 맵의 논리적 크기
    const int worldW = GRID_WIDTH * g_iGridSize;
    const int worldH = GRID_HEIGHT * g_iGridSize;

    // 펜과 브러시 설정
    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen); // 기존 펜 사용
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH)); // 채우기 없음

    // 논리적 맵 영역 전체에 사각형을 그립니다. (0, 0)에서 (worldW, worldH)까지
    Rectangle(hdc, 0, 0, worldW, worldH);

    // 사용했던 GDI 객체를 복원합니다.
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
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
    // 1. 메모리 누수 감지 플래그 설정 (동일)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // 2. 수정: 모든 보고서 모드를 디버그 출력으로 강제 설정

    // ERROR와 ASSERT는 디버그 창으로 바로 보냅니다.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    // 경고(WARN) 메시지(여기에 누수 보고서가 포함됨)도 디버그 창으로 보냅니다.
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

    _CrtSetBreakAlloc(224); // 여기에 누수 보고서의 번호(224)를 넣어줍니다.

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
    g_rbt.clear();
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
        //삽입
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        int command = 0;

        // 왼쪽 마우스 버튼 (WM_LBUTTONDOWN) -> 삽입
        if (message == WM_LBUTTONDOWN) {
            command = IDC_INSERT_BUTTON;
        }
        // 오른쪽 마우스 버튼 (WM_RBUTTONDOWN) -> 삭제
        else if (message == WM_RBUTTONDOWN) {
            command = IDC_REMOVE_BUTTON;
        }

        if (command != 0) {
            // DialogBoxParam을 사용하여 Dialog를 띄우고 어떤 명령인지 전달합니다.
            DialogBoxParam(
                (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), // 인스턴스 핸들
                MAKEINTRESOURCE(IDD_INPUTDIALOG),                   // Dialog 리소스 ID
                hWnd,                                               // 부모 윈도우
                InputDialogProc,                                    // Dialog 프로시저 (아래 4번 참고)
                (LPARAM)command                                     // GWLP_USERDATA에 command 값 전달
            );

            // Dialog가 닫힌 후 화면 갱신을 요청합니다.
            InvalidateRect(hWnd, NULL, TRUE);
        }
    }
    break;
    case WM_CREATE:
    {
        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
        g_hEmptyBrush = CreateSolidBrush(RGB(255, 255, 255));

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
          
         
         RenderMapBoundary(g_hMemDC);
         DrawRBTree(g_hMemDC, &g_rbt);

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
             g_rbt.clear();
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
        DeleteObject(g_hTileBrush);

        DeleteObject(g_hEmptyBrush);


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

// 입력 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK InputDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int commandType = 0; // Dialog가 유지되는 동안 commandType을 저장할 변수

    switch (message)
    {
    case WM_INITDIALOG:
        // WM_INITDIALOG에서만 lParam에 command 값이 들어있습니다.
        commandType = (int)lParam;

        // Dialog 제목 변경: Insert/Remove에 따라
        if (commandType == IDC_INSERT_BUTTON) {
            SetWindowText(hDlg, L"Insert Node");
        }
        else if (commandType == IDC_REMOVE_BUTTON) {
            SetWindowText(hDlg, L"Remove Node");
        }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                TCHAR szText[32];
                GetDlgItemText(hDlg, IDC_INPUT_EDIT, szText, 32);
                int data = _wtoi(szText);

                // 저장된 commandType 사용
                if (commandType == IDC_INSERT_BUTTON) {
                    g_rbt.Insert(data);
                }
                else if (commandType == IDC_REMOVE_BUTTON) {
                    g_rbt.Remove(data);
                }
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

