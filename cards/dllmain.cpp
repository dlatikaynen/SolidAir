#include "pch.h"
#include <iostream>
#include <objidl.h> 
#include <windows.h>
#include <gdiplus.h>
#include "cards.h"
#include <tchar.h>
#include "resource.h"
#pragma comment (lib,"Gdiplus.lib")

static Gdiplus::Image* cards[69];
static int numCards = 0;
constexpr int const ubound = 68;
ULONG_PTR gdiplusToken = 0;
HMODULE dllModule = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        dllModule = hModule;
        break;

    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

extern "C" BOOL WINAPI cdtInit(int* width, int* height)
{
    *width = 71;
    *height = 96;

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    for (int i = 0; i <= ubound; ++i)
    {
        cards[i] = nullptr;
        IStream* pStream = nullptr;
        HGLOBAL hGlobal = nullptr;
        void* imagebytes;
        DWORD size;
        int resNr = 0;

        switch (i)
        {
            case 0: resNr = laub_asslikum; break;
            case 1: resNr = laub_2; break;
            case 2: resNr = laub_3; break;
            case 3: resNr = laub_4; break;
            case 4: resNr = laub_5; break;
            case 5: resNr = laub_6; break;
            case 6: resNr = laub_7; break;
            case 7: resNr = laub_8; break;
            case 8: resNr = laub_9; break;
            case 9: resNr = laub_10; break;
            case 10: resNr = laub_bublikum; break;
            case 11: resNr = laub_damlikum; break;
            case 12: resNr = laub_kinigl; break;

            case 13: resNr = karo_asslikum; break;
            case 14: resNr = karo_2; break;
            case 15: resNr = karo_3; break;
            case 16: resNr = karo_4; break;
            case 17: resNr = karo_5; break;
            case 18: resNr = karo_6; break;
            case 19: resNr = karo_7; break;
            case 20: resNr = karo_8; break;
            case 21: resNr = karo_9; break;
            case 22: resNr = karo_10; break;
            case 23: resNr = karo_bublikum; break;
            case 24: resNr = karo_damlikum; break;
            case 25: resNr = karo_kinigl; break;

            case 26: resNr = herz_asslikum; break;
            case 27: resNr = herz_2; break;
            case 28: resNr = herz_3; break;
            case 29: resNr = herz_4; break;
            case 30: resNr = herz_5; break;
            case 31: resNr = herz_6; break;
            case 32: resNr = herz_7; break;
            case 33: resNr = herz_8; break;
            case 34: resNr = herz_9; break;
            case 35: resNr = herz_10; break;
            case 36: resNr = herz_bublikum; break;
            case 37: resNr = herz_damlikum; break;
            case 38: resNr = herz_kinigl; break;

            case 39: resNr = pik_asslikum; break;
            case 40: resNr = pik_2; break;
            case 41: resNr = pik_3; break;
            case 42: resNr = pik_4; break;
            case 43: resNr = pik_5; break;
            case 44: resNr = pik_6; break;
            case 45: resNr = pik_7; break;
            case 46: resNr = pik_8; break;
            case 47: resNr = pik_9; break;
            case 48: resNr = pik_10; break;
            case 49: resNr = pik_bublikum; break;
            case 50: resNr = pik_damlikum; break;
            case 51: resNr = pik_kinigl; break;
            
            case 52: resNr = joker; break;

            // backsides
            case 54: resNr = back_bloodot; break;
            case 55: resNr = back_cat; break;
            case 56: resNr = back_colorful; break;
            case 57: resNr = back_cubert; break;
            case 58: resNr = back_divine; break;
            case 59: resNr = back_paper; break;
            case 60: resNr = back_parkett; break;
            case 61: resNr = back_red; break;
            case 62: resNr = back_rocks; break;
            case 63: resNr = back_sky; break;
            case 64: resNr = back_space; break;
            case 65: resNr = back_ice; break;
            case 66: resNr = back_maple; break;
            case 67: resNr = back_pine; break;
            case 68: resNr = back_safety; break;
        }

        if (resNr == 0)
        {
            continue;
        }

        if (!GetPngFromResource(resNr, &imagebytes, &size))
        {
            return false;
        }

        // copy image bytes into a real hglobal memory handle
        hGlobal = ::GlobalAlloc(GHND, size);
        if (hGlobal)
        {
            void* pBuffer = ::GlobalLock(hGlobal);
            if (pBuffer)
            {
                memcpy(pBuffer, imagebytes, size);
                HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pStream);
                if (SUCCEEDED(hr))
                {
                    // pStream now owns the global handle and will invoke GlobalFree on release
                    hGlobal = nullptr;
                    cards[i] = new Gdiplus::Image(pStream);
                    numCards = i + 1;
                }

                if (pStream)
                {
                    pStream->Release();
                    pStream = nullptr;
                }
            }

            GlobalFree(hGlobal);
            hGlobal = nullptr;
        }
    }

    return true;
}

