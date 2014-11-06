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
    @file      DlgSessions.cpp
    @copyright Copyright 2011,2013,2014 Michael Foster <http://mfoster.com/npp/>

    The "Sessions" dialog.
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
bool _inInit, _sortChanged;

bool onInit(HWND hDlg);
bool onOk(HWND hDlg);
bool onNew(HWND hDlg);
bool onRename(HWND hDlg);
bool onDelete(HWND hDlg);
bool onDefault(HWND hDlg);
void saveOptions(HWND hDlg);
bool onPrevious(HWND hDlg);
bool fillListBox(HWND hDlg, INT sesCurIdx = SI_CURRENT);
void onResize(HWND hDlg, INT dlgW = 0, INT dlgH = 0);
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
                app_saveSession(SI_CURRENT);
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
                ::EndDialog(hDlg, 0);
                return TRUE;
            case IDC_SES_CHK_LIC:
                if (ntfy == BN_CLICKED) {
                    if (dlg::getCheck(hDlg, IDC_SES_CHK_LIC)) {
                        dlg::setCheck(hDlg, IDC_SES_CHK_LWC, true);
                    }
                }
                return TRUE;
            case IDC_SES_RAD_ALPHA:
            case IDC_SES_RAD_DATE:
                if (!_inInit && ntfy == BN_CLICKED) {
                    _sortChanged = true;
                    saveOptions(hDlg);
                    app_readSessionDirectory();
                    fillListBox(hDlg);
                }
                return TRUE;
        }
    }
    else if (uMessage == WM_WINDOWPOSCHANGED) {
        LPWINDOWPOS wp = (LPWINDOWPOS)lParam;
        if (!(wp->flags & SWP_NOSIZE)) {
            onResize(hDlg);
        }
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

/* Child dialogs use this to get the vector index of the session selected in the list. */
INT dlgSes_getLbSelectedData()
{
    return _lbSelectedData;
}

//------------------------------------------------------------------------------

namespace {

/* Determines minimum dialog size. Populates controls. Resizes, centers and
   displays the dialog window. */
bool onInit(HWND hDlg)
{
    RECT r;
    bool ret = false;

    _inInit = true;
    _sortChanged = false;
    if (_minWidth == 0) {
        ::GetWindowRect(hDlg, &r);
        _minWidth = r.right - r.left;
        _minHeight = r.bottom - r.top;
    }
    dlg::setText(hDlg, IDC_SES_CTX_CUR, app_getSessionName(SI_CURRENT));
    dlg::setText(hDlg, IDC_SES_CTX_PRV, app_getSessionName(SI_PREVIOUS));
    dlg::setCheck(hDlg, IDC_SES_CHK_LIC, gCfg.loadIntoCurrentEnabled());
    dlg::setCheck(hDlg, IDC_SES_CHK_LWC, gCfg.loadWithoutClosingEnabled());
    bool alpha = gCfg.sortAlphaEnabled();
    dlg::setCheck(hDlg, IDC_SES_RAD_ALPHA, alpha);
    dlg::setCheck(hDlg, IDC_SES_RAD_DATE, !alpha);

    if (fillListBox(hDlg)) {
        INT w, h;
        gCfg.readSesDlgSize(&w, &h);
        if (w <= 0 || h <= 0) {
            w = 0;
            h = 0;
        }
        dlg::centerWnd(hDlg, sys_getNppHandle(), 0, 0, w, h, true);
        onResize(hDlg);
        ::ShowWindow(hDlg, SW_SHOW);
        ret = true;
    }

    _inInit = false;
    return ret;
}

bool onOk(HWND hDlg)
{
    ::EndDialog(hDlg, 0);
    app_loadSession(dlg::getLbSelData(hDlg, IDC_SES_LST_SES), dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

bool onDefault(HWND hDlg)
{
    ::EndDialog(hDlg, 0);
    app_loadSession(SI_DEFAULT, dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

void saveOptions(HWND hDlg)
{
    if (_sortChanged) {
        gCfg.setSortOrder(dlg::getCheck(hDlg, IDC_SES_RAD_ALPHA) ? SORT_ORDER_ALPHA : SORT_ORDER_DATE);
        gCfg.save();
        _sortChanged = false;
    }
}

bool onPrevious(HWND hDlg)
{
    ::EndDialog(hDlg, 0);
    app_loadSession(SI_PREVIOUS, dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

bool onNew(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (::DialogBox(sys_getDllHandle(), MAKEINTRESOURCE(IDD_NEW_DLG), hDlg, dlgNew_msgProc)) {
        app_readSessionDirectory();
        status = fillListBox(hDlg, app_getSessionIndex(dlgNew_getLbNewName()));
    }
    return status;
}

bool onRename(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (::DialogBox(sys_getDllHandle(), MAKEINTRESOURCE(IDD_REN_DLG), hDlg, dlgRen_msgProc)) {
        if (app_renameSession(_lbSelectedData, dlgRen_getLbNewName())) {
            dlg::setText(hDlg, IDC_SES_CTX_CUR, app_getSessionName(SI_CURRENT));
            dlg::setText(hDlg, IDC_SES_CTX_PRV, app_getSessionName(SI_PREVIOUS));
        }
        app_readSessionDirectory();
        status = fillListBox(hDlg, app_getSessionIndex(dlgRen_getLbNewName()));
    }
    return status;
}

bool onDelete(HWND hDlg)
{
    bool status = false;
    _lbSelectedData = dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    if (_lbSelectedData == app_getCurrentIndex()) {
        msg::show(L"Cannot delete current session.", M_WARN);
    }
    else if (::DialogBox(sys_getDllHandle(), MAKEINTRESOURCE(IDD_DEL_DLG), hDlg, dlgDel_msgProc)) {
        if (_lbSelectedData == app_getPreviousIndex()) {
            app_resetPreviousIndex();
            dlg::setText(hDlg, IDC_SES_CTX_PRV, SES_NAME_NONE);
        }
        app_readSessionDirectory();
        status = fillListBox(hDlg);
    }
    return status;
}

bool fillListBox(HWND hDlg, INT sesCurIdx)
{
    HWND hLst;
    INT i, sesIdx, sesCount;

    LOGF("%i", sesCurIdx);

    hLst = ::GetDlgItem(hDlg, IDC_SES_LST_SES);
    if (hLst) {
        ::SendMessage(hLst, LB_RESETCONTENT, 0, 0);
        sesCount = app_getSessionCount();
        if (sesCurIdx == SI_CURRENT) {
            sesCurIdx = app_getCurrentIndex();
        }
        for (sesIdx = 0; sesIdx < sesCount; ++sesIdx) {
            i = (INT)::SendMessage(hLst, LB_ADDSTRING, 0, (LPARAM)app_getSessionName(sesIdx));
            ::SendMessage(hLst, LB_SETITEMDATA, i, (LPARAM)sesIdx);
        }
        i = dlg::getLbIdxByData(hDlg, IDC_SES_LST_SES, sesCurIdx);
        if (i >= 0) {
            ::SendMessage(hLst, LB_SETCURSEL, i, 0);
        }
        return true;
    }
    return false;
}

/* Resizes and repositions the Sessions dialog controls. */
void onResize(HWND hDlg, INT dlgW, INT dlgH)
{
    RECT r;
    int i, len;

    //LOGE(31, "Sessions: w=%d, h=%d", dlgW, dlgH);
    if (dlgW == 0) {
        ::GetClientRect(hDlg, &r);
        dlgW = r.right;
        dlgH = r.bottom;
    }
    // Resize the Current and Previous control text
    dlg::adjToEdge(hDlg, IDC_SES_CTX_CUR, dlgW, dlgH, 4, IDC_SES_CTX_WRO, 0);
    dlg::adjToEdge(hDlg, IDC_SES_CTX_PRV, dlgW, dlgH, 4, IDC_SES_CTX_WRO, 0);
    // Resize the session list
    dlg::adjToEdge(hDlg, IDC_SES_LST_SES, dlgW, dlgH, 4|8, IDC_SES_LST_WRO, IDC_SES_LST_HBO);
    // Move the buttons
    INT btnCol[] = {IDC_SES_BTN_LOAD, IDC_SES_BTN_PRV, IDC_SES_BTN_DEF, IDC_SES_BTN_SAVE, IDC_SES_BTN_NEW, IDC_SES_BTN_REN, IDC_SES_BTN_DEL, IDC_SES_BTN_CANCEL};
    len = sizeof btnCol / sizeof INT;
    for (i = 0; i < len; ++i) {
        dlg::adjToEdge(hDlg, btnCol[i], dlgW, dlgH, 1, IDC_SES_BTN_XRO, 0);
    }
    // Move options row 1
    INT sortRow[] = {IDC_SES_CHK_LIC, IDC_SES_RAD_ALPHA};
    len = sizeof sortRow / sizeof INT;
    for (i = 0; i < len; ++i) {
        dlg::adjToEdge(hDlg, sortRow[i], dlgW, dlgH, 2, 0, IDC_SES_OPT_R1_YBO);
    }
    // Move options row 2
    INT loadRow[] = {IDC_SES_CHK_LWC, IDC_SES_RAD_DATE};
    len = sizeof loadRow / sizeof INT;
    for (i = 0; i < len; ++i) {
        dlg::adjToEdge(hDlg, loadRow[i], dlgW, dlgH, 2, 0, IDC_SES_OPT_R2_YBO, true);
    }

    // Save new dialog size
    ::GetWindowRect(hDlg, &r);
    gCfg.saveSesDlgSize(r.right - r.left, r.bottom - r.top);
}

/* Sets the minimum size the user can resize to. */
void onGetMinSize(HWND hDlg, LPMINMAXINFO p)
{
    p->ptMinTrackSize.x = _minWidth;
    p->ptMinTrackSize.y = _minHeight;
}

} // end namespace

} // end namespace NppPlugin
