#pragma once

#include "resource.h"
#include "schema.h"

ATOM RegisterWindowClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void NewGame(HWND hWnd);
void InitNewDagobert();
bool HitTest(POINT *p, CardsBeingHit* cards);
void ClickedOnCard(HWND hWnd);
bool UncoverStockpile(HWND hWnd);
bool CycleStockpile(HWND hWnd);
bool PlaceStockpileOnDago(HWND hWnd, int pi);
bool PlaceStockpileOnTarget(HWND hWnd, int pi);
bool PlaceDagoOnTarget(HWND hWnd, int di, int ti);
void RemoveFromStockpile(HWND hWnd);
bool RemoveFromDagopile(HWND hWnd, int di);
int ProposeMaximumMove(int fromIndex, int toIndex);
bool Move(HWND hWnd, int srcIndex, int grabAt, int dstIndex);
void RedrawDagopile(HWND hWnd, int pI, int removedToTakeIntoAccount);
void SetSelectedDago(HWND hWnd, int dpIndex);
bool CanPlaceCardOnDagoPile(Cards card, DagoPile* pile);
bool CanPlaceCardOnTargetPile(Cards card, TargetPile* pile);
bool IsCardAdjacent(Cards smaller, Cards biggger, CardColorMatchMode matchColor);
bool GetColor(Cards card);
Suite GetSuite(Cards card);
int GetRank(Cards card);

typedef BOOL(WINAPI* pfcdtInit)(int*, int*);
typedef BOOL(WINAPI* pfcdtDraw)(HDC, int x, int y, int card, int type, DWORD color);
typedef BOOL(WINAPI* pfcdtDrawEx)(HDC, int x, int y, int dx, int dy, int card, int type, DWORD color);
typedef BOOL(WINAPI* pfcdtAnimate)(HDC hdc, int cardback, int x, int y, int state);
typedef void (WINAPI* pfcdtTerm) (void);