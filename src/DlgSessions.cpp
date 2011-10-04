/*
    DlgSessions.cpp
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
#include "Config.h"
#include "SessionMgr.h"
#include "DlgSessions.h"
#include "DlgNew.h"
#include "DlgRename.h"
#include "DlgDelete.h"
#include "Util.h"
#include "res\resource.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

INT _lbSelectedData;
RECT _iniRect = {0, 0, 0, 0};

bool onInit(HWND hDlg);
bool onOk(HWND hDlg);
bool onNew(HWND hDlg);
bool onRename(HWND hDlg);
bool onDelete(HWND hDlg);
bool onDefault(HWND hDlg);
bool fillListBox(HWND hDlg, INT sesCurIdx = SES_CURRENT);
void onResize(HWND hDlg, INT dlgW, INT dlgH);
void onGetSize(HWND hDlg, LPMINMAXINFO p);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgSes_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    WORD ctrl = LOWORD(wParam);
    WORD ntfy = HIWORD(wParam);
    if (uMessage == WM_COMMAND) {
        switch (ctrl) {
            case IDC_SES_LST_SES:
                if (ntfy == LBN_DBLCLK) {
                    if (onOk(hDlg)) {
                        return TRUE;
                    }
                }
                break;
            case IDOK:
            case IDC_SES_BTN_LOAD:
                if (onOk(hDlg)) {
                    return TRUE;
                }
                break;
            case IDC_SES_BTN_NEW:
                if (ntfy == BN_CLICKED) {
                    dlg::focus(hDlg, IDC_SES_BTN_LOAD);
                    if (onNew(hDlg)) {
                        return TRUE;
                    }
                }
                break;
            case IDC_SES_BTN_REN:
                if (ntfy == BN_CLICKED) {
                    dlg::focus(hDlg, IDC_SES_BTN_LOAD);
                    if (onRename(hDlg)) {
                        return TRUE;
                    }
                }
                break;
            case IDC_SES_BTN_DEL:
                if (ntfy == BN_CLICKED) {
                    dlg::focus(hDlg, IDC_SES_BTN_LOAD);
                    if (onDelete(hDlg)) {
                        return TRUE;
                    }
                }
                break;
            case IDC_SES_BTN_DEF:
                if (onDefault(hDlg)) {
                    return TRUE;
                }
                break;
            case IDC_SES_BTN_SAVE:
                app_saveSession(SES_CURRENT);
                dlg::focus(hDlg, IDC_SES_BTN_LOAD);
                return TRUE;
                break;
            case IDCANCEL:
            case IDC_SES_BTN_CANCEL:
                EndDialog(hDlg, 0);
                return TRUE;
            case IDC_SES_CHK_LIC:
                if (ntfy == BN_CLICKED) {
                    if (dlg::getCheck(hDlg, IDC_SES_CHK_LIC)) {
                        dlg::setCheck(hDlg, IDC_SES_CHK_LWC, true);
                    }
                }
                return TRUE;
        }
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
    else if (uMessage == WM_SETFOCUS) {
        dlg::focus(hDlg, IDC_SES_LST_SES);
    }
    return FALSE;
}

/* Child dialogs use this to get the vector index of the session selected in the
   list. */
INT dlgSes_getLbSelectedData()
{
    return _lbSelectedData;
}

//------------------------------------------------------------------------------

namespace {

bool onInit(HWND hDlg)
{
    bool ret = false;
    dlg::setText(hDlg, IDC_SES_TXT_CUR, app_getSesName(SES_CURRENT));
    dlg::setCheck(hDlg, IDC_SES_CHK_LIC, gCfg.getEnableLIC());
    if (gCfg.getDisableLWC()) {
        dlg::setCheck(hDlg, IDC_SES_CHK_LWC, false);
    }
    if (fillListBox(hDlg)) {
        dlg::centerWnd(hDlg, sys_getNppHwnd(), 0, 0, TRUE);
        ShowWindow(hDlg, SW_SHOW);
        ret = true;
    }
    GetWindowRect(hDlg, &_iniRect); // initial rect of dialog
    return ret;
}

bool onOk(HWND hDlg)
{
    INT selIdx;
    bool lic, lwc;

    selIdx = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    lic = dlg::getCheck(hDlg, IDC_SES_CHK_LIC);
    lwc = dlg::getCheck(hDlg, IDC_SES_CHK_LWC);
    EndDialog(hDlg, 0);
    app_loadSession(selIdx, lic, lwc);
    return true;
}

bool onDefault(HWND hDlg)
{
    bool lic, lwc;

    lic = dlg::getCheck(hDlg, IDC_SES_CHK_LIC);
    lwc = dlg::getCheck(hDlg, IDC_SES_CHK_LWC);
    EndDialog(hDlg, 0);
    app_loadSession(SES_DEFAULT, lic, lwc);
    return true;
}

bool onNew(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_NEW_DLG), hDlg, dlgNew_msgProc)) {
        app_readSesDir();
        status = fillListBox(hDlg, app_getSesIndex(dlgNew_getLbNewName()));
    }
    return status;
}

