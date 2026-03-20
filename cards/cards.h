#pragma once
#include <minwindef.h>
#include <wtypes.h>

extern "C" BOOL __declspec(dllexport) cdtInit(int* width, int* height);
extern "C" BOOL __declspec(dllexport) cdtDraw(HDC hdc, int x, int y, int card, int type, DWORD color);
extern "C" BOOL __declspec(dllexport) cdtDrawExt(HDC hdc, int x, int y, int dx, int dy, int card, int suit, DWORD color);
extern "C" BOOL __declspec(dllexport) cdtAnimate(HDC hdc, int cardback, int x, int y, int frame);
extern "C" BOOL __declspec(dllexport) cdtName(HDC hdc, int card, int type, wchar_t* name);
extern "C" void __declspec(dllexport) cdtTerm(void);

BOOL GetPngFromResource(int iResourceID, void** ppPngFileData, DWORD* pwPngDataBytes);