extern "C" BOOL WINAPI cdtDraw(HDC hdc, int x, int y, int card, int type, DWORD color)
{
    if (card < 0 || card > ubound || cards[card] == nullptr)
    {
        return false;
    }

    Gdiplus::Graphics grpx(hdc);
    grpx.DrawImage(cards[card], x, y, 71, 96);

    return true;
}

extern "C" BOOL WINAPI cdtDrawExt(HDC hdc, int x, int y, int dx, int dy, int card, int suit, DWORD color)
{
    // this version does not (yet) draw extended representations
    return true;
}

extern "C" BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame)
{
    // this version does not (yet) animate backgrounds
    return true;
}

// this had not been part of the original cards.dll, it is an innovation
extern "C" BOOL __declspec(dllexport) cdtName(HDC hdc, int card, int type, wchar_t* name)
{
    switch (card)
    {
    case 0:
        wcsncpy_s(name, 4, L"♣-1", 4);
        break;

    case 1: 
        wcsncpy_s(name, 4, L"♣-2", 4);
        break;

    case 2: 
        wcsncpy_s(name, 4, L"♣-3", 4);
        break;

    case 3: 
        wcsncpy_s(name, 4, L"♣-4", 4);
        break;

    case 4: 
        wcsncpy_s(name, 4, L"♣-5", 4);
        break;

    case 5: 
        wcsncpy_s(name, 4, L"♣-6", 4);
        break;

    case 6: 
        wcsncpy_s(name, 4, L"♣-7", 4);
        break;

    case 7: 
        wcsncpy_s(name, 4, L"♣-8", 4);
        break;

    case 8: 
        wcsncpy_s(name, 4, L"♣-9", 4);
        break;

    case 9: 
        wcsncpy_s(name, 5, L"♣-10", 5);
        break;

    case 10: 
        wcsncpy_s(name, 9, L"Lancelot", 9);
        break;

    case 11: 
        wcsncpy_s(name, 7, L"Helena", 7);
        break;

    case 12: 
        wcsncpy_s(name, 10, L"Alexander", 10);
        break;

    case 13:
        wcsncpy_s(name, 4, L"♦-1", 4);
        break;

    case 14: 
        wcsncpy_s(name, 4, L"♦-2", 4);
        break;

    case 15: 
        wcsncpy_s(name, 4, L"♦-3", 4);
        break;

    case 16: 
        wcsncpy_s(name, 4, L"♦-4", 4);
        break;

    case 17: 
        wcsncpy_s(name, 4, L"♦-5", 4);
        break;

    case 18: 
        wcsncpy_s(name, 4, L"♦-6", 4);
        break;

    case 19: 
        wcsncpy_s(name, 4, L"♦-7", 4);
        break;

    case 20: 
        wcsncpy_s(name, 4, L"♦-8", 4);
        break;

    case 21: 
        wcsncpy_s(name, 4, L"♦-9", 4);
        break;

    case 22: 
        wcsncpy_s(name, 5, L"♦-10", 5);
        break;

    case 23: 
        wcsncpy_s(name, 7, L"Hector", 7);
        break;

    case 24: 
        wcsncpy_s(name, 7, L"Rachel", 7);
        break;

    case 25: 
        wcsncpy_s(name, 7, L"Julius", 7);
        break;

    case 26: 
        wcsncpy_s(name, 4, L"♥-1", 4);
        break;

    case 27: 
        wcsncpy_s(name, 4, L"♥-2", 4);
        break;

    case 28: 
        wcsncpy_s(name, 4, L"♥-3", 4);
        break;

    case 29: 
        wcsncpy_s(name, 4, L"♥-4", 4);
        break;

    case 30: 
        wcsncpy_s(name, 4, L"♥-5", 4);
        break;

    case 31: 
        wcsncpy_s(name, 4, L"♥-6", 4);
        break;

    case 32: 
        wcsncpy_s(name, 4, L"♥-7", 4);
        break;

    case 33: 
        wcsncpy_s(name, 4, L"♥-8", 4);
        break;

    case 34: 
        wcsncpy_s(name, 4, L"♥-9", 4);
        break;

    case 35: 
        wcsncpy_s(name, 5, L"♥-10", 5);
        break;

    case 36: 
        wcsncpy_s(name, 8, L"Étienne", 8);
        break;

    case 37: 
        wcsncpy_s(name, 7, L"Jeanne", 7);
        break;

    case 38: 
        wcsncpy_s(name, 8, L"Charles", 8);
        break;

    case 39: 
        wcsncpy_s(name, 4, L"♠-1", 4);
        break;

    case 40: 
        wcsncpy_s(name, 4, L"♠-2", 4);
        break;

    case 41:
        wcsncpy_s(name, 4, L"♠-3", 4);
        break;

    case 42: 
        wcsncpy_s(name, 4, L"♠-4", 4);
        break;

    case 43: 
        wcsncpy_s(name, 4, L"♠-5", 4);
        break;

    case 44: 
        wcsncpy_s(name, 4, L"♠-6", 4);
        break;

    case 45: 
        wcsncpy_s(name, 4, L"♠-7", 4);
        break;

    case 46: 
        wcsncpy_s(name, 4, L"♠-8", 4);
        break;

    case 47: 
        wcsncpy_s(name, 4, L"♠-9", 4);
        break;

    case 48: 
        wcsncpy_s(name, 5, L"♠-10", 5);
        break;

    case 49: 
        wcsncpy_s(name, 6, L"Ogier", 6);
        break;

    case 50: 
        wcsncpy_s(name, 7, L"Pallas", 7);
        break;

    case 51:
        wcsncpy_s(name, 6, L"David", 6);
        break;

    case 52: 
        wcsncpy_s(name, 6, L"Joker", 6);
        break;

    default:
        return false;
    }

    return true;
}

extern "C" void WINAPI cdtTerm(void)
{
    for (int i = 0; i <= ubound; ++i)
    {
        if (cards[i] != nullptr)
        {
            delete cards[i];
        }
    }

    if (gdiplusToken != 0)
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }
}

BOOL GetPngFromResource(int iResourceID, void** ppPngFileData, DWORD* pwPngDataBytes)
{
    if (dllModule == 0)
    {
        return false;
    }

    HRSRC hRes = FindResource(dllModule, MAKEINTRESOURCE(iResourceID), _T("PNG"));

    if (hRes == 0)
    {
        return false;
    }

    HGLOBAL hGlobal = LoadResource(dllModule, hRes);

    if (hGlobal == 0)
    {
        return false;
    }

    *ppPngFileData = LockResource(hGlobal);

    const auto size = SizeofResource(dllModule, hRes);
    
    *pwPngDataBytes = size;
    
    return size > 0;
}