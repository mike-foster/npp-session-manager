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
    @file      DlgSettings.cpp
    @copyright Copyright 2011-2015 Michael Foster <http://mfoster.com/npp/>

    The "Settings" dialog.
*/

#include "System.h"
#include "SessionMgr.h"
#include "DlgSettings.h"
#include "Util.h"
#include "ContextMenu.h"
#include "res\resource.h"
#include <commdlg.h>
#include <shlobj.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

INT _minWidth = 0, _minHeight = 0;
bool _inInit, _opChanged, _dirChanged;

INT onOk(HWND hDlg);
void onInit(HWND hDlg);
void onResize(HWND hDlg, INT w = 0, INT h = 0);
void onGetMinSize(HWND hDlg, LPMINMAXINFO p);
bool getFolderName(HWND parent, LPWSTR buf);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgCfg_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    INT okStatus;
    INT_PTR status = FALSE;

    if (uMessage == WM_COMMAND) {
        WORD ctrl = LOWORD(wParam);
        WORD ntfy = HIWORD(wParam);
        switch (ctrl) {
            case IDOK:
                okStatus = onOk(hDlg);
                if (okStatus == 2) {
                    msg::show(L"An error occurred while creating the new session directory.\nThis setting was not changed.", M_WARN);
                }
                ::EndDialog(hDlg, 1);
                status = TRUE;
                break;
            case IDCANCEL:
                ::EndDialog(hDlg, 0);
                status = TRUE;
                break;
            case IDC_CFG_CHK_ASV:
            case IDC_CFG_CHK_ALD:
            case IDC_CFG_CHK_LIC:
            case IDC_CFG_CHK_LWC:
            case IDC_CFG_CHK_SITB:
            case IDC_CFG_CHK_SISB:
            case IDC_CFG_CHK_GBKM:
                if (!_inInit && ntfy == BN_CLICKED) {
                    _opChanged = true;
                }
                status = TRUE;
                break;
            case IDC_CFG_CHK_CTXM:
                if (!_inInit && ntfy == BN_CLICKED) {
                    _opChanged = true;
                    if (cfg::getBool(kUseContextMenu) && !dlg::getCheck(hDlg, IDC_CFG_CHK_CTXM)) {
                        ctx::unload();
                    }
                }
                status = TRUE;
                break;
            case IDC_CFG_ETX_DIR:
            case IDC_CFG_ETX_EXT:
                if (!_inInit && ntfy == EN_CHANGE) {
                    _dirChanged = true;
                }
                status = TRUE;
                break;
            case IDC_CFG_BTN_BRW:
                if (!_inInit && ntfy == BN_CLICKED) {
                    WCHAR pthBuf[MAX_PATH];
                    if (getFolderName(hDlg, pthBuf)) {
                        _dirChanged = true;
                        dlg::setText(hDlg, IDC_CFG_ETX_DIR, pthBuf);
                    }
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

    return status;
}

//------------------------------------------------------------------------------

namespace {

/** Determines minimum dialog size. Populates controls with current values from
    settings. Resizes, centers and displays the dialog window. */
void onInit(HWND hDlg)
{
    RECT r;

    _inInit = true;
    _opChanged = false;
    _dirChanged = false;
    if (_minWidth == 0) {
        ::GetWindowRect(hDlg, &r);
        _minWidth = r.right - r.left;
        _minHeight = r.bottom - r.top;
    }
    // init control values
    dlg::setCheck(hDlg, IDC_CFG_CHK_ASV,  cfg::getBool(kAutomaticSave));
    dlg::setCheck(hDlg, IDC_CFG_CHK_ALD,  cfg::getBool(kAutomaticLoad));
    dlg::setCheck(hDlg, IDC_CFG_CHK_LIC,  cfg::getBool(kLoadIntoCurrent));
    dlg::setCheck(hDlg, IDC_CFG_CHK_LWC,  cfg::getBool(kLoadWithoutClosing));
    dlg::setCheck(hDlg, IDC_CFG_CHK_SITB, cfg::getBool(kShowInTitlebar));
    dlg::setCheck(hDlg, IDC_CFG_CHK_SISB, cfg::getBool(kShowInStatusbar));
    dlg::setCheck(hDlg, IDC_CFG_CHK_GBKM, cfg::getBool(kUseGlobalProperties));
    dlg::setCheck(hDlg, IDC_CFG_CHK_CTXM, cfg::getBool(kUseContextMenu));
    dlg::setText(hDlg, IDC_CFG_ETX_DIR, cfg::getStr(kSessionDirectory));
    dlg::setText(hDlg, IDC_CFG_ETX_EXT, cfg::getStr(kSessionExtension));
    // focus the first edit control
    dlg::focus(hDlg, IDC_CFG_ETX_DIR);
    // resize, center and show the window
    INT w = cfg::getInt(kSettingsDialogWidth), h = cfg::getInt(kSettingsDialogHeight);
    if (w <= 0 || h <= 0) {
        w = 0;
        h = 0;
    }
    dlg::centerWnd(hDlg, sys_getNppHandle(), 0, 0, w, h, true);
    onResize(hDlg);
    ::ShowWindow(hDlg, SW_SHOW);
    _inInit = false;
}

/** Gets values, if changed, from dialog box controls. Updates the global
    config object and save them to the ini file. */
INT onOk(HWND hDlg)
{
    INT stat = 0;
    bool change = false;
    WCHAR buf[MAX_PATH];

    if (_opChanged) {
        change = true;
        cfg::putBool(kAutomaticSave, dlg::getCheck(hDlg, IDC_CFG_CHK_ASV));
        cfg::putBool(kAutomaticLoad, dlg::getCheck(hDlg, IDC_CFG_CHK_ALD));
        cfg::putBool(kLoadIntoCurrent, dlg::getCheck(hDlg, IDC_CFG_CHK_LIC));
        cfg::putBool(kLoadWithoutClosing, dlg::getCheck(hDlg, IDC_CFG_CHK_LWC));
        cfg::putBool(kShowInTitlebar, dlg::getCheck(hDlg, IDC_CFG_CHK_SITB));
        cfg::putBool(kShowInStatusbar, dlg::getCheck(hDlg, IDC_CFG_CHK_SISB));
        cfg::putBool(kUseGlobalProperties, dlg::getCheck(hDlg, IDC_CFG_CHK_GBKM));
        cfg::putBool(kUseContextMenu, dlg::getCheck(hDlg, IDC_CFG_CHK_CTXM));
    }
    if (_dirChanged) {
        change = true;
        dlg::getText(hDlg, IDC_CFG_ETX_DIR, buf, MAX_PATH);
        if (!cfg::setSessionDirectory(buf)) {
            stat = 2; // error creating ses dir
        }
        dlg::getText(hDlg, IDC_CFG_ETX_EXT, buf, MAX_PATH);
        cfg::setSessionExtension(buf);
    }

    if (change) {
        cfg::saveSettings();
        if (_dirChanged) {
            app_readSessionDirectory();
        }
    }
    else {
        stat = 1; // no changes
    }

    return stat;
}

/** Resizes and repositions dialog controls. */
void onResize(HWND hDlg, INT dlgW, INT dlgH)
{
    RECT r;

    if (dlgW == 0) {
        ::GetClientRect(hDlg, &r);
        dlgW = r.right;
        dlgH = r.bottom;
    }
    // Resize the Directory and Extension edit boxes
    dlg::adjToEdge(hDlg, IDC_CFG_ETX_DIR, dlgW, dlgH, 4, IDC_CFG_ETX_WRO, 0);
    dlg::adjToEdge(hDlg, IDC_CFG_ETX_EXT, dlgW, dlgH, 4, IDC_CFG_ETX_WRO, 0);
    // Move the OK and Cancel buttons
    dlg::adjToEdge(hDlg, IDOK, dlgW, dlgH, 1|2, IDC_CFG_BTN_OK_XRO, IDC_CFG_BTN_YBO);
    dlg::adjToEdge(hDlg, IDCANCEL, dlgW, dlgH, 1|2, IDC_CFG_BTN_CAN_XRO, IDC_CFG_BTN_YBO, true);
    // Save new dialog size
    ::GetWindowRect(hDlg, &r);
    cfg::putInt(kSettingsDialogWidth, r.right - r.left);
    cfg::putInt(kSettingsDialogHeight, r.bottom - r.top);
}

/** Sets the minimum size the user can resize to. */
void onGetMinSize(HWND hDlg, LPMINMAXINFO p)
{
    p->ptMinTrackSize.x = _minWidth;
    p->ptMinTrackSize.y = _minHeight;
}

/** Copied and slightly modifed from: npp.6.2.3.src\PowerEditor\src\MISC\Common\Common.cpp */
bool getFolderName(HWND parent, LPWSTR buf)
{
    bool ok = false;
    LPMALLOC pShellMalloc = 0;

    if (::SHGetMalloc(&pShellMalloc) == NO_ERROR) {
        BROWSEINFOW info;
        ::memset(&info, 0, sizeof(info));
        info.hwndOwner = parent;
        info.pidlRoot = NULL;
        WCHAR displayName[MAX_PATH];
        info.pszDisplayName = displayName;
        info.lpszTitle = L"Select a sessions folder";
        info.ulFlags = BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = ::SHBrowseForFolderW(&info);
        // pidl will be null if they cancel the browse dialog, else not null if they select a folder.
        if (pidl) {
            if (::SHGetPathFromIDListW(pidl, buf)) {
                ok = true;
            }
            pShellMalloc->Free(pidl);
        }
        pShellMalloc->Release();
    }
    return ok;
}

} // end namespace

} // end namespace NppPlugin
