#include "pch.h"
#include <iostream>
#include <objidl.h> 
#include <windows.h>
#include <gdiplus.h>
#include "cards.h"
#include <tchar.h>
#include "resource.h"
#pragma comment (lib,"Gdiplus.lib")

static Gdiplus::Image* cards[52];
static int numCards = 0;
ULONG_PTR gdiplusToken = 0;
HMODULE dllModule = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    dllModule = hModule;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
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

    for (int i = 0; i < 52; ++i)
    {
        IStream* pStream = nullptr;
        HGLOBAL hGlobal = nullptr;
        void* imagebytes;
        DWORD size;
        int resNr = 0;

        switch (i)
        {
            case 0: resNr = pik_8; break;
            case 1: resNr = pik_9; break;
            case 2: resNr = pik_bublikum; break;
            case 3: resNr = pik_damlikum; break;
            case 4: resNr = pik_kinigl; break;
        }

        if (resNr == 0)
        {
            break;
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
    if (card < 0 || card >= numCards)
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
    for (int i = 0; i < numCards; ++i)
    {
        delete cards[i];
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