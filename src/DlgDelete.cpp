/*
    DlgDelete.cpp
    Copyright 2011,2013 Michael Foster (http://mfoster.com/npp/)

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
#include "DlgDelete.h"
#include "Util.h"
#include "res\resource.h"
#include <strsafe.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

bool onInit(HWND hDlg);
bool onOk(HWND hDlg);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgDel_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
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

//------------------------------------------------------------------------------

namespace {

bool onInit(HWND hDlg)
{
    dlg::focus(hDlg, IDOK);
    dlg::centerWnd(hDlg, NULL, 150, -5);
    ShowWindow(hDlg, SW_SHOW);
    return true;
}

bool onOk(HWND hDlg)
{
    TCHAR sesPth[MAX_PATH_1];
    INT sesSelIdx = dlgSes_getLbSelectedData();
    if (app_validSesIndex(sesSelIdx)) {
        app_getSesFile(sesSelIdx, sesPth);
        if (DeleteFile(sesPth)) {
            return true;
        }
        else {
            errBox(_T("Delete"), GetLastError());
        }
    }
    return false;
}

} // end namespace

} // end namespace NppPlugin
