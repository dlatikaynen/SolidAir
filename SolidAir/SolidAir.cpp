/*
Registry.SetValue(@"HKEY_CURRENT_USER\Software\Classes\en-software.solidair.v1\shell\open\command", null, @"c:\path\to\SolidAir.exe \"%1\"");
Registry.SetValue(@"HKEY_CURRENT_USER\Software\Classes\.blerg", null, "en-software.solidair.v1");
*/

#define SECURITY_WIN32
#define SDL_MAIN_USE_CALLBACKS 1 

#include "framework.h"
#include "SolidAir.h"
#include "audio.h"
#include "DialogAbout.h"
#include "DialogSavePrompt.h"
#include <stack>
#include <vector>
#include <iostream>
#include <fstream>
#include <ranges>
#include <random>
#include <format>
#include <windows.h>
#include <Lmcons.h>
#include <shellapi.h>
#include <commctrl.h>
#include <Security.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#pragma comment (lib,"Secur32.lib")
#pragma comment (lib,"SDL3.lib")
#pragma comment (lib,"SDL3_mixer.lib")
#pragma comment (lib,"Shell32.lib")
#pragma comment (lib,"Comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

constexpr time_t DRAG_TRESHOLD = 469; // milliseconds
constexpr const wchar_t* SettingsFilename = L"solidair.ligma";
constexpr const wchar_t* SavegameExtension = L"*.solidair";

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

pfcdtInit cdtInit;
pfcdtDraw cdtDraw;
pfcdtDrawEx cdtDrawEx;
pfcdtAnimate cdtAnimate;
pfcdtName cdtName;
pfcdtTerm cdtTerm;

int cdWidth;
int cdHight;
int dist = 0;

// settings and savegame
wchar_t lastSavedAsFilename[MAX_PATH] = { 0 };
bool dirty = false;
BackgroundColors backgroundColor = BackgroundColors::Default;
Cards cardBackside = Cards::Empty;
FrontendLanguage language = FrontendLanguage::unknown;
bool soundFxOn = false;
bool musicOn = false;
bool commentaryOn = false;
GameState gameState = {};
bool winningAnimationStage = 0;
long winningAnimationTicks = 0;

// gamestate is so cheap that we can do an undo buffer as a deque of snapshots.
// a deque... get it?
std::stack<GameState> undoBuffer;
std::stack<GameState> redoBuffer;

