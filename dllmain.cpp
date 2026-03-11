// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <iostream>
#include "cards.h"

static HBITMAP* cards;
static int numCards = 0;

static BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
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

extern "C" BOOL cdtInit(int* width, int* height)
{
    *width = 71;
    *height = 96;

    return true;
}

extern "C" BOOL WINAPI cdtDraw(HDC hdc, int x, int y, int card, int type, DWORD color)
{
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
        if (!DeleteObject(cards[i]))
        {
            std::cerr << "Failed to delete " << i;
        }
    }
}