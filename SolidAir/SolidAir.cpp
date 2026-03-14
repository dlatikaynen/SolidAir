#define SECURITY_WIN32
#include "framework.h"
#include "SolidAir.h"
#include <iostream>
#include <vector>
#include <ranges>
#include <random>
#include <format>
#include <windows.h>
#include <Lmcons.h>
#include <Security.h>
#pragma comment (lib,"Secur32.lib")

constexpr auto MAX_LOADSTRING = 100;
constexpr time_t DRAG_TRESHOLD = 469; // milliseconds

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

pfcdtInit cdtInit;
pfcdtDraw cdtDraw;
pfcdtDrawEx cdtDrawEx;
pfcdtAnimate cdtAnimate;
pfcdtTerm cdtTerm;

int cdWidth;
int cdHight;
int dist = 0;
GameState gameState = {};

// mouse operation
bool hasCapturedTheMouseToDragCards = false;
bool CanInitiateDragHere(POINT p);
POINT lastLeftDownAt;
time_t lastLeftDownBy;
POINT draggedFrom;
POINT dragOffset;
HCURSOR hDragCursor = 0;
CardsBeingHit cardsBeingDragged;
CardsBeingHit cardLastClicked;

// keyboard operation
bool isCtrlPressed = false;
int currentDagopi = -1;

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
    gameState.background = Cards::BackBloodot;
    cardsBeingDragged.fromStockpile = false;
    cardsBeingDragged.pile = nullptr;
    cardsBeingDragged.index = -1;

    gameState.stockpile.numCardsOnPile = 0;
    gameState.stockpile.uncovered = -1;
    gameState.stockpile.pos.left = dist;
    gameState.stockpile.pos.top = dist;
    gameState.stockpile.pos.right = dist + cdWidth;
    gameState.stockpile.pos.bottom = dist + cdHight;
 
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

    InitNewDagobert();

    // application window initialization
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

    // cleanup
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
    WNDCLASSEXW wcex = {0};

    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOLIDAIR));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = MAKEINTRESOURCEW(IDC_SOLIDAIR);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
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

    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    hDragCursor = LoadCursor(NULL, IDC_HAND);

    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameEx(EXTENDED_NAME_FORMAT::NameDisplay, username, &username_len) == 0)
    {
        username[0] = 0;
    }

    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_LBUTTONDOWN:
        {
            const auto& pos = MAKEPOINTS(lParam);
            POINT pixelPos = { pos.x, pos.y };
            
            // remember for drag initiating detection
            lastLeftDownAt.x = pixelPos.x;
            lastLeftDownAt.y = pixelPos.y;
            lastLeftDownBy = GetTickCount64();

            HitTest(&pixelPos, &cardLastClicked);
        }

        break;

    case WM_LBUTTONUP:
        {
            bool handled = false;

            if (hasCapturedTheMouseToDragCards)
            {
                ReleaseCapture();
                hasCapturedTheMouseToDragCards = false;
                handled = true;
            }
            else
            {
                const auto& pos = MAKEPOINTS(lParam);
                POINT pixelPos = { pos.x, pos.y };
                CardsBeingHit stillHit;

                if (HitTest(&pixelPos, &stillHit))
                {
                    if (
                        (stillHit.fromStockpile && cardLastClicked.fromStockpile && stillHit.index != -1 && stillHit.index == cardLastClicked.index)
                        ||
                        (!stillHit.fromStockpile && !cardLastClicked.fromStockpile && stillHit.index != -1 && stillHit.index == cardLastClicked.index && stillHit.pile->pos.left == cardLastClicked.pile->pos.left)
                    )
                    {
                        ClickedOnCard(hWnd);
                        handled = true;
                    }
                }

                // not clicked in any reasonable way - must attempt again
                cardLastClicked.fromStockpile = false;
                cardLastClicked.pile = nullptr;
                cardLastClicked.index = -1;
            }
    
            if (handled)
            {
                return 0;
            }
        }

        break;

    case WM_MOUSEMOVE:
        {
            bool handled = false;

            if (wParam & MK_LBUTTON)
            {
                const auto& pos = MAKEPOINTS(lParam);
                POINT pixelPos = { pos.x, pos.y };

                if (hasCapturedTheMouseToDragCards)
                {
                    dragOffset.x = pixelPos.x - draggedFrom.x;
                    dragOffset.y = pixelPos.y - draggedFrom.y;
                    InvalidateRect(hWnd, NULL, false);
                    handled = true;
                }
                else
                {
                    // we might be about to get enough purchase to initiate a drag
                    int distX = pixelPos.x - lastLeftDownAt.x;
                    int distY = pixelPos.y - lastLeftDownAt.y;
                    bool overTreshold = false;
                    bool didCapture = false;

                    if (distX == 0 && distY == 0)
                    {
                        // timing-based drag treshold is only recognized as long as the mouse did not move
                        overTreshold = (GetTickCount64() - lastLeftDownBy) >= DRAG_TRESHOLD;
                    }

                    if (!overTreshold)
                    {
                        SetCapture(hWnd);
                        didCapture = true;

                        if (distX != 0 || distY != 0)
                        {
                            int cxDrag = GetSystemMetrics(SM_CXDRAG);
                            int cyDrag = GetSystemMetrics(SM_CYDRAG);
                            
                            overTreshold = abs(distX) >= cxDrag || abs(distY) >= cyDrag;
                        }
                    }

                    if (overTreshold)
                    {
                        if (CanInitiateDragHere(pixelPos))
                        {
                            draggedFrom.x = pixelPos.x;
                            draggedFrom.y = pixelPos.y;
                            dragOffset.x = 0;
                            dragOffset.y = 0;
                            hasCapturedTheMouseToDragCards = true;
                            SendMessage(hWnd, WM_SETCURSOR, 0, 0);

                            return 0;
                        }

                        if (didCapture)
                        {
                            ReleaseCapture();
                            handled = true;
                        }
                    }
                }
            }

            if (handled)
            {
                return 0;
            }
        }

        break;

    case WM_KEYDOWN:
        // keyboard operation:
        // 1..7           uncover the dagopile if covered
        // 1..7           select the dagopile as the drag source if already uncovered and not empty
        // 1..7           place the possible move from a previously selected, different dagopile there,
        //                if the source is then empty, set the target as the new selection, otherwise
        //                uncover, otherwise leave the selection at the source
        // <Alt> + 1..7   place the possible move from a previously selected, different dagopile,
        //                and set the destination as the new selected source
        // <Strg>         alone uncovers a covered stockpile, and keyupped without n cycles the stockpile
        // <Strg> + 1..7  place the stockpile top card on that dagopile, keeping any selected source
        //                dagopile unless there is none currently set, then set it
        // <Strg+Alt> + n place the stockpile top card on that dagopile, and set the destination as
        //                the new selected source
        // <Shift> + 1..4 take the card home
        {
            // <Strg> alone uncovers a covered stockpile, and keyupped without n cycles the stockpile
            if (wParam == VK_CONTROL && isCtrlPressed == false)
            {
                isCtrlPressed = true;

                if (UncoverStockpile(hWnd))
                {
                    // can still hit a number to place it, this is just so ctrl+keyup will not cycle
                    isCtrlPressed = false;

                    return 0;
                }
            }

            int newPi = -1;

            if (wParam == VK_NUMPAD1 || wParam == '1')
            {
                newPi = 0;
            }
            if (wParam == VK_NUMPAD2 || wParam == '2')
            {
                newPi = 1;
            }
            if (wParam == VK_NUMPAD3 || wParam == '3')
            {
                newPi = 2;
            }
            if (wParam == VK_NUMPAD4 || wParam == '4')
            {
                newPi = 3;
            }
            if (wParam == VK_NUMPAD5 || wParam == '5')
            {
                newPi = 4;
            }
            if (wParam == VK_NUMPAD6 || wParam == '6')
            {
                newPi = 5;
            }
            if (wParam == VK_NUMPAD7 || wParam == '7')
            {
                newPi = 6;
            }

            if (newPi != -1)
            {
                if (isCtrlPressed)
                {
                    isCtrlPressed = false;

                    // <Strg> + 1..7 place the stockpile top card on that dagopile, keeping any selected source
                    //               dagopile unless there is none currently set, then set it
                    PlaceStockpileOn(hWnd, newPi);

                    if (currentDagopi == -1)
                    {
                        currentDagopi = newPi;
                    }
                }

                return 0;
            }
        }

        break;

    case WM_KEYUP:
        // <Strg> alone uncovers a covered stockpile, and keyupped without n cycles the stockpile
        if (wParam == VK_CONTROL)
        {
            if (isCtrlPressed)
            {
                isCtrlPressed = false;

                if (CycleStockpile(hWnd))
                {
                    return 0;
                }
            }
        }

        break;

    case WM_SETCURSOR:
        if (hasCapturedTheMouseToDragCards)
        {
            SetCursor(hDragCursor);

            return true;
        }

        break;

    case WM_ERASEBKGND:
        // indicate that we handle it
        return 1;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC rawDc = BeginPaint(hWnd, &ps);
            HDC hdc = CreateCompatibleDC(rawDc);
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            HBITMAP memBitmap = CreateCompatibleBitmap(rawDc, width, height);
            SelectObject(hdc, memBitmap);

            const auto& bkColor = RGB(0, 0x80, 0);
            SetBkColor(hdc, bkColor);
            HBRUSH shrubbery = CreateSolidBrush(bkColor);
            HPEN hDashPen = CreatePen(PS_DASH, 1, RGB(0, 0, 0));
            FillRect(hdc, &ps.rcPaint, shrubbery);
            DeleteObject(shrubbery);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetSysColorBrush(GRAY_BRUSH));

            for (int pi = 0; pi < 7; ++pi)
            {
                // the target piles
                if (pi < 4)
                {
                    const auto& tpile = gameState.targetPiles[pi];
                    const auto& tpos = tpile.pos;

                    if (tpile.numCardsOnPile == 0)
                    {
                        // this target pile is empty
                        Rectangle(
                            hdc,
                            tpos.left,
                            tpos.top,
                            tpos.right,
                            tpos.bottom
                        );
                    }
                    else
                    {
                        // we only draw the visible top card. the others are considered to be beneath it and covered
                        const auto& tpileCard = tpile.pile[tpile.numCardsOnPile - 1];

                        cdtDraw(hdc, tpos.left, tpos.top, tpileCard, 0, 0);
                    }
                }

                // the dagobert piles
                const auto& dpile = gameState.dagoPiles[pi];
                const auto& dpos = dpile.pos;

                if (dpile.numCardsOnPile == 0)
                {
                    // this dago pile is empty
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hDashPen);
                    HBRUSH prevBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    Rectangle(
                        hdc,
                        dpos.left,
                        dpos.top,
                        dpos.right,
                        dpos.bottom
                    );

                    SelectObject(hdc, prevBrush);
                    SelectObject(hdc, hOldPen);
                }
                else
                {
                    // the stack
                    for (int i = 0; i < dpile.numCardsOnPile; ++i)
                    {
                        const auto& dpileCard = dpile.pile[i];

                        cdtDraw(
                            hdc,
                            dpos.left,
                            dpos.top + i * dist,
                            i < dpile.uncoveredFrom ? gameState.background : dpileCard,
                            0,
                            0
                        );
                    }
                }
            }

            // the stockpile - last so it is on top of everything else
            if (gameState.stockpile.uncovered == -1)
            {
                cdtDraw(hdc, dist, dist, gameState.background, 1, 0);
            }
            else
            {
                const auto& uncoveredCard = gameState.stockpile.pile[gameState.stockpile.uncovered];

                if (hasCapturedTheMouseToDragCards)
                {
                    // if the card being dragged is already off the pile, even slightly,
                    // only then we draw the next card (or the empty pile indication) underneath
                    if (dragOffset.x != 0 || dragOffset.y != 0)
                    {
                        if (gameState.stockpile.numCardsOnPile == 1)
                        {
                            Rectangle(
                                hdc,
                                gameState.stockpile.pos.left,
                                gameState.stockpile.pos.top,
                                gameState.stockpile.pos.right,
                                gameState.stockpile.pos.bottom
                            );
                        }
                        else if (gameState.stockpile.numCardsOnPile > 1)
                        {
                            // > 1 because the card currently being dragged, still counts as top of the pile until it is placed
                            int nextStockpileIndex = gameState.stockpile.uncovered + 1;

                            if (nextStockpileIndex == gameState.stockpile.numCardsOnPile)
                            {
                                nextStockpileIndex = 0;
                            }

                            const auto& nextStockpileCard = gameState.stockpile.pile[nextStockpileIndex];

                            cdtDraw(hdc, gameState.stockpile.pos.left, gameState.stockpile.pos.top, nextStockpileCard, 1, 0);
                        }
                    }

                    cdtDraw(hdc, gameState.stockpile.pos.left + dragOffset.x, gameState.stockpile.pos.top + dragOffset.y, uncoveredCard, 1, 0);
                }
                else
                {
                    cdtDraw(hdc, gameState.stockpile.pos.left, gameState.stockpile.pos.top, uncoveredCard, 1, 0);
                }
            }

            SelectObject(hdc, oldBrush);
            DeleteObject(hDashPen);
            BitBlt(rawDc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
            DeleteObject(memBitmap);
            DeleteDC(hdc);
            EndPaint(hWnd, &ps);
        }

        return true;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                
                return true;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                
                return true;
            }
        }

        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitNewDagobert()
{
    // we have 52 shuffled cards. thereof, we initialize the first dpile with 1
    // up to the seventh dpile with seven.
    // the remaining cards are placed on the stockpile and covered
    // I ♥ C++
    auto range_view = std::ranges::views::iota(0, 51);
    std::vector<int> v(std::ranges::begin(range_view), std::ranges::end(range_view));
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);

    int i = 0;
    for (int card : v)
    {
        int pi = 0;

        if (i >= 1 && i <= 2)
        {
            pi = 1;
        }
        else if (i >= 3 && i <= 5)
        {
            pi = 2;
        }
        else if (i >= 6 && i <= 9)
        {
            pi = 3;
        }
        else if (i >= 10 && i <= 14)
        {
            pi = 4;
        }
        else if (i >= 15 && i <= 20)
        {
            pi = 5;
        }
        else if (i >= 21 && i <= 27)
        {
            pi = 6;
        }
        
        if (i > 27)
        {
            gameState.stockpile.numCardsOnPile++;
            gameState.stockpile.pile[gameState.stockpile.numCardsOnPile - 1] = (Cards)card;
        }
        else
        {
            gameState.dagoPiles[pi].numCardsOnPile++;
            gameState.dagoPiles[pi].pile[gameState.dagoPiles[pi].numCardsOnPile - 1] = (Cards)card;
            gameState.dagoPiles[pi].uncoveredFrom = gameState.dagoPiles[pi].numCardsOnPile - 1;
        }

        ++i;
    }
}