bool onRename(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_REN_DLG), hDlg, dlgRen_msgProc)) {
        app_readSesDir();
        status = fillListBox(hDlg, app_getSesIndex(dlgRen_getLbNewName()));
    }
    return status;
}

bool onDelete(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_DEL_DLG), hDlg, dlgDel_msgProc)) {
        app_readSesDir();
        status = fillListBox(hDlg);
    }
    return status;
}

bool fillListBox(HWND hDlg, INT sesCurIdx)
{
    HWND hLst;
    INT i, sesIdx, sesCount;

    hLst = GetDlgItem(hDlg, IDC_SES_LST_SES);
    if (hLst) {
        SendMessage(hLst, LB_RESETCONTENT, 0, 0);
        sesCount = app_getSesCount();
        if (sesCurIdx == SES_CURRENT) {
            sesCurIdx = app_getSesIndex();
        }
        for (sesIdx = 0; sesIdx < sesCount; ++sesIdx) {
            //addSession(hLst, sesIdx, sesCurIdx);
            i = (INT)SendMessage(hLst, LB_ADDSTRING, 0, (LPARAM)app_getSesName(sesIdx));
            SendMessage(hLst, LB_SETITEMDATA, i, (LPARAM)sesIdx);
            if (sesIdx == sesCurIdx) {
                SendMessage(hLst, LB_SETCURSEL, i, 0);
            }
        }
        /* When the listbox is displayed, the focused item and selected item
        are not the same - haven't solved that yet. */
        //dlg::focus(hDlg, IDC_SES_LST_SES);
        return true;
    }
    return false;
}

void onResize(HWND hDlg, INT dlgW, INT dlgH)
{
    // Convert dialog units to pixels
    RECT r0 = {IDC_ALL_MARGIN, IDC_ALL_MARGIN, IDD_SES_W - IDC_ALL_MARGIN - IDC_SES_LST_SES_W, IDD_SES_H - IDC_SES_LST_SES_Y - IDC_SES_LST_SES_H};
    RECT r1 = {r0.left, IDD_SES_H - IDC_SES_CHK_LIC_Y - IDC_SES_CHK_H, r0.right, r0.bottom};
    MapDialogRect(hDlg, &r0);
    MapDialogRect(hDlg, &r1);
    // Resize the session list
    dlg::adjToEdge(hDlg, IDC_SES_LST_SES, dlgW, dlgH, -1, -1, r0.right, r0.bottom);
    // Resize the Current text
    dlg::adjToEdge(hDlg, IDC_SES_TXT_CUR, dlgW, dlgH, -1, -1, r0.left, -1);
    // Move the buttons
    INT btns[] = {IDC_SES_BTN_LOAD, IDC_SES_BTN_NEW, IDC_SES_BTN_REN, IDC_SES_BTN_DEL, IDC_SES_BTN_DEF, IDC_SES_BTN_SAVE, IDC_SES_BTN_CANCEL};
    for (int i = 0; i < 7; ++i) {
        dlg::adjToEdge(hDlg, btns[i], dlgW, dlgH, r0.left, -1, -1, -1);
    }
    // Move the checkboxes
    dlg::adjToEdge(hDlg, IDC_SES_CHK_LIC, dlgW, dlgH, -1, r1.top, -1, -1);
    dlg::adjToEdge(hDlg, IDC_SES_CHK_LWC, dlgW, dlgH, -1, r0.top, -1, -1);
}

void onGetSize(HWND hDlg, LPMINMAXINFO p)
{
    if (_iniRect.left > 0) {
        p->ptMinTrackSize.x = _iniRect.right - _iniRect.left;
        p->ptMinTrackSize.y = _iniRect.bottom - _iniRect.top;
    }
}

} // end namespace

} // end namespace NppPlugin
