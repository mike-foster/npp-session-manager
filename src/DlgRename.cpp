/*
    DlgRename.cpp
    Copyright 2011 Michael Foster (http://mfoster.com/npp/)

    This file is part of SessionMgr, A Plugin for Notepad++.

    SessionMgr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "System.h"
#include "SessionMgr.h"
#include "Config.h"
#include "DlgSessions.h"
#include "DlgRename.h"
#include "Util.h"
#include "res\resource.h"
#include <strsafe.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

TCHAR _lbNewName[SES_MAX_LEN];

bool onInit(HWND hDlg);
bool onOk(HWND hDlg);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgRen_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if (uMessage == WM_COMMAND) {
        switch (LOWORD(wParam)) {

            case IDOK:
                if (onOk(hDlg)) {
                    EndDialog(hDlg, 1);
                    return TRUE;
                }
                break;

            case IDCANCEL:
                EndDialog(hDlg, 0);
                return TRUE;
        }
    }
    else if (uMessage == WM_INITDIALOG) {
        if (onInit(hDlg)) {
            return TRUE;
        }
    }

    return FALSE;
}

// DlgSessions uses this to get the new name.
TCHAR* dlgRen_getLbNewName()
{
    return _lbNewName;
}

//------------------------------------------------------------------------------

namespace {

bool onInit(HWND hDlg)
{
    dlg::setText(hDlg, IDC_REN_EDT_NAME, app_getSesName(dlgSes_getLbSelectedData()));
    dlg::focus(hDlg, IDC_REN_EDT_NAME);
    dlg::centerWnd(hDlg, NULL, 150, -35);
    ShowWindow(hDlg, SW_SHOW);
    return true;
}

bool onOk(HWND hDlg)
{
    bool status = false;
    TCHAR srcPathname[MAX_PATH_1];
    TCHAR dstPathname[MAX_PATH_1];
    TCHAR newName[SES_MAX_LEN];

    // Set the destination file pathname.
    newName[0] = 0;
    dlg::getText(hDlg, IDC_REN_EDT_NAME, newName);
    if (newName[0] == 0) {
        msgBox(_T("Missing file name."), M_WARN);
        return false;
    }
    StringCchCopy(dstPathname, MAX_PATH, gCfg.getSesDir());
    StringCchCat(dstPathname, MAX_PATH, newName);
    StringCchCat(dstPathname, MAX_PATH, gCfg.getSesExt());

    // Set the source file that will be renamed.
    INT sesSelIdx = dlgSes_getLbSelectedData();
    app_getSesFile(sesSelIdx, srcPathname);

    // Rename the file.
    _lbNewName[0] = 0;
    if (MoveFileEx(srcPathname, dstPathname, 0)) {
        StringCchCopy(_lbNewName, SES_MAX_LEN - 1, newName);
        status = true;
    }
    else {
        errBox(_T("Rename"), GetLastError());
    }

    return status;
}

} // end namespace

} // end namespace NppPlugin
