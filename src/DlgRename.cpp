/*
    This file is part of SessionMgr, A Plugin for Notepad++. SessionMgr is free
    software: you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation,
    either version 3 of the License, or (at your option) any later version.
    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License along with this program. If not, see <http://www.gnu.org/licenses/>.
*//**
    @file      DlgRename.cpp
    @copyright Copyright 2011-2014 Michael Foster <http://mfoster.com/npp/>

    The "Rename Session" dialog.
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

ChildDialogData *_dialogData;

void onInit(HWND hDlg);
bool onOk(HWND hDlg);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgRen_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    INT_PTR status = FALSE;

    if (uMessage == WM_COMMAND) {
        switch (LOWORD(wParam)) {

            case IDOK:
                if (onOk(hDlg)) {
                    ::EndDialog(hDlg, 1);
                    status = TRUE;
                }
                break;

            case IDCANCEL:
                ::EndDialog(hDlg, 0);
                status = TRUE;
                break;
        }
    }
    else if (uMessage == WM_INITDIALOG) {
        _dialogData = (ChildDialogData*)lParam;
        onInit(hDlg);
    }

    return status;
}

//------------------------------------------------------------------------------

namespace {

void onInit(HWND hDlg)
{
    dlg::setText(hDlg, IDC_REN_ETX_NAME, app_getSessionName(_dialogData->selectedSessionIndex));
    dlg::focus(hDlg, IDC_REN_ETX_NAME);
    dlg::centerWnd(hDlg, NULL, 150, -35);
    ::ShowWindow(hDlg, SW_SHOW);
}

bool onOk(HWND hDlg)
{
    bool status = false;
    WCHAR srcPathname[MAX_PATH];
    WCHAR dstPathname[MAX_PATH];
    WCHAR newName[SES_NAME_BUF_LEN];

    // Set the destination file pathname.
    newName[0] = 0;
    dlg::getText(hDlg, IDC_REN_ETX_NAME, newName, SES_NAME_BUF_LEN);
    if (newName[0] == 0) {
        msg::show(L"Missing file name.", M_WARN);
        return false;
    }
    ::StringCchCopyW(dstPathname, MAX_PATH, gCfg.getSesDir());
    ::StringCchCatW(dstPathname, MAX_PATH, newName);
    ::StringCchCatW(dstPathname, MAX_PATH, gCfg.getSesExt());

    // Set the source file that will be renamed.
    app_getSessionFile(_dialogData->selectedSessionIndex, srcPathname);

    // Rename the file.
    _dialogData->newSessionName[0] = 0;
    if (::MoveFileExW(srcPathname, dstPathname, 0)) {
        ::StringCchCopyW(_dialogData->newSessionName, SES_NAME_BUF_LEN, newName);
        status = true;
    }
    else {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error renaming from \"%s\" to \"%s\".", _W(__FUNCTION__), srcPathname, dstPathname);
    }

    return status;
}

} // end namespace

} // end namespace NppPlugin