// audio
bool isAudioAvailable = false;
static MIX_Mixer* mixer = NULL;
static void* audioData1 = NULL;
static MIX_Track* track1 = NULL;

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
bool isAltPressed = false;
bool isShiftPressed = false;
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
    cdtName = (pfcdtName)GetProcAddress(cards, "cdtName");
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

    if (cdtName == nullptr)
    {
        // not having this is ignored since it is a later addition
        std::cerr << "Failed to bind to cdtName";
    }

    if (cdtTerm == nullptr)
    {
        std::cerr << "Failed to bind to cdtTerm";

        return FALSE;
    }

    cdtInit(&cdWidth, &cdHight);
    if (MIX_Init())
    {
        isAudioAvailable = true;
        
        MIX_Audio* music = NULL;
        MIX_Audio* sound = NULL;
        
        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        if (!mixer)
        {
            SDL_Log("Could not create audio mixer: %s", SDL_GetError());
            
            isAudioAvailable = false;
        }

        music = LoadBackgroundMusic(mixer, IDR_MUSIC_HAPPY, &audioData1);
        if (!music) 
        {
            isAudioAvailable = false;
        }

        track1 = MIX_CreateTrack(mixer);
        if (!track1)
        {
            SDL_Log("Could not create a mixer track: %s", SDL_GetError());
            
            isAudioAvailable = false;
        }

        if (isAudioAvailable)
        {
            MIX_SetTrackAudio(track1, music);
            MIX_SetTrackGain(track1, 0);
            const auto options = SDL_CreateProperties();

            if (!options)
            {
                SDL_Log("Could not create audio playback options: %s", SDL_GetError());
                
                isAudioAvailable = false;
            }

            if (isAudioAvailable)
            {
                /* loop forever */
                SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, -1);

                /* start the audio playing */
                MIX_PlayTrack(track1, options);
                SDL_DestroyProperties(options);
            }
        }
    }
    else
    {
        std::cerr << "Could not initialize the SDL_mixer audio library: " << SDL_GetError();
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SOLIDAIR, szWindowClass, MAX_LOADSTRING);
    RegisterWindowClass(hInstance);

    // initialize the game state
    dist = 100 - cdWidth;
    cardsBeingDragged.fromStockpile = false;
    cardsBeingDragged.pile = nullptr;
    cardsBeingDragged.index = -1;

    gameState.stockpile.pos.left = dist;
    gameState.stockpile.pos.top = dist;
    gameState.stockpile.pos.right = dist + cdWidth;
    gameState.stockpile.pos.bottom = dist + cdHight;
 
    for (int pi = 0; pi < 7; ++pi)
    {
        if (pi < 4)
        {
            gameState.targetPiles[pi].pos.left = dist + (pi + 3) * (dist + cdWidth);
            gameState.targetPiles[pi].pos.top = dist;
            gameState.targetPiles[pi].pos.right = dist + (pi + 3) * (dist + cdWidth) + cdWidth;
            gameState.targetPiles[pi].pos.bottom = dist + cdHight;
        }

        gameState.dagoPiles[pi].pos.left = dist + pi * (dist + cdWidth);
        gameState.dagoPiles[pi].pos.top = dist + cdHight + dist;
        gameState.dagoPiles[pi].pos.right = dist + pi * (dist + cdWidth) + cdWidth;
        gameState.dagoPiles[pi].pos.bottom = dist + cdHight + dist + cdHight;
    }

    // application window initialization
    HWND hWnd;

    if (!InitInstance(hInstance, nCmdShow, &hWnd))
    {
        std::cerr << "Failed to initialize instance";

        return FALSE;
    }

    NewGame(hWnd);

    if (isAudioAvailable)
    {
        MIX_SetTrackGain(track1, musicOn ? 0.8 : 0);
    }
    else
    {
        const auto& hMenu = GetMenu(hWnd);

        EnableMenuItem(hMenu, ID_SETTINGS_BACKGROUNDMUSIC, MF_DISABLED);
        EnableMenuItem(hMenu, ID_SETTINGS_SOUNDFX, MF_DISABLED);
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

    if (isAudioAvailable)
    {
        MIX_Quit();
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
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SOLIDAIR));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = MAKEINTRESOURCEW(IDC_SOLIDAIR);
    wcex.lpszClassName = szWindowClass;
 
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND *createdHwnd)
{
    hInst = hInstance;

    INITCOMMONCONTROLSEX cc;

    cc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    cc.dwICC = ICC_STANDARD_CLASSES | ICC_LINK_CLASS;

    if (!InitCommonControlsEx(&cc))
    {
        std::cerr << "Failed to initialize common controls library: " << GetLastError();
    }

    int width = 7 * (cdWidth + dist) + dist;
    int hight = 4 * (cdHight + dist) + dist;
    RECT rect = { 0, 0, width, hight };

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, true);
    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
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

    // give the toplevel popup menu items an id... windows...
    HMENU hMenu = GetMenu(hWnd);
    MENUITEMINFO mii;
    ZeroMemory(&mii, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID;
    mii.wID = ID_GAME;
    SetMenuItemInfo(hMenu, 0, true, &mii);
    mii.wID = ID_PLAY;
    SetMenuItemInfo(hMenu, 1, true, &mii);
    mii.wID = ID_SETTINGS;
    SetMenuItemInfo(hMenu, 2, true, &mii);
    mii.wID = ID_HELP;
    SetMenuItemInfo(hMenu, 3, true, &mii);

    hMenu = GetSubMenu(hMenu, 2); // settings popup
    mii.wID = ID_CARDBACK;
    SetMenuItemInfo(hMenu, 9, true, &mii);
    mii.wID = ID_BACKGROUNDCOLOR;
    SetMenuItemInfo(hMenu, 10, true, &mii);

    // on a first start, sense language from environment
    if (language == FrontendLanguage::unknown)
    {
        WCHAR lcid[MAX_LOADSTRING] = {};
        LoadStringW(hInstance, IDS_LCID, lcid, MAX_LOADSTRING);

        if (wcsncmp(lcid, L"de", MAX_LOADSTRING) == 0)
        {
            SetLanguage(hWnd, FrontendLanguage::de);
        }
        else if (wcsncmp(lcid, L"fi", MAX_LOADSTRING) == 0)
        {
            SetLanguage(hWnd, FrontendLanguage::fi);
        }
        else if (wcsncmp(lcid, L"ua", MAX_LOADSTRING) == 0)
        {
            SetLanguage(hWnd, FrontendLanguage::ua);
        }
        else 
        {
            SetLanguage(hWnd, FrontendLanguage::en);
        }
    }

    LoadSettings(hWnd);

    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED);
    ShowWindow(hWnd, nCmdShow);
    hDragCursor = LoadCursor(NULL, IDC_HAND);

    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;

    if (GetUserNameEx(EXTENDED_NAME_FORMAT::NameDisplay, username, &username_len) == 0)
    {
        username[0] = 0;
    }

    *createdHwnd = hWnd;

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
            // [dlatikay 20260315] do not listen to non-clientarea clicks (menu)
            if (pixelPos.x >= 0 && pixelPos.y >= 0)
            {
                lastLeftDownAt.x = pixelPos.x;
                lastLeftDownAt.y = pixelPos.y;
                lastLeftDownBy = GetTickCount64();

                HitTest(&pixelPos, &cardLastClicked);
            }
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
                        (stillHit.fromStockpile && cardLastClicked.fromStockpile && stillHit.index == cardLastClicked.index)
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
                 
                    // optimization potential: we can invalidate just the areas that are covered by the drag
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
        // <Shift> + 1..4 take the card home (also works directly from stockpile, if there is a fitting
        //                one, and there is no selected source dagopile or selected is not eligible
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

            // keep track of the other two modifier keys
            if ((wParam == VK_SHIFT || wParam == VK_RSHIFT) && isShiftPressed == false)
            {
                isShiftPressed = true;
            }
            else if ((wParam == VK_LMENU || wParam == VK_RMENU) && isAltPressed == false)
            {
                isAltPressed = true;
            }

            int newPi = -1;

            if (wParam == VK_NUMPAD1 || wParam == '1')
            {
                newPi = 0;
            }
            else if (wParam == VK_NUMPAD2 || wParam == '2')
            {
                newPi = 1;
            }
            else if (wParam == VK_NUMPAD3 || wParam == '3')
            {
                newPi = 2;
            }
            else if (wParam == VK_NUMPAD4 || wParam == '4')
            {
                newPi = 3;
            }
            else if (wParam == VK_NUMPAD5 || wParam == '5')
            {
                newPi = 4;
            }
            else if (wParam == VK_NUMPAD6 || wParam == '6')
            {
                newPi = 5;
            }
            else if (wParam == VK_NUMPAD7 || wParam == '7')
            {
                newPi = 6;
            }
            else if (wParam == VK_NUMPAD0 || wParam == '0')
            {
                SetSelectedDago(hWnd, -1);
            }
            else if (isShiftPressed && (wParam == VK_OEM_5 || wParam == VK_SPACE))
            {
                // german layout: top left key left to the 1 (degree symbol)
                // shift+space (or shift+°) means to move the first eligible card
                // to the first target that is accepting it
                AutoMoveToTarget(hWnd);
            }

            if (newPi != -1)
            {
                if (isCtrlPressed)
                {
                    // <Strg> + 1..7 place the stockpile top card on that dagopile, keeping any selected source
                    //               dagopile unless there is none currently set, then set it
                    PlaceStockpileOnDago(hWnd, newPi);

                    // <Strg+Alt> + n place the stockpile top card on that dagopile, and set the destination as
                    //                the new selected source
                    if (currentDagopi == -1 || isAltPressed)
                    {
                        SetSelectedDago(hWnd, newPi);
                    }

                    isCtrlPressed = false;
                }
                else if (isAltPressed)
                {
                    // <Alt> + 1..7 place the possible move from a previously selected, different dagopile,
                    //              and set the destination as the new selected source
                    PlaceStockpileOnDago(hWnd, newPi);
                    SetSelectedDago(hWnd, newPi);
                }
                else if (isShiftPressed && newPi <= 4)
                {
                    // <Shift> + 1..4 take the card home (also works directly from stockpile, if there is a fitting
                    //                one, and there is no selected source dagopile or selected is not eligible
                    if (currentDagopi != -1)
                    {
                        // source is dago last one
                        if (PlaceDagoOnTarget(hWnd, currentDagopi, newPi))
                        {
                            return 0;
                        }
                    }

                    // fallback: we also attempt this when we are on a selected pile that cannot go to target
                    PlaceStockpileOnTarget(hWnd, newPi);
                }
                else
                {
                    // 1..7 uncover the dagopile if covered
                    // 1..7 select the dagopile as the drag source if already uncovered and not empty
                    // 1..7 place the possible move from a previously selected, different dagopile there,
                    //      if the source is then empty, set the target as the new selection, otherwise
                    //      uncover, otherwise leave the selection at the source
                    auto& dP = gameState.dagoPiles[newPi];
                    bool mustRedraw = false;

                    if (dP.uncoveredFrom >= dP.numCardsOnPile && dP.numCardsOnPile > 0)
                    {
                        dP.uncoveredFrom = dP.numCardsOnPile - 1;
                        mustRedraw = true;
                    }

                    if (newPi != currentDagopi)
                    {
                        if (currentDagopi == -1)
                        {
                            SetSelectedDago(hWnd, newPi);
                        }
                        else
                        {
                            // can we move something to here?
                            const int& grabAt = ProposeMaximumMove(currentDagopi, newPi);

                            if (grabAt != -1)
                            {
                                if (Move(hWnd, currentDagopi, grabAt, newPi))
                                {
                                    mustRedraw = false;
                                }
                            }
                            else
                            {
                                // cannot move cards. at least move selection
                                SetSelectedDago(hWnd, newPi);
                            }
                        }
                    }

                    // if the addressed pile changed and we did not yet redraw, redraw now
                    if (mustRedraw)
                    {
                        RedrawDagopile(hWnd, newPi, 0);
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

        if ((wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) && isShiftPressed)
        {
            isShiftPressed = false;
        }

        else if ((wParam == VK_LMENU || wParam == VK_RMENU) && isAltPressed)
        {
            isAltPressed = false;
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

            // paint shrubbery: always
            COLORREF bkgColor = 0;
            switch (gameState.backgroundColor)
            {
            case BackgroundColors::Green:
                bkgColor = RGB(0, 0x80, 0);
                break;

            case BackgroundColors::Black:
                bkgColor = RGB(3, 3, 3);
                break;

            case BackgroundColors::DarkBlue:
                bkgColor = RGB(1, 1, 0x80);
                break;

            case BackgroundColors::DarkGray:
                bkgColor = RGB(0x69, 0x69, 0x69);
                break;

            case BackgroundColors::Pink:
                bkgColor = RGB(0xff, 0x79, 0x79);
                break;
            }

            SetBkColor(hdc, bkgColor);
            HBRUSH shrubbery = CreateSolidBrush(bkgColor);
            FillRect(hdc, &ps.rcPaint, shrubbery);
            DeleteObject(shrubbery);

            if (winningAnimationStage == 0)
            {
                PaintGame(hdc, &ps);
            }
            else
            {
                PaintDance(hdc, &ps);
            }

            BitBlt(rawDc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
            DeleteObject(memBitmap);
            DeleteDC(hdc);
            EndPaint(hWnd, &ps);
        }

        return true;
    
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            const auto& hMenu = GetMenu(hWnd);

            switch (wmId)
            {
            case ID_GAME_NEW:
                if (!PromptToSaveChanges(hWnd))
                {
                    return true;
                }

                NewGame(hWnd);

                return true;

            case ID_GAME_LOAD:
                if (!PromptToSaveChanges(hWnd))
                {
                    return true;
                }

                LoadGame(hWnd);
                break;

            case ID_GAME_SAVE:
                SaveGame(hWnd, false);
                break;

            case ID_GAME_SAVEAS:
                SaveGame(hWnd, true);
                break;

            case ID_PLAY_UNDO:
                Undo(hWnd);
                break;

            case ID_PLAY_REDO:
                Redo(hWnd);
                break;

            case ID_CARDBACK_BLUEDOT:
                if (cardBackside != Cards::BackBloodot)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackBloodot;
                }

                break;

            case ID_CARDBACK_COLORFUL:
                if (cardBackside != Cards::BackColorful)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackColorful;
                }

                break;

            case ID_CARDBACK_CUBERT:
                if (cardBackside != Cards::BackCubert)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackCubert;
                }

                break;

            case ID_CARDBACK_DIVINE:
                if (cardBackside != Cards::BackDivine)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackDivine;
                }

                break;

            case ID_CARDBACK_FELINE:
                if (cardBackside != Cards::BackCat)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackCat;
                }

                break;

            case ID_CARDBACK_ICE:
                if (cardBackside != Cards::BackIce)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackIce;
                }

                break;

            case ID_CARDBACK_MAPLE:
                if (cardBackside != Cards::BackMaple)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackMaple;
                }

                break;

            case ID_CARDBACK_PAPER:
                if (cardBackside != Cards::BackPaper)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackPaper;
                }

                break;

            case ID_CARDBACK_PARKETT:
                if (cardBackside != Cards::BackParkett)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackParkett;
                }

                break;

            case ID_CARDBACK_PINE:
                if (cardBackside != Cards::BackPine)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackPine;
                }

                break;

            case ID_CARDBACK_RED:
                if (cardBackside != Cards::BackRed)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackRed;
                }

                break;

            case ID_CARDBACK_ROCKS:
                if (cardBackside != Cards::BackRocks)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackRocks;
                }

                break;

            case ID_CARDBACK_SAFETY:
                if (cardBackside != Cards::BackSafety)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackSafety;
                }

                break;

            case ID_CARDBACK_SKY:
                if (cardBackside != Cards::BackSky)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackSky;
                }

                break;

            case ID_CARDBACK_SPACE:
                if (cardBackside != Cards::BackSpace)
                {
                    UncheckAllBackgroundMenuItems(hMenu, wmId);
                    cardBackside = Cards::BackSpace;
                }

                break;
                
            // radiogroup background (table) colors
            case ID_BACKGROUNDCOLOR_GREEN:
                if (backgroundColor != BackgroundColors::Green)
                {
                    UncheckAllBkgColorMenuItems(hMenu, wmId);
                    backgroundColor = BackgroundColors::Green;
                }

                break;

            case ID_BACKGROUNDCOLOR_BLACK:
                if (backgroundColor != BackgroundColors::Black)
                {
                    UncheckAllBkgColorMenuItems(hMenu, wmId);
                    backgroundColor = BackgroundColors::Black;
                }

                break;

            case ID_BACKGROUNDCOLOR_DARKBLUE:
                if (backgroundColor != BackgroundColors::DarkBlue)
                {
                    UncheckAllBkgColorMenuItems(hMenu, wmId);
                    backgroundColor = BackgroundColors::DarkBlue;
                }

                break;

            case ID_BACKGROUNDCOLOR_DARKGRAY:
                if (backgroundColor != BackgroundColors::DarkGray)
                {
                    UncheckAllBkgColorMenuItems(hMenu, wmId);
                    backgroundColor = BackgroundColors::DarkGray;
                }

                break;

            case ID_BACKGROUNDCOLOR_PINK:
                if (backgroundColor != BackgroundColors::Pink)
                {
                    UncheckAllBkgColorMenuItems(hMenu, wmId);
                    backgroundColor = BackgroundColors::Pink;
                }

                break;

            // radiogroup language
            case ID_LANGUAGE_DEUTSCH:
                if (language != FrontendLanguage::de)
                {
                    SetLanguage(hWnd, FrontendLanguage::de);
                }

                break;

            case ID_LANGUAGE_ENGLISH:
                if (language != FrontendLanguage::en)
                {
                    SetLanguage(hWnd, FrontendLanguage::en);
                }

                break;

            case ID_LANGUAGE_SUOMI:
                if (language != FrontendLanguage::fi)
                {
                    SetLanguage(hWnd, FrontendLanguage::fi);
                }

                break;

            case ID_LANGUAGE_UKRAINSKA:
                if (language != FrontendLanguage::ua)
                {
                    SetLanguage(hWnd, FrontendLanguage::ua);
                }

                break;

            // boolean toggles
            case ID_SETTINGS_BACKGROUNDMUSIC:
                musicOn = !musicOn;
                CheckMenuItem(hMenu, ID_SETTINGS_BACKGROUNDMUSIC, musicOn ? MF_CHECKED : MF_UNCHECKED);
                MIX_SetTrackGain(track1, musicOn ? 0.8 : 0);

                break;

            case ID_SETTINGS_SOUNDFX:
                soundFxOn = !soundFxOn;
                CheckMenuItem(hMenu, ID_SETTINGS_SOUNDFX, soundFxOn ? MF_CHECKED : MF_UNCHECKED);

                break;

            case ID_SETTINGS_COMMENTARY:
                commentaryOn = !commentaryOn;
                CheckMenuItem(hMenu, ID_SETTINGS_COMMENTARY, commentaryOn ? MF_CHECKED : MF_UNCHECKED);

                break;

            case ID_HELP_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                
                return true;

            case ID_GAME_EXIT:
            case IDM_EXIT:
                if (!PromptToSaveChanges(hWnd))
                {
                    return true;
                }

                DestroyWindow(hWnd);
                
                return true;
            }
        }

        break;

    case WM_CLOSE:
        if (!PromptToSaveChanges(hWnd))
        {
            return 0;
        }

        break;

    case WM_DESTROY:
        SaveSettings();
        PostQuitMessage(0);

        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void PaintGame(HDC hdc, PAINTSTRUCT* ps)
{
    HPEN hDashPen = CreatePen(PS_DASH, 1, RGB(0, 0, 0));

    // draw selection indicator
    if (currentDagopi != -1)
    {
        HPEN selPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
        HPEN prevOutline = (HPEN)SelectObject(hdc, selPen);
        HBRUSH prevFill = (HBRUSH)SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));

        const auto& dagoPos = gameState.dagoPiles[currentDagopi].pos;
        const auto& halfDist = dist / 2;
        const auto& centerX = dagoPos.left + (dagoPos.right - dagoPos.left) / 2;
        const auto& centerY = dagoPos.top - halfDist;
        POINT vertices[] = {
            {centerX - dist, centerY - 3},
            {centerX + dist, centerY - 3},
            {centerX, centerY + 3}
        };

        Polygon(hdc, vertices, sizeof(vertices) / sizeof(vertices[0]));
        SelectObject(hdc, prevOutline);
        SelectObject(hdc, prevFill);
        DeleteObject(selPen);
    }

    // prepare brush for pile borders
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

        // first determine the not dragged part
        const auto& inDrag = hasCapturedTheMouseToDragCards && !cardsBeingDragged.fromStockpile && (dragOffset.x != 0 || dragOffset.y != 0);
        auto drawCount = dpile.numCardsOnPile;

        if (inDrag && cardsBeingDragged.pile == &dpile)
        {
            drawCount -= cardsBeingDragged.index; // TODO: likely not correct wrt to index vs count
        }

        if (drawCount == 0)
        {
            // this dago pile is empty
            // this also means that we could not drag anything off here,
            // so we don't need to handle the inDrag scenario here
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
            for (int i = 0; i < drawCount; ++i)
            {
                const auto& dpileCard = dpile.pile[i];

                cdtDraw(
                    hdc,
                    dpos.left,
                    dpos.top + i * dist,
                    i < dpile.uncoveredFrom ? gameState.backside : dpileCard,
                    0,
                    0
                );
            }
        }
    }

    // the stockpile - last so it is on top of everything else
    if (gameState.stockpile.uncovered == -1)
    {
        // the empty stack is rendered as a card back because
        // that's the joker hiding under there
        cdtDraw(hdc, dist, dist, gameState.backside, 1, 0);
    }
    else
    {
        const auto& uncoveredCard = gameState.stockpile.pile[gameState.stockpile.uncovered];

        if (hasCapturedTheMouseToDragCards && cardsBeingDragged.fromStockpile && (dragOffset.x != 0 || dragOffset.y != 0))
        {
            // if the card being dragged is already off the pile, even slightly,
            // only then we draw the next card (or the empty pile indication) underneath
            if (gameState.stockpile.numCardsOnPile == 1)
            {
                // the empty stack is rendered as a card back because
                // that's the joker hiding there at the bottom
                cdtDraw(hdc, dist, dist, gameState.backside, 1, 0);
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
                cdtDraw(hdc, gameState.stockpile.pos.left + dragOffset.x, gameState.stockpile.pos.top + dragOffset.y, uncoveredCard, 1, 0);
            }
        }
        else
        {
            cdtDraw(hdc, gameState.stockpile.pos.left, gameState.stockpile.pos.top, uncoveredCard, 1, 0);
        }
    }

    SelectObject(hdc, oldBrush);
    DeleteObject(hDashPen);
}

