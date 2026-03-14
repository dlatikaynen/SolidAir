#pragma once

#include "resource.h"
#include "schema.h"

ATOM RegisterWindowClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void InitNewDagobert();
bool HitTest(POINT *p, CardsBeingHit* cards);
void ClickedOnCard(HWND hWnd);
bool UncoverStockpile(HWND hWnd);
bool CycleStockpile(HWND hWnd);
bool PlaceStockpileOnDago(HWND hWnd, int pi);
bool PlaceStockpileOnTarget(HWND hWnd, int pi);
void RemoveFromStockpile(HWND hWnd);
bool CanPlaceCardOnDagoPile(Cards card, DagoPile* pile);
bool CanPlaceCardOnTargetPile(Cards card, TargetPile* pile);
bool IsCardAdjacent(Cards smaller, Cards biggger, CardColorMatchMode matchColor);
bool GetColor(Cards card);
int GetRank(Cards card);

typedef BOOL(WINAPI* pfcdtInit)(int*, int*);
typedef BOOL(WINAPI* pfcdtDraw)(HDC, int x, int y, int card, int type, DWORD color);
typedef BOOL(WINAPI* pfcdtDrawEx)(HDC, int x, int y, int dx, int dy, int card, int type, DWORD color);
typedef BOOL(WINAPI* pfcdtAnimate)(HDC hdc, int cardback, int x, int y, int state);
typedef void (WINAPI* pfcdtTerm) (void);