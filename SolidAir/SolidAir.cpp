#include "framework.h"
#include "SolidAir.h"
#include "schema.h"
#include <iostream>

constexpr auto MAX_LOADSTRING = 100;

// Global Variables
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module
ATOM                RegisterWindowClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

pfcdtInit cdtInit;
pfcdtDraw cdtDraw;
pfcdtDrawEx cdtDrawEx;
pfcdtAnimate cdtAnimate;
pfcdtTerm cdtTerm;

int cdWidth;
int cdHight;
int dist = 0;
GameState gameState = { 0 };

int APIENTRY wWinMain(
        _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    const HMODULE cards = LoadLibraryW(L"cards.dll");

    if (cards == 0)
    {
        std::cerr << "Failed to load cards library";
        
        return FALSE;
    }

    cdtInit = (pfcdtInit)GetProcAddress(cards, "cdtInit");
    cdtDraw = (pfcdtDraw)GetProcAddress(cards, "cdtDraw");
    cdtDrawEx = (pfcdtDrawEx)GetProcAddress(cards, "cdtDrawExt");
    cdtAnimate = (pfcdtAnimate)GetProcAddress(cards, "cdtAnimate");
    cdtTerm = (pfcdtTerm)GetProcAddress(cards, "cdtTerm");

    if (cdtInit == nullptr)
    {
        std::cerr << "Failed to bind to cdtInit";

        return FALSE;
    }

    if (cdtDraw == nullptr)
    {
        std::cerr << "Failed to bind to cdtDraw";

        return FALSE;
    }

    if (cdtDrawEx == nullptr)
    {
        std::cerr << "Failed to bind to cdtDrawEx";

        return FALSE;
    }

    if (cdtAnimate == nullptr)
    {
        std::cerr << "Failed to bind to cdtAnimate";

        return FALSE;
    }

    if (cdtTerm == nullptr)
    {
        std::cerr << "Failed to bind to cdtTerm";

        return FALSE;
    }

    cdtInit(&cdWidth, &cdHight);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SOLIDAIR, szWindowClass, MAX_LOADSTRING);
    RegisterWindowClass(hInstance);

    // initialize the game state
    dist = 100 - cdWidth;

    for (int pi = 0; pi < 7; ++pi)
    {
        if (pi < 4)
        {
            gameState.targetPiles[pi].numCardsOnPile = 0;
            gameState.targetPiles[pi].pos.left = dist + (pi + 3) * (dist + cdWidth);
            gameState.targetPiles[pi].pos.top = dist;
            gameState.targetPiles[pi].pos.right = dist + (pi + 3) * (dist + cdWidth) + cdWidth;
            gameState.targetPiles[pi].pos.bottom = dist + cdHight;
        }

        gameState.dagoPiles[pi].numCardsOnPile = 0;
        gameState.dagoPiles[pi].pos.left = dist + pi * (dist + cdWidth);
        gameState.dagoPiles[pi].pos.top = dist + cdHight + dist;
        gameState.dagoPiles[pi].pos.right = dist + pi * (dist + cdWidth) + cdWidth;
        gameState.dagoPiles[pi].pos.bottom = dist + cdHight + dist + cdHight;
    }

    // Perform application window initialization
    if (!InitInstance (hInstance, nCmdShow))
    {
        std::cerr << "Failed to initialize instance";

        return FALSE;
    }

    // ancient main message loop
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SOLIDAIR));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (cards != 0)
    {
        cdtTerm();

        if (!FreeLibrary(cards))
        {
            std::cerr << "Failed to free cards library";
        }
    }

    return (int)msg.wParam;
}

ATOM RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOLIDAIR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SOLIDAIR);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
    {
        std::cerr << "Failed to create window";

        return false;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }

        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            HBRUSH shrubbery = CreateSolidBrush(RGB(0, 0x80, 0));
            FillRect(hdc, &ps.rcPaint, shrubbery);
            DeleteObject(shrubbery);
            SelectObject(hdc, GetSysColorBrush(DKGRAY_BRUSH));
            
            // the stockpile
            cdtDraw(hdc, dist, dist, Cards::BackDivine, 1, 0);

            for (int pi = 0; pi < 7; ++pi)
            {
                const auto& dpos = gameState.dagoPiles[pi].pos;

                cdtDraw(hdc, dpos.left, dpos.top, pi % 6, 0, 0);
                if (pi < 4)
                {
                    // the target piles
                    const auto& tpos = gameState.targetPiles[pi].pos;

                    Rectangle(
                        hdc,
                        tpos.left,
                        tpos.top,
                        tpos.right,
                        tpos.bottom
                    );
                }
            }

            EndPaint(hWnd, &ps);
        }

        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

/// <summary>
/// About dialog
/// </summary>
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
    
        break;
    }
    
    return (INT_PTR)FALSE;
}