bool Won(HWND hWnd)
{
    for (int ti = 0; ti < 4; ++ti)
    {
        if (gameState.targetPiles[ti].numCardsOnPile != 13)
        {
            return false;
        }

        if (GetRank(gameState.targetPiles[ti].pile[12]) != Cards::LaubKinigl)
        {
            return false;
        }
    }

    ClearUndoRedo(hWnd);
    ResetDirty(hWnd);
    Dance(hWnd);

    return true;
}

void UncheckAllBackgroundMenuItems(HMENU hMenu, int check)
{
    CheckMenuItem(hMenu, ID_CARDBACK_BLUEDOT, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_COLORFUL, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_CUBERT, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_DIVINE, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_FELINE, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_ICE, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_MAPLE, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_PAPER, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_PARKETT, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_PINE, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_RED, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_ROCKS, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_SAFETY, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_SKY, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_CARDBACK_SPACE, MF_UNCHECKED);
    CheckMenuItem(hMenu, check, MF_CHECKED);
}

UINT MenuItemFromCardBackside(Cards card)
{
    switch (card)
    {
    case Cards::BackBloodot: return ID_CARDBACK_BLUEDOT;
    case Cards::BackColorful: return ID_CARDBACK_COLORFUL;
    case Cards::BackCubert: return ID_CARDBACK_CUBERT;
    case Cards::BackDivine: return ID_CARDBACK_DIVINE;
    case Cards::BackCat: return ID_CARDBACK_FELINE;
    case Cards::BackIce: return ID_CARDBACK_ICE;
    case Cards::BackMaple: return ID_CARDBACK_MAPLE;
    case Cards::BackPaper: return ID_CARDBACK_PAPER;
    case Cards::BackParkett: return ID_CARDBACK_PARKETT;
    case Cards::BackPine: return ID_CARDBACK_PINE;
    case Cards::BackRed: return ID_CARDBACK_RED;
    case Cards::BackRocks: return ID_CARDBACK_ROCKS;
    case Cards::BackSafety: return ID_CARDBACK_SAFETY;
    case Cards::BackSky: return ID_CARDBACK_SKY;
    case Cards::BackSpace: return ID_CARDBACK_SPACE;
    }

    return 0;
}

