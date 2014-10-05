/*
    DlgSessions.cpp
    Copyright 2011,2013,2014 Michael Foster (http://mfoster.com/npp/)

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
INT _minWidth = 0, _minHeight = 0;

bool onInit(HWND hDlg);
bool onOk(HWND hDlg);
bool onNew(HWND hDlg);
bool onRename(HWND hDlg);
bool onDelete(HWND hDlg);
bool onDefault(HWND hDlg);
bool onPrevious(HWND hDlg);
bool fillListBox(HWND hDlg, INT sesCurIdx = SES_CURRENT);
void onResize(HWND hDlg, INT dlgW, INT dlgH);
void onGetMinSize(HWND hDlg, LPMINMAXINFO p);

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
            case IDC_SES_BTN_PRV:
                if (onPrevious(hDlg)) {
                    return TRUE;
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
        onGetMinSize(hDlg, (LPMINMAXINFO)lParam);
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

/* Determines minimum dialog size. Populates controls. Resizes, centers and
   displays the dialog window.
   XXX Adding 6 and 24 is magic. */
bool onInit(HWND hDlg)
{
    bool ret = false;

    if (_minWidth == 0) {
        RECT rect = {0, 0, IDD_SES_W, IDD_SES_H};
        MapDialogRect(hDlg, &rect);
        _minWidth = rect.right - rect.left + 6;
        _minHeight = rect.bottom - rect.top + 24;
    }
    dlg::setText(hDlg, IDC_SES_TXT_CUR, app_getSessionName(SES_CURRENT));
    dlg::setText(hDlg, IDC_SES_TXT_PRV, app_getSessionName(SES_PREVIOUS));
    dlg::setCheck(hDlg, IDC_SES_CHK_LIC, gCfg.getLoadIntoCurrent());
    dlg::setCheck(hDlg, IDC_SES_CHK_LWC, gCfg.getLoadWithoutClosing());

    if (fillListBox(hDlg)) {
        INT w, h;
        gCfg.readSesDlgSize(&w, &h);
        if (w <= 0 || h <= 0) {
            w = 0;
            h = 0;
        }
        else {
            w += 6;
            h += 24;
        }
        dlg::centerWnd(hDlg, sys_getNppHwnd(), 0, 0, w, h, true);
        ShowWindow(hDlg, SW_SHOW);
        ret = true;
    }

    return ret;
}

bool onOk(HWND hDlg)
{
    EndDialog(hDlg, 0);
    app_loadSession(dlg::getLbSelData(hDlg, IDC_SES_LST_SES), dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

bool onDefault(HWND hDlg)
{
    EndDialog(hDlg, 0);
    app_loadSession(SES_DEFAULT, dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

bool onPrevious(HWND hDlg)
{
    EndDialog(hDlg, 0);
    app_loadSession(SES_PREVIOUS, dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

bool onNew(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_NEW_DLG), hDlg, dlgNew_msgProc)) {
        app_readSessionDirectory();
        status = fillListBox(hDlg, app_getSessionIndex(dlgNew_getLbNewName()));
    }
    return status;
}

bool onRename(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_REN_DLG), hDlg, dlgRen_msgProc)) {
        app_readSessionDirectory();
        status = fillListBox(hDlg, app_getSessionIndex(dlgRen_getLbNewName()));
    }
    return status;
}

bool onDelete(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_DEL_DLG), hDlg, dlgDel_msgProc)) {
        app_readSessionDirectory();
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
        sesCount = app_getSessionCount();
        if (sesCurIdx == SES_CURRENT) {
            sesCurIdx = app_getSessionIndex();
        }
        for (sesIdx = 0; sesIdx < sesCount; ++sesIdx) {
            i = (INT)SendMessage(hLst, LB_ADDSTRING, 0, (LPARAM)app_getSessionName(sesIdx));
            SendMessage(hLst, LB_SETITEMDATA, i, (LPARAM)sesIdx);
        }
        i = dlg::getLbIdxByData(hDlg, IDC_SES_LST_SES, sesCurIdx);
        if (i >= 0) {
            SendMessage(hLst, LB_SETCURSEL, i, 0);
        }
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
    // Resize the Current and Previous text
    dlg::adjToEdge(hDlg, IDC_SES_TXT_CUR, dlgW, dlgH, -1, -1, r0.left, -1);
    dlg::adjToEdge(hDlg, IDC_SES_TXT_PRV, dlgW, dlgH, -1, -1, r0.left, -1);
    // Move the buttons
    INT btns[] = {IDC_SES_BTN_LOAD, IDC_SES_BTN_PRV, IDC_SES_BTN_DEF, IDC_SES_BTN_SAVE, IDC_SES_BTN_NEW, IDC_SES_BTN_REN, IDC_SES_BTN_DEL, IDC_SES_BTN_CANCEL};
    int len = sizeof btns / sizeof INT;
    for (int i = 0; i < len; ++i) {
        dlg::adjToEdge(hDlg, btns[i], dlgW, dlgH, r0.left, -1, -1, -1);
    }
    // Move the checkboxes
    dlg::adjToEdge(hDlg, IDC_SES_CHK_LIC, dlgW, dlgH, -1, r1.top, -1, -1);
    dlg::adjToEdge(hDlg, IDC_SES_CHK_LWC, dlgW, dlgH, -1, r0.top, -1, -1);
    // Save new dialog size
    gCfg.saveSesDlgSize(dlgW, dlgH);

    LOGE(31, "Sessions: w=%d, h=%d", dlgW, dlgH);
}

/* Sets the minimum size the user can resize to. */
void onGetMinSize(HWND hDlg, LPMINMAXINFO p)
{
    p->ptMinTrackSize.x = _minWidth;
    p->ptMinTrackSize.y = _minHeight;
}

} // end namespace

} // end namespace NppPlugin
