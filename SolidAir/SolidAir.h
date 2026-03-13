#pragma once

#include "resource.h"
#include "schema.h"

void InitNewDagobert();
bool HitTest(POINT *p, CardsBeingHit* cards);
void ClickedOnCard(HWND hWnd);

typedef BOOL(WINAPI* pfcdtInit)(int*, int*);
typedef BOOL(WINAPI* pfcdtDraw)(HDC, int x, int y, int card, int type, DWORD color);
typedef BOOL(WINAPI* pfcdtDrawEx)(HDC, int x, int y, int dx, int dy, int card, int type, DWORD color);
typedef BOOL(WINAPI* pfcdtAnimate)(HDC hdc, int cardback, int x, int y, int state);
typedef void (WINAPI* pfcdtTerm) (void);