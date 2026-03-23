#include "DialogSavePrompt.h"
#include <SolidAir.h>
#include <CommCtrl.h>

/// <summary>
/// "Unsaved changes" dialog
/// </summary>
INT_PTR CALLBACK PromptSave(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HWND hwndOwner;
    RECT rc, rcDlg, rcOwner;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            wchar_t caption[MAX_LOADSTRING];

            LoadStringW(hInst, IDS_ABOUT_DIALOG_TITLE, caption, MAX_LOADSTRING);
            SetWindowText(hDlg, caption);
        }

        break;
    }

    return (INT_PTR)FALSE;
}
