/*
    DlgNew.cpp
    Copyright 2011,2012 Michael Foster (http://mfoster.com/npp/)

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
#include "DlgNew.h"
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
bool newAsEmpty(TCHAR *dstPathname);
bool newFromOpen(TCHAR *dstPathname);
bool newByCopy(TCHAR *dstPathname);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgNew_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
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

            case IDC_NEW_RAD_EMPTY:
            case IDC_NEW_RAD_OPEN:
            case IDC_NEW_RAD_COPY:
                if (HIWORD(wParam) == BN_CLICKED) {
                    TCHAR buf[SES_MAX_LEN];
                    buf[0] = 0;
                    dlg::getText(hDlg, IDC_NEW_EDT_NAME, buf);
                    if (buf[0] == 0) {
                        dlg::setText(hDlg, IDC_NEW_EDT_NAME, app_getSesName(dlgSes_getLbSelectedData()));
                    }
                    dlg::focus(hDlg, IDC_NEW_EDT_NAME);
                    return TRUE;
                }
                break;
        }
    }
    else if (uMessage == WM_INITDIALOG) {
        if (onInit(hDlg)) {
            return TRUE;
        }
    }

    return FALSE;
}

/* DlgSessions uses this to get the new name. */
TCHAR* dlgNew_getLbNewName()
{
    return _lbNewName;
}

//------------------------------------------------------------------------------

namespace {

bool onInit(HWND hDlg)
{
    dlg::setCheck(hDlg, IDC_NEW_RAD_EMPTY, true);
    dlg::focus(hDlg, IDC_NEW_EDT_NAME);
    dlg::centerWnd(hDlg, NULL, 150, -40);
    ShowWindow(hDlg, SW_SHOW);
    return true;
}

bool onOk(HWND hDlg)
{
    bool succ;
    TCHAR newName[SES_MAX_LEN];
    TCHAR dstPathname[MAX_PATH_1];

    // Set the destination file pathname.
    newName[0] = 0;
    dlg::getText(hDlg, IDC_NEW_EDT_NAME, newName);
    if (newName[0] == 0) {
        msgBox(_T("Missing file name."), M_WARN);
        return false;
    }
    StringCchCopy(dstPathname, MAX_PATH, gCfg.getSesDir());
    StringCchCat(dstPathname, MAX_PATH, newName);
    StringCchCat(dstPathname, MAX_PATH, gCfg.getSesExt());

    if (dlg::getCheck(hDlg, IDC_NEW_RAD_EMPTY)) {
        succ = newAsEmpty(dstPathname);
    }
    else if (dlg::getCheck(hDlg, IDC_NEW_RAD_OPEN)) {
        succ = newFromOpen(dstPathname);
    }
    else {
        succ = newByCopy(dstPathname);
    }
    if (succ) {
        StringCchCopy(_lbNewName, SES_MAX_LEN - 1, newName);
    }
    return succ;
}

/* Create a new, empty session. */
bool newAsEmpty(TCHAR *dstPathname)
{
    createIfNotPresent(dstPathname, SES_DEFAULT_CONTENTS);
    return true;
}

/* Create a new session containing the currently open files. */
bool newFromOpen(TCHAR *dstPathname)
{
    SendMessage(sys_getNppHwnd(), NPPM_SAVECURRENTSESSION, 0, (LPARAM)dstPathname);
    return true;
}

/* Create a new session by copying the selected session. */
bool newByCopy(TCHAR *dstPathname)
{
    bool status = false;
    TCHAR srcPathname[MAX_PATH_1];

    // Set the source file pathname.
    INT sesSelIdx = dlgSes_getLbSelectedData();
    app_getSesFile(sesSelIdx, srcPathname);
    // Copy the file.
    _lbNewName[0] = 0;
    if (CopyFile(srcPathname, dstPathname, TRUE)) {
        status = true;
    }
    else {
        errBox(_T("Copy"), GetLastError());
    }
    return status;
}

} // end namespace

} // end namespace NppPlugin
