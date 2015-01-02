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
    @copyright Copyright 2011,2013-2015 Michael Foster <http://mfoster.com/npp/>

    The "Sessions" dialog.
*/

#include "System.h"
#include "SessionMgr.h"
#include "DlgSessions.h"
#include "DlgNew.h"
#include "DlgRename.h"
#include "DlgDelete.h"
#include "Util.h"
#include "res\resource.h"
#include <strsafe.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

bool _inInit, _favoriteChanged;
INT _minWidth = 0, _minHeight = 0;
WCHAR _currentFilter[FILTER_BUF_LEN];

void onInit(HWND hDlg);
bool onOk(HWND hDlg);
bool onFilterChange(HWND hDlg, WORD ntfy);
bool onNew(HWND hDlg);
bool onRename(HWND hDlg);
bool onDelete(HWND hDlg);
bool onPrevious(HWND hDlg);
INT onVirtualKey(HWND hDlg, WCHAR vKey);
INT getSelSesIdx(HWND hDlg);
void populateFiltersList(HWND hDlg);
bool isFiltered(LPCWSTR sesName);
void getSessionMark(Session *ses, LPWSTR buf);
bool populateSessionsList(HWND hDlg, INT sesSelIdx = SI_CURRENT);
void onResize(HWND hDlg, INT dlgW = 0, INT dlgH = 0);
void onGetMinSize(HWND hDlg, LPMINMAXINFO p);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgSes_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    INT_PTR status = FALSE;
    WORD ctrl = LOWORD(wParam);
    WORD ntfy = HIWORD(wParam);

    if (uMessage == WM_COMMAND) {
        switch (ctrl) {
            case IDC_SES_LST_SES:
                if (ntfy == LBN_DBLCLK) {
                    if (onOk(hDlg)) {
                        status = TRUE;
                    }
                }
                break;
            case IDOK:
            case IDC_SES_BTN_LOAD:
                if (onOk(hDlg)) {
                    status = TRUE;
                }
                break;
            case IDC_SES_CMB_FIL:
                if (!_inInit && (ntfy == CBN_SELCHANGE || ntfy == CBN_KILLFOCUS)) {
                    if (onFilterChange(hDlg, ntfy)) {
                        status = TRUE;
                    }
                }
                break;
            case IDC_SES_BTN_PRV:
                if (onPrevious(hDlg)) {
                    status = TRUE;
                }
                break;
            case IDC_SES_BTN_SAVE:
                app_saveSession(SI_CURRENT);
                dlg::focus(hDlg, IDC_SES_BTN_LOAD, false);
                status = TRUE;
                break;
            case IDC_SES_BTN_NEW:
                if (ntfy == BN_CLICKED) {
                    dlg::focus(hDlg, IDC_SES_BTN_LOAD, false);
                    if (onNew(hDlg)) {
                        status = TRUE;
                    }
                }
                break;
            case IDC_SES_BTN_REN:
                if (ntfy == BN_CLICKED) {
                    dlg::focus(hDlg, IDC_SES_BTN_LOAD, false);
                    if (onRename(hDlg)) {
                        status = TRUE;
                    }
                }
                break;
            case IDC_SES_BTN_DEL:
                if (ntfy == BN_CLICKED) {
                    dlg::focus(hDlg, IDC_SES_BTN_LOAD, false);
                    if (onDelete(hDlg)) {
                        status = TRUE;
                    }
                }
                break;
            case IDC_SES_BTN_FAV:
                if (ntfy == BN_CLICKED) {
                    Session *ses = (Session*)dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
                    ses->isFavorite = !ses->isFavorite;
                    WCHAR tmpBuf[SES_NAME_BUF_LEN + 3];
                    getSessionMark(ses, tmpBuf);
                    ::StringCchCatW(tmpBuf, SES_NAME_BUF_LEN + 3, ses->name);
                    dlg::lbReplaceSelItem(hDlg, IDC_SES_LST_SES, tmpBuf, (LPARAM)ses);
                    _favoriteChanged = true;
                }
                break;
            case IDCANCEL:
            case IDC_SES_BTN_CANCEL:
                ::EndDialog(hDlg, 0);
                if (_favoriteChanged) {
                    app_updateFavorites();
                }
                status = TRUE;
                break;
            case IDC_SES_CHK_LIC:
                if (ntfy == BN_CLICKED) {
                    if (dlg::getCheck(hDlg, IDC_SES_CHK_LIC)) {
                        dlg::setCheck(hDlg, IDC_SES_CHK_LWC, true);
                    }
                }
                status = TRUE;
                break;
            case IDC_SES_RAD_ALPHA:
            case IDC_SES_RAD_DATE:
                if (!_inInit && ntfy == BN_CLICKED) {
                    cfg::putInt(kSessionSortOrder, dlg::getCheck(hDlg, IDC_SES_RAD_ALPHA) ? SORT_ORDER_ALPHA : SORT_ORDER_DATE);
                    app_readSessionDirectory();
                    populateSessionsList(hDlg);
                }
                status = TRUE;
                break;
        } // end switch
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
        onInit(hDlg);
    }
    else if (uMessage == WM_VKEYTOITEM) {
        //LOG("WM_VKEYTOITEM: vkey=%i, pos=%i", ctrl, ntfy);
        status = onVirtualKey(hDlg, (WCHAR)ctrl);
    }

    return status;
}

