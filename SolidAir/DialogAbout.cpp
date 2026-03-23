#include "DialogAbout.h"
#include <SolidAir.h>
#include <CommCtrl.h>

/// <summary>
/// About dialog
/// </summary>
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HWND hwndOwner;
    RECT rc, rcDlg, rcOwner;

    switch (message)
    {
    case WM_INITDIALOG:
        wchar_t caption[MAX_LOADSTRING];

        LoadStringW(hInst, IDS_ABOUT_DIALOG_TITLE, caption, MAX_LOADSTRING);
        SetWindowText(hDlg, caption);

        LoadStringW(hInst, IDS_ABOUT_PRODUCT, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_PRODUCT, caption);

        LoadStringW(hInst, IDS_ABOUT_COPYRIGHT, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_COPYRIIGHT, caption);

        LoadStringW(hInst, IDS_ABOUT_PUBLISHER, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_PUBLISHER, caption);

        LoadStringW(hInst, IDS_ABOUT_DESIGN, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_DESIGN, caption);

        LoadStringW(hInst, IDS_ABOUT_ARTWORK, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_ADDITIONAL_ARTWORK, caption);

        LoadStringW(hInst, IDS_ABOUT_MUSIC, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_MUSIC, caption);

        LoadStringW(hInst, IDS_ABOUT_LEGAL, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_LEGAL, caption);

        LoadStringW(hInst, IDS_ABOUT_DEDICATION, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_DEDICATION, caption);

        LoadStringW(hInst, IDS_ABOUT_GITHUB, caption, MAX_LOADSTRING);
        SetDlgItemText(hDlg, IDC_ABOUT_LABEL_GITHUB, caption);

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

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;

        if (pnmh->code == NM_CLICK || pnmh->code == NM_RETURN)
        {
            PNMLINK pNMLink = (PNMLINK)lParam;
            const auto& item = pNMLink->hdr.idFrom;

            if (item == IDC_ABOUT_LABEL_MUSIC)
            {
                ShellExecuteA(0, "open", musicUrl, NULL, NULL, SW_SHOWDEFAULT);
            }
            else if (item == IDC_ABOUT_LABEL_GITHUB)
            {
                ShellExecuteA(0, "open", githubUrl, NULL, NULL, SW_SHOWDEFAULT);
            }
        }
    }

    break;

    case WM_COMMAND:
    {
        const auto& cmd = LOWORD(wParam);

        if (cmd == IDC_ABOUT_OK || cmd == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));

            return (INT_PTR)TRUE;
        }
    }

    break;
    }

    return (INT_PTR)FALSE;
}
