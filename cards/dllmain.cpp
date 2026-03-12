#include "pch.h"
#include <iostream>
#include <objidl.h> 
#include <windows.h>
#include <gdiplus.h>
#include "cards.h"
#include <tchar.h>
#include "resource.h"
#pragma comment (lib,"Gdiplus.lib")

static Gdiplus::Image* cards[68];
static int numCards = 0;
constexpr int const ubound = 67;
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

    for (int i = 0; i < 68; ++i)
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
            //Laub2 = 1,
            //Laub3 = 2,
            //Laub4 = 3,
            //Laub5 = 4,
            //Laub6 = 5,
            //Laub7 = 6,
            //Laub8 = 7,
            //Laub9 = 8,
            //Laub10 = 9,
            case 10: resNr = laub_bublikum; break;
            case 11: resNr = laub_damlikum; break;
            case 12: resNr = laub_kinigl; break;
            //KaroAsslikum = 13,
            //Karo2 = 14,
            //Karo3 = 15,
            //Karo4 = 16,
            //Karo5 = 17,
            //Karo6 = 18,
            //Karo7 = 19,
            //Karo8 = 20,
            //Karo9 = 21,
            case 22: resNr = karo_10; break;
            //KaroBublikum = 23,
            case 24: resNr = karo_damlikum; break;
            //KaroKinigl = 25,

            //HerzAsslikum = 26,
            case 27: resNr = herz_2; break;
            //Herz3 = 28,
            //Herz4 = 29,
            case 30: resNr = herz_5; break;
            //Herz6 = 31,
            //Herz7 = 32,
            //Herz8 = 33,
            //Herz9 = 34,
            //Herz10 = 35,
            //HerzBublikum = 36,
            case 37: resNr = herz_damlikum; break;
            //HerzKinigl = 38,

            //PikAsslikum = 39,
            //Pik2 = 40,
            //Pik3 = 41,
            //Pik4 = 42,
            //Pik5 = 43,
            //Pik6 = 44,
            //Pik7 = 45,
            case 46: resNr = pik_8; break;
            case 47: resNr = pik_9; break;
            //Pik10 = 48,
            case 49: resNr = pik_bublikum; break;
            case 50: resNr = pik_damlikum; break;
            //PikKinigl = 51,
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
    return true;
}

extern "C" BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame)
{
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