bool CanInitiateDragHere(POINT p)
{
    OutputDebugString(std::format(L"determining whether there is a draggable card at {0}, {1}\n", p.x, p.y).c_str());
    
    if (HitTest(&p, &cardsBeingDragged))
    {
        // card must be available and uncovered in order to drag it
        if (cardsBeingDragged.fromStockpile)
        {
            return gameState.stockpile.numCardsOnPile > 0 && gameState.stockpile.uncovered != -1;
        }

        return cardsBeingDragged.pile->uncoveredFrom <= cardsBeingDragged.index;
    }

    return false;
}

bool HitTest(POINT *p, CardsBeingHit* cards)
{
    cards->fromStockpile = false;
    cards->pile = nullptr;
    cards->index = -1;

    // (1) is it the stockpile?
    const auto& spPos = gameState.stockpile.pos;
    const auto& stockPile = RECT{ spPos.left, spPos.top, spPos.right, spPos.bottom };

    if (PtInRect(&stockPile, *p))
    {
        // is there a card on the stockpile?
        if (gameState.stockpile.numCardsOnPile != 0)
        {
            // take this one!
            cards->fromStockpile = true;
            cards->pile = nullptr;
            cards->index = gameState.stockpile.numCardsOnPile - 1;

            return true;
        }
    }

    return false;
}