UINT MenuItemFromBackgroundColor(BackgroundColors color)
{
    switch (color)
    {
    case BackgroundColors::Green: return ID_BACKGROUNDCOLOR_GREEN;
    case BackgroundColors::Black: return ID_BACKGROUNDCOLOR_BLACK;
    case BackgroundColors::DarkBlue: return ID_BACKGROUNDCOLOR_DARKBLUE;
    case BackgroundColors::DarkGray: return ID_BACKGROUNDCOLOR_DARKGRAY;
    case BackgroundColors::Pink: return ID_BACKGROUNDCOLOR_PINK;
    }

    return 0;
}

void UncheckAllBkgColorMenuItems(HMENU hMenu, int check)
{
    CheckMenuItem(hMenu, ID_BACKGROUNDCOLOR_GREEN, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_BACKGROUNDCOLOR_BLACK, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_BACKGROUNDCOLOR_DARKBLUE, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_BACKGROUNDCOLOR_DARKGRAY, MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_BACKGROUNDCOLOR_PINK, MF_UNCHECKED);
    CheckMenuItem(hMenu, check, MF_CHECKED);
}

void LoadSettings(HWND hWnd)
{
    WCHAR msg[MAX_LOADSTRING];
    auto loadedLang = FrontendLanguage::unknown;
    auto loadedBackside = Cards::BackBloodot;
    auto loadedBkgColor = BackgroundColors::Default;
    SettingsStruct checksig = {};
    SettingsStruct settings = {};

    std::fstream in(SettingsFilename, std::ios::in | std::ios::binary);
    if (in)
    {
        in.read((char*)&settings, sizeof(settings));
        in.close();

        if (
            settings.Preamble0 == checksig.Preamble0
            && settings.Preamble1 == checksig.Preamble1
            && settings.Preamble2 == checksig.Preamble2
            && settings.Preamble3 == checksig.Preamble3
            && settings.Preamble4 == checksig.Preamble4
            && settings.Preamble5 == checksig.Preamble5
            && settings.Preamble6 == checksig.Preamble6
            && settings.Preamble7 == checksig.Preamble7
            && settings.Preamble8 == checksig.Preamble8
        ) {
            if (settings.Preamble9 != '\6')
            {
                if (settings.Preamble9 == '\0')
                {
                    LoadString(hInst, IDS_MSG_BLOODOTSETTINGS, msg, MAX_LOADSTRING);
                    std::cerr << &msg;
                }
                if (settings.Preamble9 == '\7')
                {
                    LoadString(hInst, IDS_MSG_NOTSETTINGS, msg, MAX_LOADSTRING);
                    std::cerr << &msg;
                }
                else
                {
                    LoadString(hInst, IDS_MSG_SETTINGS_OTHERGAME, msg, MAX_LOADSTRING);
                    std::cerr << &msg;
                }
            }
            else
            {
                loadedLang = settings.language;
                loadedBackside = settings.cardBackside;
                loadedBkgColor = settings.backgroundColor;
                soundFxOn = settings.sfxOn;
                musicOn = settings.musicOn;
                commentaryOn = settings.commentaryOn;
            }
        }
    }

    auto hMenu = GetMenu(hWnd);
    
    // default language setting is taken from windows resource matching
    if (loadedLang > FrontendLanguage::unknown && loadedLang <= FrontendLanguage::ua && loadedLang != language)
    {
        SetLanguage(hWnd, loadedLang);
    }

    if (loadedBkgColor <= BackgroundColors::Default || loadedBkgColor > BackgroundColors::Pink)
    {
        loadedBkgColor = BackgroundColors::Green;
    }

    if (loadedBkgColor != backgroundColor)
    {
        backgroundColor = loadedBkgColor;
        UncheckAllBkgColorMenuItems(hMenu, MenuItemFromBackgroundColor(backgroundColor));
    }

    if (loadedBackside <= Cards::Empty || loadedBackside > Cards::BackSafety)
    {
        loadedBackside = Cards::BackBloodot;
    }

    if (loadedBackside != cardBackside)
    {
        cardBackside = loadedBackside;
        UncheckAllBackgroundMenuItems(hMenu, MenuItemFromCardBackside(cardBackside));
    }

    CheckMenuItem(hMenu, ID_SETTINGS_BACKGROUNDMUSIC, musicOn ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_SETTINGS_SOUNDFX, soundFxOn ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_SETTINGS_COMMENTARY, commentaryOn ? MF_CHECKED : MF_UNCHECKED);
}

void SaveSettings()
{
    SettingsStruct settings;

    settings.backgroundColor = backgroundColor;
    settings.cardBackside = cardBackside;
    settings.language = language;
    settings.musicOn = musicOn;
    settings.sfxOn = soundFxOn;
    settings.commentaryOn = commentaryOn;

    std::fstream out(SettingsFilename, std::ios::out | std::ios::trunc | std::ios::binary);
    out.write((char*)&settings, sizeof(settings));
    out.close();
}

void NewGame(HWND hWnd)
{
    lastSavedAsFilename[0] = 0;
    gameState.backside = cardBackside;
    gameState.backgroundColor = backgroundColor;
    gameState.stockpile.numCardsOnPile = 0;
    gameState.stockpile.uncovered = -1;

    for (int i = 0; i < 7; ++i)
    {
        if (i < 4)
        {
            gameState.targetPiles[i].numCardsOnPile = 0;
        }

        gameState.dagoPiles[i].numCardsOnPile = 0;
        gameState.dagoPiles[i].uncoveredFrom = -1;
    }

    InitNewDagobert();
    ResetDirty(hWnd);
    ClearUndoRedo(hWnd);

    if (hWnd != nullptr)
    {
        UpdateWindow(hWnd);
        InvalidateRect(hWnd, NULL, false);
    }
}

