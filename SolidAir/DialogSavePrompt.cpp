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

            LoadStringW(hInst, IDS_SAVEPROMPT_TITLE, caption, MAX_LOADSTRING);
            SetWindowText(hDlg, caption);

            LoadStringW(hInst, IDS_SAVEPROMPT_LABEL, caption, MAX_LOADSTRING);
            SetDlgItemText(hDlg, IDC_SAVEPROMPT_LABEL, caption);

            LoadStringW(hInst, IDS_SAVEPROMPT_SAVE, caption, MAX_LOADSTRING);
            SetDlgItemText(hDlg, IDC_SAVEPROMPT_SAVE, caption);

            LoadStringW(hInst, IDS_SAVEPROMPT_DONT_SAVE, caption, MAX_LOADSTRING);
            SetDlgItemText(hDlg, IDC_SAVEPROMPT_DONT_SAVE, caption);

            LoadStringW(hInst, IDS_CANCEL, caption, MAX_LOADSTRING);
            SetDlgItemText(hDlg, IDC_SAVEPROMPT_CANCEL, caption);

            if ((hwndOwner = GetParent(hDlg)) == NULL)
            {
                hwndOwner = GetDesktopWindow();
            }

            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);
            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
            SetWindowPos(
                hDlg,
                HWND_TOP,
                rcOwner.left + (rc.right / 2),
                rcOwner.top + (rc.bottom / 2),
                0,
                0,
                SWP_NOSIZE
            );

            return (INT_PTR)TRUE;
        }

        break;

        case WM_CLOSE:
            // equal to "Cancel"
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;

        case WM_COMMAND:
        {
            const auto& cmd = LOWORD(wParam);

            if (cmd == IDC_SAVEPROMPT_SAVE)
            {
                EndDialog(hDlg, 1);

                return (INT_PTR)TRUE;
            }
            else if (cmd == IDC_SAVEPROMPT_DONT_SAVE)
            {
                EndDialog(hDlg, -1);

                return (INT_PTR)TRUE;
            }
            else if (cmd == IDC_SAVEPROMPT_CANCEL)
            {
                EndDialog(hDlg, 0);

                return (INT_PTR)TRUE;
            }
        }

        break;
    }

    return (INT_PTR)FALSE;
}
