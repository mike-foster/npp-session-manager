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
    @file      DlgDelete.cpp
    @copyright Copyright 2011,2013,2014 Michael Foster <http://mfoster.com/npp/>

    The "Delete Session" dialog.
*/

#include "System.h"
#include "SessionMgr.h"
#include "Config.h"
#include "DlgSessions.h"
#include "DlgDelete.h"
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

INT_PTR CALLBACK dlgDel_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
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
    dlg::focus(hDlg, IDOK);
    dlg::centerWnd(hDlg, NULL, 150, -5);
    ::ShowWindow(hDlg, SW_SHOW);
}

bool onOk(HWND hDlg)
{
    WCHAR sesPth[MAX_PATH];
    if (app_isValidSessionIndex(_dialogData->selectedSessionIndex)) {
        app_getSessionFile(_dialogData->selectedSessionIndex, sesPth);
        if (::DeleteFileW(sesPth)) {
            return true;
        }
        else {
            DWORD le = ::GetLastError();
            msg::error(le, L"%s: Error deleting \"%s\".", _W(__FUNCTION__), sesPth);
        }
    }
    return false;
}

} // end namespace

} // end namespace NppPlugin