void LoadGame(HWND hWnd)
{
    WCHAR title[MAX_LOADSTRING];
    WCHAR filter[MAX_LOADSTRING];
    wchar_t szFile[MAX_PATH] = { 0 };
    OPENFILENAME dialogInfo = {};

    ZeroMemory(&dialogInfo, sizeof(OPENFILENAME));
    PrepareSaveFileDialogFilter(&dialogInfo, title, filter, IDS_LOAD_GAME);

    dialogInfo.lStructSize = sizeof(OPENFILENAME);
    dialogInfo.hwndOwner = hWnd;
    dialogInfo.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
    dialogInfo.lpstrFile = szFile;
    dialogInfo.nMaxFile = MAX_PATH;

    if (GetOpenFileName(&dialogInfo))
    {
        LoadGamestateFromFile(hWnd, dialogInfo.lpstrFile);
        InvalidateRect(hWnd, NULL, false);
    }
}

// returns true if actually saved something, also if there is nothing new to be saved
bool SaveGame(HWND hWnd, bool saveAs)
{
    WCHAR title[MAX_LOADSTRING];
    WCHAR filter[MAX_LOADSTRING];
    wchar_t szFile[MAX_PATH] = { 0 };
    OPENFILENAME dialogInfo = {};

    ZeroMemory(&dialogInfo, sizeof(OPENFILENAME));
    PrepareSaveFileDialogFilter(&dialogInfo, title, filter, saveAs ? IDS_SAVE_GAME_AS : IDS_SAVE_GAME);

    dialogInfo.lStructSize = sizeof(OPENFILENAME);
    dialogInfo.hwndOwner = hWnd;
    dialogInfo.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    dialogInfo.lpstrFile = szFile;
    dialogInfo.nMaxFile = MAX_PATH;

    if (saveAs)
    {
        if (GetSaveFileName(&dialogInfo))
        {
            SaveGamestateToFile(hWnd, dialogInfo.lpstrFile);
            wcsncpy_s(lastSavedAsFilename, dialogInfo.lpstrFile, MAX_PATH);
        }
    }
    else if (lastSavedAsFilename[0] == 0)
    {
        if (GetSaveFileName(&dialogInfo))
        {
            SaveGamestateToFile(hWnd, dialogInfo.lpstrFile);
            wcsncpy_s(lastSavedAsFilename, dialogInfo.lpstrFile, MAX_PATH);
        }
        else 
        {
            // decided not to save
            return false;
        }
    }
    else if (!dirty)
    {
        // nothing new to save
        return true;
    }

    // repeated save, overwrite / append
    SaveGamestateToFile(hWnd, lastSavedAsFilename);

    return true;
}

void PrepareSaveFileDialogFilter(OPENFILENAME* dialogInfo, WCHAR(&title)[MAX_LOADSTRING], WCHAR(&filter)[MAX_LOADSTRING], ULONG titleId)
{
    ZeroMemory(filter, MAX_LOADSTRING);
    LoadString(hInst, titleId, title, MAX_LOADSTRING);
    LoadString(hInst, IDS_SAVEGAME_FILEFILTER, filter, MAX_LOADSTRING);

    // filter expression is special, it has a null terminator inbetween
    const auto& transLen = wcsnlen_s(filter, MAX_LOADSTRING);
    const auto& wildcard = SavegameExtension;

    if (transLen < MAX_LOADSTRING)
    {
        const auto& wildLen = wcsnlen_s(wildcard, MAX_LOADSTRING);

        // "f0w0": both lengths are 1
        filter[transLen] = TEXT('\0');
        for (int i = 0; i < wildLen; ++i)
        {
            const auto& index = transLen + 1 + i;

            if (index < MAX_LOADSTRING)
            {
                filter[index] = wildcard[i];
            }
        }

        const auto& term = transLen + wildLen + 2;

        if (term < MAX_LOADSTRING)
        {
            filter[term - 1] = TEXT('\0');
            filter[term - 0] = TEXT('\0'); // yes, really.
        }
    }

    dialogInfo->lpstrTitle = title;
    dialogInfo->lpstrFilter = filter;
    dialogInfo->lpstrDefExt = wildcard;
}

void LoadGamestateFromFile(HWND hWnd, LPWSTR filename)
{
    WCHAR msg[MAX_LOADSTRING];
    SettingsStruct checksig = {};
    SettingsStruct settings = {};
    bool canCue = false;
    WCHAR title[MAX_LOADSTRING];

    LoadString(hInst, IDS_OPEN_SAVEGAME, title, MAX_LOADSTRING);
    std::fstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        in.read((char*)&settings, sizeof(settings));

        if (
            settings.Preamble0 == checksig.Preamble0
            && settings.Preamble1 == checksig.Preamble1
            && settings.Preamble2 == checksig.Preamble2
            && settings.Preamble3 == checksig.Preamble3
            && settings.Preamble4 == checksig.Preamble4
            && settings.Preamble5 == checksig.Preamble5
            && settings.Preamble6 == checksig.Preamble6
            && settings.Preamble7 == checksig.Preamble7
            && settings.Preamble8 == checksig.Preamble8
            ) {
            if (settings.Preamble9 != '\7')
            {
                if (settings.Preamble9 == '\0')
                {
                    LoadString(hInst, IDS_MSG_BLOODOTSAVEGAME, msg, MAX_LOADSTRING);
                    MessageBox(hWnd, msg, title, 0);
                }
                if (settings.Preamble9 == '\6')
                {
                    LoadString(hInst, IDS_MSG_NOTSAVEGAME, msg, MAX_LOADSTRING);
                    MessageBox(hWnd, msg, title, 0);
                }
                else
                {
                    LoadString(hInst, IDS_MSG_SAVEGAME_OTHERGAME, msg, MAX_LOADSTRING);
                    MessageBox(hWnd, msg, title, 0);
                }
            }
            else
            {
                canCue = true;
            }
        }
        else
        {
            LoadString(hInst, IDS_MSG_INVALID_SAVEGAME, msg, MAX_LOADSTRING);
            MessageBox(hWnd, msg, title, 0);
        }

        // now load the actual gamestate (piles)
        if (canCue)
        {
            gameState.backgroundColor = settings.backgroundColor;
            gameState.backside = settings.cardBackside;

            in.read((char*)&gameState.stockpile, sizeof(StockPile));
            for (int i = 0; i < 7; ++i)
            {
                if (i <= 3)
                {
                    in.read((char*)&gameState.targetPiles[i], sizeof(TargetPile));
                }

                in.read((char*)&gameState.dagoPiles[i], sizeof(DagoPile));
            }
        }

        in.close();
    }
    else
    {
        LoadString(hInst, IDS_COULD_NOT_READ_FILE, msg, MAX_LOADSTRING);
        MessageBox(hWnd, msg, title, 0);
    }

    // loading means that we can save over this file
    // as soon as we make any additional move
    wcsncpy_s(lastSavedAsFilename, filename, MAX_PATH);
    ResetDirty(hWnd);
    ClearUndoRedo(hWnd);
}