void ClickedOnCard(HWND hWnd)
{
    if (cardLastClicked.fromStockpile && gameState.stockpile.numCardsOnPile != 0)
    {
        const auto& spPos = gameState.stockpile.pos;
        const auto& stockPile = RECT{ spPos.left, spPos.top, spPos.right, spPos.bottom };

        if (gameState.stockpile.uncovered == -1)
        {
            UncoverStockpile(hWnd);
        }
        else
        { 
            // repeatedly clicking here cycles it
            CycleStockpile(hWnd);
        }
    }

}

bool UncoverStockpile(HWND hWnd)
{
    if (gameState.stockpile.numCardsOnPile != 0 && gameState.stockpile.uncovered == -1)
    {
        const auto& spPos = gameState.stockpile.pos;
        const auto& stockPile = RECT{ spPos.left, spPos.top, spPos.right, spPos.bottom };

        gameState.stockpile.uncovered = 0;

        InvalidateRect(hWnd, &stockPile, false);

        return true;
    }

    return false;
}

bool CycleStockpile(HWND hWnd)
{
    // can't cycle on a single card, can't cycle until uncovered
    if (gameState.stockpile.numCardsOnPile > 1 && gameState.stockpile.uncovered != -1)
    {
        ++gameState.stockpile.uncovered;

        // cycle through them
        if (gameState.stockpile.uncovered == gameState.stockpile.numCardsOnPile)
        {
            gameState.stockpile.uncovered = 0;
        }

        const auto& spPos = gameState.stockpile.pos;
        const auto& stockPile = RECT{ spPos.left, spPos.top, spPos.right, spPos.bottom };

        InvalidateRect(hWnd, &stockPile, false);
        
        return true;
    }

    return false;
}

