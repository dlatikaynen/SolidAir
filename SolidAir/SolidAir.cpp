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

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        std::cerr << "Failed to initialize instance";

        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SOLIDAIR));
    MSG msg;

    // ancient main message loop
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
            const auto dist = 100 - cdWidth;
            cdtDraw(hdc, dist, dist, 0, 1, 0);
            cdtDraw(hdc, dist + 0 * (dist + cdWidth), dist + cdHight + dist, 0, 1, 0);
            cdtDraw(hdc, dist + 1 * (dist + cdWidth), dist + cdHight + dist, 1, 1, 0);
            cdtDraw(hdc, dist + 2 * (dist + cdWidth), dist + cdHight + dist, 2, 1, 0);
            cdtDraw(hdc, dist + 3 * (dist + cdWidth), dist + cdHight + dist, 3, 1, 0);
            cdtDraw(hdc, dist + 4 * (dist + cdWidth), dist + cdHight + dist, 4, 1, 0);
            cdtDraw(hdc, dist + 5 * (dist + cdWidth), dist + cdHight + dist, 0, 1, 0);
            cdtDraw(hdc, dist + 6 * (dist + cdWidth), dist + cdHight + dist, 1, 1, 0);

            // the target piles
            SelectObject(hdc, GetSysColorBrush(DKGRAY_BRUSH));
            Rectangle(
                hdc,
                dist + 3 * (dist + cdWidth),
                dist, dist + 3 * (dist + cdWidth) + cdWidth,
                dist + cdHight
            );

            Rectangle(
                hdc,
                dist + 4 * (dist + cdWidth),
                dist, dist + 4 * (dist + cdWidth) + cdWidth,
                dist + cdHight
            );

            Rectangle(
                hdc,
                dist + 5 * (dist + cdWidth),
                dist, dist + 5 * (dist + cdWidth) + cdWidth,
                dist + cdHight
            );

            Rectangle(
                hdc,
                dist + 6 * (dist + cdWidth),
                dist, dist + 6 * (dist + cdWidth) + cdWidth,
                dist + cdHight
            );

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