void SaveGamestateToFile(HWND hWnd, LPWSTR filename)
{
    // first part is the settings with the game-specific ones overridden
    SettingsStruct settings;

    settings.Preamble9 = '\7'; // 6..solidair settings, 7..solidair savegame
    settings.backgroundColor = gameState.backgroundColor;
    settings.cardBackside = gameState.backside;
    settings.language = language;         // not restored on load
    settings.musicOn = musicOn;           // not restored on load
    settings.sfxOn = soundFxOn;           // not restored on load
    settings.commentaryOn = commentaryOn; // not restored on load

    std::fstream out(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    out.write((char*)&settings, sizeof(settings));
    out.write((char*)&gameState.stockpile, sizeof(StockPile));
    for (int i = 0; i < 7; ++i)
    {
        // that's why they're interleaved so strangely in the file
        if (i <= 3)
        {
            out.write((char*)&gameState.targetPiles[i], sizeof(TargetPile));
        }

        out.write((char*)&gameState.dagoPiles[i], sizeof(DagoPile));
    }

    out.close();

    // saving once means that we can save again after something changed
    // without being prompted for a filename
    ResetDirty(hWnd);

    // and also undo and redo buffers are reset
    ClearUndoRedo(hWnd);
}

void InitNewDagobert()
{
    // we have 52 shuffled cards. thereof, we initialize the first dpile with 1
    // up to the seventh dpile with seven.
    // the remaining cards are placed on the stockpile and covered
    // I ♥ C++
    // [dlatikay 20260317] does not compensate for stupidity. I had [0..51) here,
    //                     then I introduced the "empty" card, then I wondered
    //                     why the king of spades has made himself so scarce
    auto range_view = std::ranges::views::iota((int)Cards::LaubAsslikum, ((int)Cards::PikKinigl) + 1);
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

void PushState(HWND hWnd)
{
    undoBuffer.push(gameState);
    if (!redoBuffer.empty())
    {
        std::stack<GameState>().swap(redoBuffer);
    }

    const auto& hMenu = GetMenu(hWnd);
    EnableMenuItem(hMenu, ID_PLAY_UNDO, MF_ENABLED);
    EnableMenuItem(hMenu, ID_PLAY_REDO, MF_DISABLED);
}

void ClearUndoRedo(HWND hWnd)
{
    std::stack<GameState>().swap(undoBuffer);
    std::stack<GameState>().swap(redoBuffer);

    const auto& hMenu = GetMenu(hWnd);

    EnableMenuItem(hMenu, ID_PLAY_UNDO, MF_DISABLED);
    EnableMenuItem(hMenu, ID_PLAY_REDO, MF_DISABLED);
}

void Undo(HWND hWnd)
{
    if (undoBuffer.empty())
    {
        return;
    }

    const auto& hMenu = GetMenu(hWnd);

    redoBuffer.push(gameState);
    gameState = undoBuffer.top();
    undoBuffer.pop();
    EnableMenuItem(hMenu, ID_PLAY_UNDO, undoBuffer.empty() ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, ID_PLAY_REDO, redoBuffer.empty() ? MF_DISABLED : MF_ENABLED);

    if (undoBuffer.empty())
    {
        ResetDirty(hWnd);
    }
    else 
    {
        SetDirty(hWnd);
    }

    InvalidateRect(hWnd, NULL, false);
}

void Redo(HWND hWnd)
{
    if (redoBuffer.empty())
    {
        return;
    }

    const auto& hMenu = GetMenu(hWnd);

    undoBuffer.push(gameState);
    gameState = redoBuffer.top();
    redoBuffer.pop();
    EnableMenuItem(hMenu, ID_PLAY_UNDO, undoBuffer.empty() ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, ID_PLAY_REDO, redoBuffer.empty() ? MF_DISABLED : MF_ENABLED);
    SetDirty(hWnd);
    InvalidateRect(hWnd, NULL, false);
}

void SetDirty(HWND hWnd)
{
    if (dirty)
    {
        return;
    }

    WCHAR name[MAX_LOADSTRING] = {};
    
    wcsncpy_s(name, szTitle, MAX_LOADSTRING);
    wcsncat_s(name, L"*", 1);
    SetWindowText(hWnd, &name[0]);

    dirty = true;
}

void ResetDirty(HWND hWnd)
{
    if (!dirty)
    {
        return;
    }

    SetWindowText(hWnd, szTitle);
    dirty = false;
}

// will return true if we're allowed to proceed with exiting
bool PromptToSaveChanges(HWND hWnd)
{
    if (!dirty)
    {
        return true;
    }

    const auto& result = DialogBox(hInst, MAKEINTRESOURCE(IDD_SAVEPROMPT), hWnd, PromptSave);
    
    if (result == 1)
    {
        // save now
        if (!SaveGame(hWnd, false))
        {
            // canceled in save dialog, aborts as if we'd selected "cancel" here right away
            return false;
        }
    }

    return result != 0;
}

void UpdateTitle(HWND hWnd)
{
    LoadStringW(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    if (dirty)
    {
        ResetDirty(hWnd);
        SetDirty(hWnd);
    }
    else
    {
        SetDirty(hWnd);
        ResetDirty(hWnd);
    }
}

void SetLanguage(HWND hWnd, FrontendLanguage lang)
{
    if (language == lang)
    {
        return;
    }

    switch (lang)
    {
    case FrontendLanguage::de:
        SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_NEUTRAL), SORT_DEFAULT));
        break;

    case FrontendLanguage::fi:
        SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_FINNISH, SUBLANG_NEUTRAL), SORT_DEFAULT));
        break;

    case FrontendLanguage::ua:
        SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_UKRAINIAN, SUBLANG_NEUTRAL), SORT_DEFAULT));
        break;

    case FrontendLanguage::en:
    default:
        SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), SORT_DEFAULT));
        break;
    }

    const auto& hMenu = GetMenu(hWnd);
    CheckMenuItem(hMenu, ID_LANGUAGE_ENGLISH, lang == FrontendLanguage::en ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_LANGUAGE_DEUTSCH, lang == FrontendLanguage::de ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_LANGUAGE_SUOMI, lang == FrontendLanguage::fi ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_LANGUAGE_UKRAINSKA, lang == FrontendLanguage::ua ? MF_CHECKED : MF_UNCHECKED);

    language = lang;

    // translate what's on the screen
    UpdateTitle(hWnd);
    
    WCHAR szCaption[MAX_LOADSTRING];
    
    LoadStringW(hInst, IDS_GAME, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_GAME, MF_BYCOMMAND | MF_STRING, ID_GAME, szCaption);
    LoadStringW(hInst, IDS_GAME_NEW, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_GAME_NEW, MF_BYCOMMAND | MF_STRING, ID_GAME_NEW, szCaption);
    LoadStringW(hInst, IDS_GAME_LOAD, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_GAME_LOAD, MF_BYCOMMAND | MF_STRING, ID_GAME_LOAD, szCaption);
    LoadStringW(hInst, IDS_GAME_SAVE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_GAME_SAVE, MF_BYCOMMAND | MF_STRING, ID_GAME_SAVE, szCaption);
    LoadStringW(hInst, IDS_GAME_SAVEAS, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_GAME_SAVEAS, MF_BYCOMMAND | MF_STRING, ID_GAME_SAVEAS, szCaption);
    LoadStringW(hInst, IDS_GAME_EXIT, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_GAME_EXIT, MF_BYCOMMAND | MF_STRING, ID_GAME_EXIT, szCaption);

    LoadStringW(hInst, IDS_PLAY, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_PLAY, MF_BYCOMMAND | MF_STRING, ID_PLAY, szCaption);
    LoadStringW(hInst, IDS_PLAY_UNDO, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_PLAY_UNDO, MF_BYCOMMAND | MF_STRING, ID_PLAY_UNDO, szCaption);
    LoadStringW(hInst, IDS_PLAY_REDO, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_PLAY_REDO, MF_BYCOMMAND | MF_STRING, ID_PLAY_REDO, szCaption);

    LoadStringW(hInst, IDS_SETTINGS, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_SETTINGS, MF_BYCOMMAND | MF_STRING, ID_SETTINGS, szCaption);
    LoadStringW(hInst, IDS_SETTINGS_BACKGROUNDMUSIC, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_SETTINGS_BACKGROUNDMUSIC, MF_BYCOMMAND | MF_STRING, ID_SETTINGS_BACKGROUNDMUSIC, szCaption);
    LoadStringW(hInst, IDS_SETTINGS_SOUNDFX, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_SETTINGS_SOUNDFX, MF_BYCOMMAND | MF_STRING, ID_SETTINGS_SOUNDFX, szCaption);
    LoadStringW(hInst, IDS_SETTINGS_COMMENTARY, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_SETTINGS_COMMENTARY, MF_BYCOMMAND | MF_STRING, ID_SETTINGS_COMMENTARY, szCaption);
    LoadStringW(hInst, IDS_CARDBACK, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK, MF_BYCOMMAND | MF_STRING, ID_CARDBACK, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_BLOODOT, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_BLUEDOT, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_BLUEDOT, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_FELINE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_FELINE, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_FELINE, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_COLORFUL, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_COLORFUL, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_COLORFUL, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_CUBERT, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_CUBERT, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_CUBERT, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_DIVINE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_DIVINE, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_DIVINE, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_ICE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_ICE, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_ICE, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_MAPLE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_MAPLE, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_MAPLE, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_PAPER, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_PAPER, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_PAPER, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_PARKETT, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_PARKETT, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_PARKETT, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_PINE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_PINE, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_PINE, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_RED, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_RED, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_RED, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_ROCKS, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_ROCKS, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_ROCKS, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_SAFETY, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_SAFETY, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_SAFETY, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_SKY, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_SKY, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_SKY, szCaption);
    LoadStringW(hInst, IDS_CARDBACK_SPACE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_CARDBACK_SPACE, MF_BYCOMMAND | MF_STRING, ID_CARDBACK_SPACE, szCaption);
    LoadStringW(hInst, IDS_BACKGROUNDCOLOR, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_BACKGROUNDCOLOR, MF_BYCOMMAND | MF_STRING, ID_BACKGROUNDCOLOR, szCaption);
    LoadStringW(hInst, IDS_BACKGROUNDCOLOR_GREEN, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_BACKGROUNDCOLOR_GREEN, MF_BYCOMMAND | MF_STRING, ID_BACKGROUNDCOLOR_GREEN, szCaption);
    LoadStringW(hInst, IDS_BACKGROUNDCOLOR_BLACK, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_BACKGROUNDCOLOR_BLACK, MF_BYCOMMAND | MF_STRING, ID_BACKGROUNDCOLOR_BLACK, szCaption);
    LoadStringW(hInst, IDS_BACKGROUNDCOLOR_DARKBLUE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_BACKGROUNDCOLOR_DARKBLUE, MF_BYCOMMAND | MF_STRING, ID_BACKGROUNDCOLOR_DARKBLUE, szCaption);
    LoadStringW(hInst, IDS_BACKGROUNDCOLOR_DARKGRAY, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_BACKGROUNDCOLOR_DARKGRAY, MF_BYCOMMAND | MF_STRING, ID_BACKGROUNDCOLOR_DARKGRAY, szCaption);
    LoadStringW(hInst, IDS_BACKGROUNDCOLOR_PINK, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_BACKGROUNDCOLOR_PINK, MF_BYCOMMAND | MF_STRING, ID_BACKGROUNDCOLOR_PINK, szCaption);

    LoadStringW(hInst, IDS_HELP, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_HELP, MF_BYCOMMAND | MF_STRING, ID_HELP, szCaption);
    LoadStringW(hInst, IDS_HELP_QUICKREFERENCE, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_HELP_QUICKREFERENCE, MF_BYCOMMAND | MF_STRING, ID_HELP_QUICKREFERENCE, szCaption);
    LoadStringW(hInst, IDS_HELP_ONLINEDOCS, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_HELP_ONLINEDOCS, MF_BYCOMMAND | MF_STRING, ID_HELP_ONLINEDOCS, szCaption);
    LoadStringW(hInst, IDS_HELP_ABOUT, szCaption, MAX_LOADSTRING);
    ModifyMenu(hMenu, ID_HELP_ABOUT, MF_BYCOMMAND | MF_STRING, ID_HELP_ABOUT, szCaption);

    EnableMenuItem(hMenu, ID_PLAY_UNDO, undoBuffer.empty() ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, ID_PLAY_REDO, redoBuffer.empty() ? MF_DISABLED : MF_ENABLED);
    DrawMenuBar(hWnd);
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
            cards->index = gameState.stockpile.uncovered;

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

        PushState(hWnd);
        gameState.stockpile.uncovered = 0;
        InvalidateRect(hWnd, &stockPile, false);
        SetDirty(hWnd);

        return true;
    }

    return false;
}

