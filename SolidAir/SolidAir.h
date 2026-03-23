#pragma once

#include "commdlg.h"
#include "resource.h"
#include "schema.h"

constexpr const char* musicUrl = "https://freesound.org/s/560446";
constexpr const char* githubUrl = "https://github.com/dlatikaynen/SolidAir/issues/new";
constexpr auto MAX_LOADSTRING = 300;

extern HINSTANCE hInst;

ATOM RegisterWindowClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND* createdHwnd);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void UncheckAllBackgroundMenuItems(HMENU hMenu, int check);
void UncheckAllBkgColorMenuItems(HMENU hMenu, int check);
void LoadSettings(HWND hWnd);
void SaveSettings();
void SetLanguage(HWND hWnd, FrontendLanguage lang);
UINT MenuItemFromCardBackside(Cards card);
UINT MenuItemFromBackgroundColor(BackgroundColors color);
void UpdateTitle(HWND hWnd);
void NewGame(HWND hWnd);
void LoadGame(HWND hWnd);
void SaveGame(HWND hWnd, bool saveAs);
void PrepareSaveFileDialogFilter(OPENFILENAME* dialogInfo, WCHAR(&title)[MAX_LOADSTRING], WCHAR(&filter)[MAX_LOADSTRING], ULONG titleId);
void LoadGamestateFromFile(HWND hWnd, LPWSTR filename);
void SaveGamestateToFile(HWND hWnd, LPWSTR filename);
void Redo(HWND hWnd);
void Undo(HWND hWnd);
void ClearUndoRedo(HWND hWnd);
void PushState(HWND hWnd);
void SetDirty(HWND hWnd);
void ResetDirty(HWND hWnd);
void PromptToSaveChanges(HWND hWnd);
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
typedef BOOL(WINAPI* pfcdtName)(HDC hdc, int card, int type, wchar_t* name);
typedef void (WINAPI* pfcdtTerm) (void);