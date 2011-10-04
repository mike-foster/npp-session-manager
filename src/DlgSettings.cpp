/*
    DlgSettings.cpp
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
#include "DlgSettings.h"
#include "Util.h"
#include "res\resource.h"
#include <strsafe.h>
#include <commdlg.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

RECT _iniRect = {0, 0, 0, 0};
bool _inInit, _opChanged, _dirChanged;

bool onOk(HWND hDlg);
bool onInit(HWND hDlg);
void onResize(HWND hDlg, INT w, INT h);
void onGetSize(HWND hDlg, LPMINMAXINFO p);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgCfg_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if (uMessage == WM_COMMAND) {
        WORD ctrl = LOWORD(wParam);
        WORD ntfy = HIWORD(wParam);
        switch (ctrl) {
            case IDOK:
                if (onOk(hDlg)) {
                    EndDialog(hDlg, 1);
                    return TRUE;
                }
                break;
            case IDCANCEL:
                EndDialog(hDlg, 0);
                return TRUE;
            case IDC_CFG_CHK_ASV:
            case IDC_CFG_CHK_ALD:
            case IDC_CFG_CHK_ELIC:
            case IDC_CFG_CHK_DLWC:
                if (!_inInit && ntfy == BN_CLICKED) {
                    _opChanged = true;
                }
                return TRUE;
            case IDC_CFG_EDT_DIR:
            case IDC_CFG_EDT_EXT:
                if (!_inInit && ntfy == EN_CHANGE) {
                    _dirChanged = true;
                }
                return TRUE;
        } // end switch
    }
    else if (uMessage == WM_SIZE) {
        onResize(hDlg, LOWORD(lParam), HIWORD(lParam));
    }
    else if (uMessage == WM_GETMINMAXINFO) {
        onGetSize(hDlg, (LPMINMAXINFO)lParam);
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

/* Populate the dialog box controls with current values from gCfg. */
bool onInit(HWND hDlg)
{
    _inInit = true;
    _opChanged = false;
    _dirChanged = false;
    // init control values
    dlg::setCheck(hDlg, IDC_CFG_CHK_ASV, gCfg.getAutoSave());
    dlg::setCheck(hDlg, IDC_CFG_CHK_ALD, gCfg.getAutoLoad());
    dlg::setCheck(hDlg, IDC_CFG_CHK_ELIC, gCfg.getEnableLIC());
    dlg::setCheck(hDlg, IDC_CFG_CHK_DLWC, gCfg.getDisableLWC());
    dlg::setText(hDlg, IDC_CFG_EDT_DIR, gCfg.getSesDir());
    dlg::setText(hDlg, IDC_CFG_EDT_EXT, gCfg.getSesExt());
    // focus the first edit control, center and show the window
    dlg::focus(hDlg, IDC_CFG_EDT_DIR);
    dlg::centerWnd(hDlg, sys_getNppHwnd(), 0, 0, TRUE);
    ShowWindow(hDlg, SW_SHOW);
    GetWindowRect(hDlg, &_iniRect);
    _inInit = false;
    return true;
}

/* Get values, if changed, from dialog box controls. Update the global
   Config object and save them to the ini file. */
bool onOk(HWND hDlg)
{
    bool change = false;
    TCHAR buf[MAX_PATH_1];

    if (_opChanged) {
        change = true;
        gCfg.setAutoSave(dlg::getCheck(hDlg, IDC_CFG_CHK_ASV));
        gCfg.setAutoLoad(dlg::getCheck(hDlg, IDC_CFG_CHK_ALD));
        gCfg.setEnableLIC(dlg::getCheck(hDlg, IDC_CFG_CHK_ELIC));
        gCfg.setDisableLWC(dlg::getCheck(hDlg, IDC_CFG_CHK_DLWC));
    }
    if (_dirChanged) {
        change = true;
        dlg::getText(hDlg, IDC_CFG_EDT_DIR, buf);
        gCfg.setSesDir(buf);
        dlg::getText(hDlg, IDC_CFG_EDT_EXT, buf);
        gCfg.setSesExt(buf);
    }

    if (change) {
        if (gCfg.save()) {
            if (_dirChanged) {
                app_readSesDir();
            }
        }
    }
    else {
        msgBox(_T("There were no changes."), M_INFO);
    }

    return true;
}

void onResize(HWND hDlg, INT w, INT h)
{
    HWND hCtrl = GetDlgItem(hDlg, IDC_CFG_EDT_DIR);
    if (hCtrl) {
        // Convert dialog units to pixels
        RECT r0 = {IDC_ALL_MARGIN, IDC_ALL_MARGIN, IDC_ALL_MARGIN + 10, IDC_ALL_MARGIN + 10};
        MapDialogRect(hDlg, &r0);
        // Resize the Directory edit
        RECT r;
        POINT p;
        GetWindowRect(hCtrl, &r);
        p.x = r.left;
        p.y = r.top;
        if (ScreenToClient(hDlg, &p)) {
            MoveWindow(hCtrl, p.x, p.y, w - p.x - r0.left, r.bottom - r.top, TRUE);
            ShowWindow(hCtrl, SW_SHOW);
        }
    }
}

/* Do not allow vertical resize. */
void onGetSize(HWND hDlg, LPMINMAXINFO p)
{
    if (_iniRect.left > 0) {
        p->ptMinTrackSize.x = _iniRect.right - _iniRect.left;
        p->ptMinTrackSize.y = p->ptMaxTrackSize.y = _iniRect.bottom - _iniRect.top;
    }
}

} // end namespace

} // end namespace NppPlugin