bool CycleStockpile(HWND hWnd)
{
    // can't cycle on a single card, can't cycle until uncovered
    if (gameState.stockpile.numCardsOnPile > 1 && gameState.stockpile.uncovered != -1)
    {
        PushState(hWnd);
        ++gameState.stockpile.uncovered;

        // cycle through them
        if (gameState.stockpile.uncovered == gameState.stockpile.numCardsOnPile)
        {
            gameState.stockpile.uncovered = 0;
        }

        const auto& spPos = gameState.stockpile.pos;
        const auto& stockPile = RECT{ spPos.left, spPos.top, spPos.right, spPos.bottom };

        InvalidateRect(hWnd, &stockPile, false);
        SetDirty(hWnd);

        return true;
    }

    return false;
}

bool PlaceStockpileOnDago(HWND hWnd, int pi)
{
    if (gameState.stockpile.uncovered != -1 && gameState.stockpile.numCardsOnPile >= 1)
    {
        const auto& card = gameState.stockpile.pile[gameState.stockpile.uncovered];

        // now, can it be placed there?
        auto& dP = gameState.dagoPiles[pi];
        if (!CanPlaceCardOnDagoPile(card, &dP))
        {
            return false;
        }

        // add it to the target pile
        dP.pile[dP.numCardsOnPile] = card;
        dP.numCardsOnPile++;

        RemoveFromStockpile(hWnd);

        // redraw the dago pile
        RedrawDagopile(hWnd, pi, 0);

        return true;
    }

    return false;
}

void RedrawDagopile(HWND hWnd, int pI, int removedToTakeIntoAccount)
{
    // redraw the dago pile
    const auto& dP = gameState.dagoPiles[pI];
    const auto& dpPos = dP.pos;
    const auto& dagoPile = RECT{ 
        dpPos.left,
        dpPos.top,
        dpPos.right,
        // account for removed cards, that area needs to be redrawn as well to clear it
        dpPos.bottom + (dP.numCardsOnPile - 1 + removedToTakeIntoAccount) * dist 
    };

    InvalidateRect(hWnd, &dagoPile, false);
}

bool PlaceStockpileOnTarget(HWND hWnd, int pi)
{
    if (gameState.stockpile.uncovered != -1 && gameState.stockpile.numCardsOnPile >= 1)
    {
        const auto& card = gameState.stockpile.pile[gameState.stockpile.uncovered];

        // now, can it be placed there?
        auto& tP = gameState.targetPiles[pi];
        if (!CanPlaceCardOnTargetPile(card, &tP))
        {
            return false;
        }

        PushState(hWnd);

        // add it to the target pile
        tP.pile[tP.numCardsOnPile] = card;
        tP.numCardsOnPile++;

        RemoveFromStockpile(hWnd);

        // redraw the target pile
        const auto& tpPos = tP.pos;
        const auto& targetPile = RECT{ tpPos.left, tpPos.top, tpPos.right, tpPos.bottom };

        InvalidateRect(hWnd, &targetPile, false);

        if (Won(hWnd))
        {
            // HOORAY
        }
        else
        {
            SetDirty(hWnd);
        }

        return true;
    }

    return false;
}

int ProposeMaximumMove(int fromIndex, int toIndex)
{
    if (fromIndex == -1 || toIndex == -1 || fromIndex == toIndex)
    {
        return -1;
    }

    const auto& fromPile = gameState.dagoPiles[fromIndex];

    if (fromPile.numCardsOnPile == 0 || fromPile.uncoveredFrom >= fromPile.numCardsOnPile)
    {
        return -1;
    }

    auto& toPile = gameState.dagoPiles[toIndex];

    for (int i = fromPile.uncoveredFrom; i < fromPile.numCardsOnPile; ++i)
    {
        const auto& card = fromPile.pile[i];

        if (CanPlaceCardOnDagoPile(card, &toPile))
        {
            return i;
        }
    }

    return -1;
}

bool Move(HWND hWnd, int srcIndex, int grabAt, int dstIndex)
{
    if (srcIndex == -1 || dstIndex == -1 || srcIndex == dstIndex)
    {
        return false;
    }

    auto& srcPile = gameState.dagoPiles[srcIndex];
    auto& dstPile = gameState.dagoPiles[dstIndex];

    int cardsMoved = 0;
    bool statePushed = false;

    for (int i = grabAt; i < srcPile.numCardsOnPile; ++i)
    {
        const auto& card = srcPile.pile[i];

        if (!statePushed)
        {
            PushState(hWnd);
            statePushed = true;
        }

        gameState.dagoPiles[dstIndex].pile[gameState.dagoPiles[dstIndex].numCardsOnPile] = card;
        ++gameState.dagoPiles[dstIndex].numCardsOnPile;
        ++cardsMoved;
    }

    if (cardsMoved != 0)
    {
        gameState.dagoPiles[srcIndex].numCardsOnPile -= cardsMoved;

        // refresh both piles
        RedrawDagopile(hWnd, srcIndex, cardsMoved);
        RedrawDagopile(hWnd, dstIndex, 0);
        SetDirty(hWnd);

        return true;
    }

    return false;
}