bool PlaceStockpileOn(HWND hWnd, int pi)
{
    if (gameState.stockpile.uncovered != -1 && gameState.stockpile.numCardsOnPile >= 1)
    {
        const auto& card = gameState.stockpile.pile[gameState.stockpile.uncovered];

        // now, can it be placed there?
        auto& tP = gameState.dagoPiles[pi];

        // add it to the target pile
        tP.pile[tP.numCardsOnPile] = card;
        tP.numCardsOnPile++;

        // remove it from the stockpile
        if (gameState.stockpile.numCardsOnPile == 1)
        {
            // our stockpile is now completely empty
            gameState.stockpile.numCardsOnPile = 0;
            gameState.stockpile.uncovered = -1;
        }
        else
        {
            if (gameState.stockpile.uncovered == gameState.stockpile.numCardsOnPile - 1)
            {
                // wraparound
                gameState.stockpile.uncovered = 0;
            }
            else
            {
                for (int i = gameState.stockpile.uncovered + 1; i < gameState.stockpile.numCardsOnPile; ++i)
                {
                    gameState.stockpile.pile[i - 1] = gameState.stockpile.pile[i];
                }
            }

            --gameState.stockpile.numCardsOnPile;
        }

        // redraw the stockpile
        const auto& spPos = gameState.stockpile.pos;
        const auto& stockPile = RECT{ spPos.left, spPos.top, spPos.right, spPos.bottom };

        InvalidateRect(hWnd, &stockPile, false);

        // redraw the target pile
        const auto& tpPos = tP.pos;
        const auto& targetPile = RECT{ tpPos.left, tpPos.top, tpPos.right, tpPos.bottom + (tP.numCardsOnPile - 1) * dist };

        InvalidateRect(hWnd, &targetPile, false);

        return true;
    }

    return false;
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