//------------------------------------------------------------------------------

namespace {

/** Determines minimum dialog size. Populates controls. Resizes, centers and
    displays the dialog window. */
void onInit(HWND hDlg)
{
    RECT r;
    bool alpha;

    _favoriteChanged = false;
    _inInit = true;
    if (_minWidth == 0) {
        ::GetWindowRect(hDlg, &r);
        _minWidth = r.right - r.left;
        _minHeight = r.bottom - r.top;
    }
    dlg::setCheck(hDlg, IDC_SES_CHK_LIC, cfg::getBool(kLoadIntoCurrent));
    dlg::setCheck(hDlg, IDC_SES_CHK_LWC, cfg::getBool(kLoadWithoutClosing));
    alpha = cfg::isSortAlpha();
    dlg::setCheck(hDlg, IDC_SES_RAD_ALPHA, alpha);
    dlg::setCheck(hDlg, IDC_SES_RAD_DATE, !alpha);
    populateFiltersList(hDlg);
    populateSessionsList(hDlg);
    INT w = cfg::getInt(kSessionsDialogWidth), h = cfg::getInt(kSessionsDialogHeight);
    if (w <= 0 || h <= 0) {
        w = 0;
        h = 0;
    }
    dlg::centerWnd(hDlg, sys_getNppHandle(), 0, 0, w, h, true);
    onResize(hDlg);
    ::ShowWindow(hDlg, SW_SHOW);
    dlg::focus(hDlg, IDC_SES_LST_SES);

    _inInit = false;
}

bool onOk(HWND hDlg)
{
    ::EndDialog(hDlg, 1);
    if (_favoriteChanged) {
        app_updateFavorites();
    }
    app_loadSession(getSelSesIdx(hDlg), dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

bool onFilterChange(HWND hDlg, WORD ntfy)
{
    WCHAR buf[FILTER_BUF_LEN];

    if (ntfy == CBN_SELCHANGE) {
        dlg::getCbSelText(hDlg, IDC_SES_CMB_FIL, buf);
    }
    else {
        dlg::getText(hDlg, IDC_SES_CMB_FIL, buf, FILTER_BUF_LEN);
    }
    if (buf[0] != 0 && ::wcscmp(_currentFilter, buf) != 0) {
        ::StringCchCopyW(_currentFilter, FILTER_BUF_LEN, buf);
        cfg::moveToTop(kFilters, _currentFilter);
        populateFiltersList(hDlg);
        populateSessionsList(hDlg);
        if (ntfy == CBN_SELCHANGE) {
            _inInit = true;
            dlg::focus(hDlg, IDC_SES_LST_SES, false);
            _inInit = false;
        }
        return true;
    }
    return false;
}

bool onPrevious(HWND hDlg)
{
    ::EndDialog(hDlg, 1);
    if (_favoriteChanged) {
        app_updateFavorites();
    }
    app_loadSession(SI_PREVIOUS, dlg::getCheck(hDlg, IDC_SES_CHK_LIC), dlg::getCheck(hDlg, IDC_SES_CHK_LWC));
    return true;
}

bool onNew(HWND hDlg)
{
    bool status = false;
    ChildDialogData cdd;
    cdd.selectedSessionIndex = getSelSesIdx(hDlg);
    if (::DialogBoxParam(sys_getDllHandle(), MAKEINTRESOURCE(IDD_NEW_DLG), hDlg, dlgNew_msgProc, (LPARAM)&cdd)) {
        app_readSessionDirectory();
        status = populateSessionsList(hDlg, app_getSessionIndex(cdd.newSessionName));
    }
    return status;
}

bool onRename(HWND hDlg)
{
    bool status = false;
    ChildDialogData cdd;
    Session *ses = (Session*)dlg::getLbSelData(hDlg, IDC_SES_LST_SES);

    cdd.selectedSessionIndex = ses->index;
    if (::DialogBoxParam(sys_getDllHandle(), MAKEINTRESOURCE(IDD_REN_DLG), hDlg, dlgRen_msgProc, (LPARAM)&cdd)) {
        if (ses->isFavorite) {
            _favoriteChanged = true;
        }
        app_renameSession(ses->index, cdd.newSessionName);
        app_readSessionDirectory();
        status = populateSessionsList(hDlg, app_getSessionIndex(cdd.newSessionName));
    }
    return status;
}

bool onDelete(HWND hDlg)
{
    bool status = false;
    ChildDialogData cdd;
    Session *ses = (Session*)dlg::getLbSelData(hDlg, IDC_SES_LST_SES);

    cdd.selectedSessionIndex = ses->index;
    if (ses->index == app_getDefaultIndex()) {
        msg::show(L"Cannot delete the default session.", M_WARN);
    }
    else if (ses->index == app_getCurrentIndex()) {
        msg::show(L"Cannot delete the current session.", M_WARN);
    }
    else if (::DialogBoxParam(sys_getDllHandle(), MAKEINTRESOURCE(IDD_DEL_DLG), hDlg, dlgDel_msgProc, (LPARAM)&cdd)) {
        if (ses->isFavorite) {
            _favoriteChanged = true;
        }
        if (ses->index == app_getPreviousIndex()) {
            app_resetPreviousIndex();
        }
        app_readSessionDirectory();
        status = populateSessionsList(hDlg);
    }
    return status;
}

/** Selects the session whose name begins with the alphanumeric key pressed.
    @return -2 if the event was handled else -1 */
INT onVirtualKey(HWND hDlg, WCHAR vKey)
{
    INT lbIdx;
    WCHAR ch = 0;

    if (vKey == VK_OEM_MINUS) { // TODO: '_' = VK_SHIFT followed by VK_OEM_MINUS
        ch = L'-';
    }
    else if ((vKey >= 0x30 && vKey <= 0x39) || (vKey >= 0x41 && vKey <= 0x5A)) { // '0' - '9' or 'A' - 'Z'
        ch = vKey;
    }
    if (ch > 0) {
        lbIdx = app_getLbIdxStartingWith(ch);
        if (lbIdx >= 0) {
            ::SendMessage(::GetDlgItem(hDlg, IDC_SES_LST_SES), LB_SETCURSEL, lbIdx, 0);
        }
        return -2;
    }
    return -1;
}

INT getSelSesIdx(HWND hDlg)
{
    Session *ses = (Session*)dlg::getLbSelData(hDlg, IDC_SES_LST_SES);
    return ses ? ses->index : app_getDefaultIndex();
}

void populateFiltersList(HWND hDlg)
{
    HWND hCmb;
    INT i = 0;
    LPCWSTR exp;

    LOGF("");
    hCmb = ::GetDlgItem(hDlg, IDC_SES_CMB_FIL);
    if (hCmb) {
        ::SendMessage(hCmb, CB_LIMITTEXT, FILTER_BUF_LEN - 1, 0);
        // XXX ::SendMessage(hCmb, CB_SETMINVISIBLE, 7, 0);
        ::SendMessage(hCmb, CB_RESETCONTENT, 0, 0);
        do {
            exp = cfg::getStr(kFilters, i);
            if (exp) {
                ++i;
                ::SendMessage(hCmb, CB_ADDSTRING, 0, (LPARAM)exp);
            }
        } while (exp != NULL);
        if (i > 0) {
            ::SendMessage(hCmb, CB_SETCURSEL, 0, 0);
            ::StringCchCopyW(_currentFilter, FILTER_BUF_LEN, cfg::getStr(kFilters, 0));
        }
        else {
            _currentFilter[0] = 0;
        }
    }
}

/** Filters the current and previous sessions and any session that contains the
    filter. No filter or '*' filters everything.
    @return true if sesName should be displayed in the sessions list, else false */
bool isFiltered(LPCWSTR sesName)
{
    INT sesIdx = app_getSessionIndex(sesName);

    if (_currentFilter[0] == 0 || _currentFilter[0] == L'*' ||
        app_getCurrentIndex() == sesIdx || app_getPreviousIndex() == sesIdx ||
        ::wcsstr(sesName, _currentFilter)
    ) {
        return true;
    }
    return false;
}

/** Determines the mark to be used for ses, if any, and writes it to buf. */
void getSessionMark(Session *ses, LPWSTR buf)
{
    INT sesCurIdx, sesPrvIdx, sesDefIdx;

    sesCurIdx = app_getCurrentIndex();
    sesPrvIdx = app_getPreviousIndex();
    sesDefIdx = app_getDefaultIndex();
    if (ses->isFavorite) {
        if (ses->index == sesCurIdx) cfg::getMarkStr(kCurrentFavMark, buf);
        else if (ses->index == sesPrvIdx) cfg::getMarkStr(kPreviousFavMark, buf);
        else if (ses->index == sesDefIdx) cfg::getMarkStr(kDefaultFavMark, buf);
        else cfg::getMarkStr(kFavoriteMark, buf);
    }
    else {
        if (ses->index == sesCurIdx) cfg::getMarkStr(kCurrentMark, buf);
        else if (ses->index == sesPrvIdx) cfg::getMarkStr(kPreviousMark, buf);
        else if (ses->index == sesDefIdx) cfg::getMarkStr(kDefaultMark, buf);
        else ::StringCchCopyW(buf, 3, L" \t");
    }
}

bool populateSessionsList(HWND hDlg, INT sesSelIdx)
{
    HWND hLst;
    Session *ses;
    WCHAR buf[SES_NAME_BUF_LEN + 3];
    INT lbIdx, lbSelIdx = -1, sesIdx, sesCount;
    INT tabStops[1] = {8};

    LOGF("%i", sesSelIdx);
    hLst = ::GetDlgItem(hDlg, IDC_SES_LST_SES);
    if (hLst) {
        ::SendMessage(hLst, WM_SETREDRAW, FALSE, 0);
        ::SendMessage(hLst, LB_RESETCONTENT, 0, 0);
        ::SendMessage(hLst, LB_SETTABSTOPS, 1, (LPARAM)tabStops);
        sesCount = app_getSessionCount();
        if (sesSelIdx == SI_CURRENT) {
            sesSelIdx = app_getCurrentIndex();
        }
        for (sesIdx = 0; sesIdx < sesCount; ++sesIdx) {
            ses = app_getSessionObject(sesIdx);
            if (ses) {
                if (isFiltered(ses->name)) {
                    ses->isVisible = true;
                    getSessionMark(ses, buf);
                    ::StringCchCatW(buf, SES_NAME_BUF_LEN + 3, ses->name);
                    lbIdx = (INT)::SendMessage(hLst, LB_INSERTSTRING, -1, (LPARAM)buf);
                    ::SendMessage(hLst, LB_SETITEMDATA, lbIdx, (LPARAM)ses);
                    if (sesIdx == sesSelIdx) {
                        lbSelIdx = lbIdx;
                    }
                }
                else {
                    ses->isVisible = false;
                }
            }
        }
        if (lbSelIdx >= 0) {
            ::SendMessage(hLst, LB_SETCURSEL, lbSelIdx, 0);
        }
        ::SendMessage(hLst, WM_SETREDRAW, TRUE, 0);
        ::RedrawWindow(hLst, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
        return true;
    }
    return false;
}

/* Resizes and repositions the Sessions dialog controls. */
void onResize(HWND hDlg, INT dlgW, INT dlgH)
{
    RECT r;
    int i, len;

    if (dlgW == 0) {
        ::GetClientRect(hDlg, &r);
        dlgW = r.right;
        dlgH = r.bottom;
    }
    // Resize the filters combo box
    dlg::adjToEdge(hDlg, IDC_SES_CMB_FIL, dlgW, dlgH, 4, IDC_SES_CMB_WRO, 0);
    // Resize the session list
    dlg::adjToEdge(hDlg, IDC_SES_LST_SES, dlgW, dlgH, 4|8, IDC_SES_LST_WRO, IDC_SES_LST_HBO);
    // Move the buttons
    INT btnCol[] = {IDC_SES_BTN_LOAD, IDC_SES_BTN_PRV, IDC_SES_BTN_SAVE, IDC_SES_BTN_NEW, IDC_SES_BTN_REN, IDC_SES_BTN_DEL, IDC_SES_BTN_FAV, IDC_SES_BTN_CANCEL};
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
    cfg::putInt(kSessionsDialogWidth, r.right - r.left);
    cfg::putInt(kSessionsDialogHeight, r.bottom - r.top);
}

/* Sets the minimum size the user can resize to. */
void onGetMinSize(HWND hDlg, LPMINMAXINFO p)
{
    p->ptMinTrackSize.x = _minWidth;
    p->ptMinTrackSize.y = _minHeight;
}

} // end namespace

} // end namespace NppPlugin