bool PlaceDagoOnTarget(HWND hWnd, int di, int ti)
{
    if (di == -1 || ti < 0 || ti > 3)
    {
        return false;
    }

    auto& source = gameState.dagoPiles[di];

    if (source.numCardsOnPile == 0)
    {
        return false;
    }

    auto& card = source.pile[source.numCardsOnPile - 1];

    // now, can it be placed there?
    auto& tP = gameState.targetPiles[ti];
    if (!CanPlaceCardOnTargetPile(card, &tP))
    {
        return false;
    }

    PushState(hWnd);

    // add it to the target pile
    tP.pile[tP.numCardsOnPile] = card;
    tP.numCardsOnPile++;

    RemoveFromDagopile(hWnd, di);

    // redraw the target pile
    const auto& tpPos = tP.pos;
    const auto& targetPile = RECT{ tpPos.left, tpPos.top, tpPos.right, tpPos.bottom };

    InvalidateRect(hWnd, &targetPile, false);

    if (Won(hWnd))
    {
        // HOORAY
    }
    else 
    {
        SetDirty(hWnd);
    }

    return true;
}

void AutoMoveToTarget(HWND hWnd)
{
    // look at every uncovered card from left to right and check 
    // if there is a target ready to take it. move it to the target if this is the case
    // this does not look at the stockpile. so if there was a direct move from stock
    // to target available, it is not executed by the automove function. this is a design
    // decision which is utterly inconsistent and now as I'm writing this I realized that
    // it is most likely illegal to move anything from stock to target without having
    // a dagopile as a stopover anyway, at least by klondike rules. anyway.
    for (int di = 0; di < 7; ++di)
    {
        auto& cP = gameState.dagoPiles[di];

        if (cP.numCardsOnPile != 0 && cP.uncoveredFrom != -1 && cP.uncoveredFrom < cP.numCardsOnPile)
        {
            auto& cC = cP.pile[cP.numCardsOnPile - 1];

            for (int ti = 0; ti < 4; ++ti)
            {
                auto& tP = gameState.targetPiles[ti];

                if (CanPlaceCardOnTargetPile(cC, &tP))
                {
                    PlaceDagoOnTarget(hWnd, di, ti);
 
                    return;
                }
            }
        }
    }
}

void SetSelectedDago(HWND hWnd, int dpIndex)
{
    if (currentDagopi != dpIndex)
    {
        // clear if none is selected any more, otherwise draw on newly selected pile
        if (currentDagopi != -1)
        {
            auto& prevP = gameState.dagoPiles[currentDagopi];
            const auto& prevPos = prevP.pos;
            const auto& prevIndicator = RECT{ prevPos.left, prevPos.top - dist + 1, prevPos.right, prevPos.top - 1 };

            InvalidateRect(hWnd, &prevIndicator, false);
        }

        currentDagopi = dpIndex;

        auto& selP = gameState.dagoPiles[currentDagopi];
        const auto& selPos = selP.pos;
        const auto& selIndicator = RECT{ selPos.left, selPos.top - dist + 1, selPos.right, selPos.top - 1 };

        InvalidateRect(hWnd, &selIndicator, false);
    }
}

void RemoveFromStockpile(HWND hWnd)
{
    if (gameState.stockpile.numCardsOnPile == 0)
    {
        return;
    }

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
    SetDirty(hWnd);
}

bool RemoveFromDagopile(HWND hWnd, int di)
{
    if (di == -1 || gameState.dagoPiles[di].numCardsOnPile == 0)
    {
        return false;
    }

    // remove it from the dagopile
    --gameState.dagoPiles[di].numCardsOnPile;

    // redraw the dagopile
    RedrawDagopile(hWnd, di, 1);
    SetDirty(hWnd);

    return true;
}

bool CanPlaceCardOnDagoPile(Cards card, DagoPile* pile)
{
    if (pile->numCardsOnPile == 0)
    {
        return IsCardAdjacent(Cards::Empty, card, CardColorMatchMode::Ignore);
    }

    // cannot place on a covered pile, and must be adjacent to fit
    // TODO: there appears to be a bug, I had been able to place a queen on a single-card pile with (supposedly) a covered king
    return pile->uncoveredFrom < pile->numCardsOnPile && IsCardAdjacent(
        card,
        pile->pile[pile->numCardsOnPile - 1],
        CardColorMatchMode::MustDifferInColor
    );
}

bool CanPlaceCardOnTargetPile(Cards card, TargetPile* pile)
{
    if (pile->numCardsOnPile == 0)
    {
        // can place any ace on an empty target, but nothing else than an ace
        return IsCardAdjacent(card, Cards::Empty, CardColorMatchMode::Ignore);
    }

    // must be adjacent and same suite to place on target
    return IsCardAdjacent(pile->pile[pile->numCardsOnPile - 1], card, CardColorMatchMode::MustMatchSuite);
}

bool IsCardAdjacent(Cards smaller, Cards biggger, CardColorMatchMode matchColor)
{
    // when a card is compared to an empty target, it must be an asslikum to fit
    if (biggger == Cards::Empty)
    {
        return smaller == LaubAsslikum || smaller == PikAsslikum || smaller == HerzAsslikum || smaller == KaroAsslikum;
    }

    // towards an empty dagopile, it must be a kinigl to fit
    if (smaller == Cards::Empty)
    {
        return biggger == LaubKinigl || biggger == PikKinigl || biggger == HerzKinigl || biggger == KaroKinigl;
    }

    // apply the color match rule. if this is not satisfied, no need to look further
    if (matchColor == CardColorMatchMode::MustDifferInColor)
    {
        if (GetColor(smaller) == GetColor(biggger))
        {
            return false;
        }
    }
    else if (matchColor == CardColorMatchMode::MustMatchSuite)
    {
        if (GetSuite(smaller) != GetSuite(biggger))
        {
            return false;
        }
    }

    // finally, the rank comparison
    const auto& rankSmaller = GetRank(smaller);
    const auto& rankBiggger = GetRank(biggger);

    return rankSmaller == rankBiggger - 1;
}

bool GetColor(Cards card)
{
    return (card >= KaroAsslikum && card <= KaroKinigl) || (card >= HerzAsslikum && card <= HerzKinigl);
}

Suite GetSuite(Cards card)
{
    if (card >= LaubAsslikum && card <= LaubKinigl)
    {
        return Suite::Laub;
    }
    else if (card >= KaroAsslikum && card <= KaroKinigl)
    {
        return Suite::Karo;
    }
    else if (card >= HerzAsslikum && card <= HerzKinigl)
    {
        return Suite::Herz;
    }
    else if (card >= PikAsslikum && card <= PikKinigl)
    {
        return Suite::Pik;
    }

    return Suite::None;
}

int GetRank(Cards card)
{
    if (card >= LaubAsslikum && card <= LaubKinigl)
    {
        return (int)card;
    }

    if (card >= KaroAsslikum && card <= KaroKinigl)
    {
        return (int)(card - KaroAsslikum);
    }

    if (card >= HerzAsslikum && card <= HerzKinigl)
    {
        return (int)(card - HerzAsslikum);
    }

    if (card >= PikAsslikum && card <= PikKinigl)
    {
        return (int)(card - PikAsslikum);
    }

    return -1;
}

void Dance(HWND hWnd)
{
    MSG msg;
    winningAnimationStage = 1;

    while (true)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            PostMessage(hWnd, WM_QUIT, 0, 0);

            break;
        }

        // animate continuously in 30 fps
        InvalidateRect(hWnd, NULL, false);
        ++winningAnimationTicks;
    }
}

void PaintDance(HDC hdc, PAINTSTRUCT* ps)
{
    switch (winningAnimationStage)
    {
        case 1:
        {
            // 1. cards leave the building face down
            auto& pos = gameState.targetPiles[0].pos;

            for (int i = 1; i < 10000; ++i)
            {
                if (i > winningAnimationTicks)
                {
                    cdtDraw(
                        hdc,
                        pos.left - 10 * sqrt(i * 12 + 10),
                        pos.top + i * 12,
                        Cards::Joker,
                        0,
                        0
                    );

                    break;
                }
                else
                {
                    cdtDraw(
                        hdc,
                        pos.left - 10 * sqrt(i * 12 + 10),
                        pos.top + i * 12,
                        gameState.backside,
                        0,
                        0
                    );
                }
            }
        }
     
        break;

    case 2:
        // 2. cards parade face up
        break;

        // 3. cards dance
    case 3:
        break;

        // 4. cards are shredded
    case 4:
        break;

        // 5. card shreds play game of life until the board is empty or a still life ensues
    case 5:
        break;
    }
}