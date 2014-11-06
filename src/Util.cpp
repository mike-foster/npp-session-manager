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
    @file      Util.cpp
    @copyright Copyright 2011-2014 Michael Foster <http://mfoster.com/npp/>
*/

#include "System.h"
#include "SessionMgr.h"
#include "Config.h"
#include "Util.h"
#include <strsafe.h>
//#include <commctrl.h> // XXX experimental for statusbar and tooltips

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace msg {

/** Displays a simple message box. For title/options see the M_* constants. */
INT show(LPCWSTR msg, LPWSTR title, UINT options)
{
    return ::MessageBoxW(sys_getNppHandle(), msg, title != NULL ? title : PLUGIN_FULL_NAME, options);
}

/** Displays an error message and logs it. lastError is expected to be from GetLastError. */
void error(DWORD lastError, LPCWSTR format, ...)
{
    INT len;
    va_list argptr;
    LPWSTR buf, buf1, buf2 = NULL, lem, lastErrorMsg = NULL;

    va_start(argptr, format);
    len = ::_vscwprintf(format, argptr);
    buf1 = (LPWSTR)sys_alloc((len + 2) * sizeof(WCHAR));
    if (buf1 == NULL) {
        log("Error in msg::error allocating memory for: \"%S\".", format);
        return;
    }
    ::vswprintf_s(buf1, len + 1, format, argptr);
    va_end(argptr);
    buf = buf1;

    if (lastError) {
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lastErrorMsg, 0, NULL);
        lem = lastErrorMsg == NULL ? EMPTY_STR : lastErrorMsg;
        LPCWSTR fmt = L"%s\nError %i: %s";
        len = ::_scwprintf(fmt, buf1, lastError, lem);
        buf2 = (LPWSTR)sys_alloc((len + 2) * sizeof(WCHAR));
        if (buf2) {
            ::swprintf_s(buf2, len + 1, fmt, buf1, lastError, lem);
            buf = buf2;
        }
    }

    show(buf, M_ERR);
    log("%S", buf);
    sys_free(buf1);
    sys_free(buf2);
    if (lastErrorMsg) {
        LocalFree((HLOCAL)lastErrorMsg);
    }
}

/** Writes a formatted UTF-8 string to the debug log file. */
void log(const char *format, ...)
{
    if (gCfg.debug && gCfg.logFile[0]) {
        FILE *fp;
        ::_wfopen_s(&fp, gCfg.logFile, L"a+");
        if (fp) {
            va_list argptr;
            va_start(argptr, format);
            ::vfprintf(fp, format, argptr); // Expects 'format' to be UTF-8, vfwprintf_s would write UTF-16
            va_end(argptr);
            ::fputwc(L'\n', fp);
            ::fflush(fp);
            ::fclose(fp);
        }
    }
}

} // end namespace msg

//------------------------------------------------------------------------------

namespace pth {

/** Removes the file name extension.
    @return non-zero on error */
errno_t removeExt(LPWSTR buf, size_t bufLen)
{
    errno_t err;
    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME];
    err = ::_wsplitpath_s(buf, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, NULL, 0);
    if (err == 0) {
        err = ::_wmakepath_s(buf, bufLen, drive, dir, fname, NULL);
    }
    return err;
}

/** Removes the file name, leaving only the path and trailing slash.
    @return non-zero on error */
errno_t removeName(LPWSTR buf, size_t bufLen)
{
    errno_t err;
    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR];
    err = ::_wsplitpath_s(buf, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
    if (err == 0) {
        err = ::_wmakepath_s(buf, bufLen, drive, dir, NULL, NULL);
    }
    return err;
}

/** Removes the path, leaving only the file name.
    @return non-zero on error */
errno_t removePath(LPWSTR buf, size_t bufLen)
{
    errno_t err;
    WCHAR fname[_MAX_FNAME], ext[_MAX_EXT];
    err = ::_wsplitpath_s(buf, NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
    if (err == 0) {
        err = ::_wmakepath_s(buf, bufLen, NULL, NULL, fname, ext);
    }
    return err;
}

/** Appends a backslash if a trailing backslash or foreslash is not already present. */
void appendSlash(LPWSTR buf, size_t bufLen)
{
    LPWSTR p;
    if (buf != NULL) {
        p = ::wcsrchr(buf, L'\0');
        if (p != NULL) {
            p = ::CharPrevW(buf, p);
            if (p != NULL && *p != L'\\' && *p != L'/') {
                ::StringCchCatW(buf, bufLen, L"\\");
            }
        }
    }
}

/** @return true if path is an existing directory */
bool dirExists(LPCWSTR path)
{
    DWORD a = ::GetFileAttributesW(path);
    return (bool)(a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY));
}

/** @return true if pathname is an existing file */
bool fileExists(LPCWSTR pathname)
{
    DWORD a = ::GetFileAttributesW(pathname);
    return (bool)(a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY));
}

/** Creates a new file with initial contents, if the file doesn't already exist. */
void createFileIfMissing(LPCWSTR pathname, const char *contents)
{
    BOOL suc;
    HANDLE hFile;
    DWORD len, bytes;

    hFile = ::CreateFileW(pathname, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        len = ::strlen(contents);
        suc = ::WriteFile(hFile, contents, len, &bytes, NULL);
        if (!suc || bytes != len) {
            WCHAR msg[MAX_PATH];
            ::StringCchCopyW(msg, MAX_PATH, L"Failed creating file: ");
            ::StringCchCatW(msg, MAX_PATH, pathname);
            msg::show(msg, M_ERR);
        }
        ::CloseHandle(hFile);
    }
}

} // end namespace pth

//------------------------------------------------------------------------------

namespace dlg {

bool setText(HWND hDlg, UINT idCtrl, LPCWSTR text)
{
    bool status = false;
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        status = ::SetWindowTextW(hCtrl, text) ? true : false;
        redrawControl(hDlg, hCtrl);
    }
    return status;
}

bool getText(HWND hDlg, UINT idCtrl, LPWSTR buf, INT bufLen)
{
    HWND hEdit = ::GetDlgItem(hDlg, idCtrl);
    if (hEdit) {
        buf[0] = 0;
        ::GetWindowTextW(hEdit, buf, bufLen);
        return true;
    }
    return false;
}

bool edtModified(HWND hDlg, UINT idCtrl)
{
    bool modified = false;
    HWND hEdit = ::GetDlgItem(hDlg, idCtrl);
    if (hEdit) {
        if (::SendMessage(hEdit, EM_GETMODIFY, 0, 0)) {
            modified = true;
        }
    }
    return modified;
}

bool setCheck(HWND hDlg, UINT idCtrl, bool bChecked)
{
    HWND hCheckBox = ::GetDlgItem(hDlg, idCtrl);
    if (hCheckBox) {
        ::SendMessage(hCheckBox, BM_SETCHECK, (WPARAM) (bChecked ? BST_CHECKED : BST_UNCHECKED), 0);
        return true;
    }
    return false;
}

bool getCheck(HWND hDlg, UINT idCtrl)
{
    HWND hCheckBox = ::GetDlgItem(hDlg, idCtrl);
    if (hCheckBox) {
        return (::SendMessage(hCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED) ? true : false;
    }
    return false;
}

bool focus(HWND hDlg, UINT idCtrl)
{
    HWND h = ::GetDlgItem(hDlg, idCtrl);
    if (h) {
        // which is correct???
        ::SetFocus(h);
        //::SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)h, TRUE);
        //::PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)h, TRUE);
        return true;
    }
    return false;
}

INT getLbSelData(HWND hDlg, UINT idCtrl)
{
    INT i = SI_NONE;
    HWND hLst;
    hLst = ::GetDlgItem(hDlg, idCtrl);
    if (hLst) {
        i = (INT)::SendMessage(hLst, LB_GETCURSEL, 0, 0);
        i = (INT)::SendMessage(hLst, LB_GETITEMDATA, i, 0);
    }
    return i;
}

INT getLbIdxByData(HWND hDlg, UINT idCtrl, INT data)
{
    HWND hLst;
    INT count, i, d, idx = -1;
    hLst = ::GetDlgItem(hDlg, idCtrl);
    if (hLst) {
        count = (INT)::SendMessage(hLst, LB_GETCOUNT, 0, 0);
        if (count != LB_ERR) {
            for (i = 0; i < count; ++i) {
                d = (INT)::SendMessage(hLst, LB_GETITEMDATA, i, 0);
                if (d == data) {
                    idx = i;
                    break;
                }
            }
        }
    }
    return idx;
}

// http://stackoverflow.com/questions/1823883/updating-text-in-a-c-win32-api-static-control-drawn-with-ws-ex-transparent
void redrawControl(HWND hDlg, HWND hCtrl)
{
    RECT r;
    ::GetClientRect(hCtrl, &r);
    ::InvalidateRect(hCtrl, &r, TRUE);
    ::MapWindowPoints(hCtrl, hDlg, (POINT *)&r, 2);
    ::RedrawWindow(hDlg, &r, NULL, RDW_ERASE | RDW_INVALIDATE);
}

/** Centers window hWnd relative to window hParentWnd with the given sizes and offsets. */
bool centerWnd(HWND hWnd, HWND hParentWnd, INT xOffset, INT yOffset, INT width, INT height, bool bRepaint)
{
    RECT rect, rectParent;
    INT  x, y;
    if (hParentWnd == NULL) {
        hParentWnd = ::GetParent(hWnd);
    }
    ::GetWindowRect(hParentWnd, &rectParent);
    ::GetWindowRect(hWnd, &rect);
    width = width > 0 ? width : rect.right - rect.left;
    height = height > 0 ? height : rect.bottom - rect.top;
    x = ((rectParent.right - rectParent.left) - width) / 2;
    x += rectParent.left + xOffset;
    y = ((rectParent.bottom - rectParent.top) - height) / 2;
    y += rectParent.top + yOffset;
    return ::MoveWindow(hWnd, x, y, width, height, bRepaint) ? true : false;
}

/** Sets the control's position and/or size relative to the right and bottom edges of the dialog.
    @param toChange  X=1, Y=2, W=4, H=8
    @param duoRight  offset from dialog right edge in dialog units
    @param duoBottom offset from dialog bottom edge in dialog units
    @param redraw    if true, the control will be redrawn, sometimes needed for the last row of controls on a dialog
*/
void adjToEdge(HWND hDlg, INT idCtrl, INT dlgW, INT dlgH, INT toChange, INT duoRight, INT duoBottom, bool redraw)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        RECT ro = {0, 0, duoRight, duoBottom};
        ::MapDialogRect(hDlg, &ro);
        RECT rc;
        POINT p;
        ::GetWindowRect(hCtrl, &rc);

        p.x = rc.left;
        p.y = rc.top;
        ::ScreenToClient(hDlg, &p);
        rc.left = p.x;
        rc.top = p.y;

        p.x = rc.right;
        p.y = rc.bottom;
        ::ScreenToClient(hDlg, &p);
        rc.right = p.x;
        rc.bottom = p.y;

        INT x = rc.left;
        INT y = rc.top;
        INT w = rc.right - rc.left;
        INT h = rc.bottom - rc.top;

        // change x with offset from client right
        if (toChange & 1) {
            if (duoRight) {
                x = dlgW - ro.right;
            }
        }
        // or change width with offset from client right
        else if (toChange & 4) {
            if (duoRight) {
                w = dlgW - ro.right - x;
            }
        }
        // change y with offset from client bottom
        if (toChange & 2) {
            if (duoBottom) {
                y = dlgH - ro.bottom;
            }
        }
        // or change height with offset from client bottom
        else if (toChange & 8) {
            if (duoBottom) {
                h = dlgH - ro.bottom - y;
            }
        }

        ::MoveWindow(hCtrl, x, y, w, h, true);
        if (redraw) {
            redrawControl(hDlg, hCtrl);
        }
    }
}

/*
bool setSbText(HWND hDlg, UINT idCtrl, INT nPart, LPCWSTR text)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        return ::SendMessage(hCtrl, SB_SETTEXT, (SBT_NOBORDERS << 8) | nPart, (LPARAM)text) ? true : false;
    }
    return false;
}

bool setSbtText(HWND hDlg, UINT idCtrl, INT nPart, LPCWSTR text)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        return ::SendMessage(hCtrl, SB_SETTIPTEXT, nPart, (LPARAM)text) ? true : false;
    }
    return false;
}
*/

// Description: 
//   Creates a status bar and divides it into the specified number of parts.
// Parameters:
//   hwndParent - parent window for the status bar.
//   idStatus - child window identifier of the status bar.
//   hinst - handle to the application instance.
//   cParts - number of parts into which to divide the status bar.
// Returns:
//   The handle to the status bar.
// Copied from: http://msdn.microsoft.com/en-us/library/windows/desktop/hh298378(v=vs.85).aspx
/*
HWND createStatusBar(HWND hwndParent, int idStatus, HINSTANCE hinst, int cParts)
{
    HWND hwndStatus;
    RECT rcClient;
    HLOCAL hloc;
    PINT paParts;
    int i, nWidth;

    // Ensure that the common control DLL is loaded.
    //InitCommonControls();

    // Create the status bar.
    hwndStatus = ::CreateWindowEx(
        0,                       // no extended styles
        STATUSCLASSNAME,         // name of status bar class
        (PCTSTR) NULL,           // no text when first created
        SBARS_SIZEGRIP |         // includes a sizing grip
        WS_CHILD | WS_VISIBLE |  // creates a visible child window
        SBT_TOOLTIPS,
        0, 0, 0, 0,              // ignores size and position
        hwndParent,              // handle to parent window
        (HMENU) idStatus,       // child window identifier
        hinst,                   // handle to application instance
        NULL);                   // no window creation data

    // Get the coordinates of the parent window's client area.
    ::GetClientRect(hwndParent, &rcClient);

    // Allocate an array for holding the right edge coordinates.
    hloc = ::LocalAlloc(LHND, sizeof(int) * cParts);
    paParts = (PINT)::LocalLock(hloc);

    // Calculate the right edge coordinate for each part, and
    // copy the coordinates to the array.
    nWidth = rcClient.right / cParts;
    int rightEdge = nWidth;
    for (i = 0; i < cParts; i++) { 
       paParts[i] = rightEdge;
       rightEdge += nWidth;
    }

    // Tell the status bar to create the window parts.
    ::SendMessage(hwndStatus, SB_SETPARTS, (WPARAM) cParts, (LPARAM)paParts);

    // Free the array, and return.
    ::LocalUnlock(hloc);
    ::LocalFree(hloc);
    return hwndStatus;
}
*/

// Description:
//   Creates a tooltip for an item in a dialog box.
// Parameters:
//   idCtrl - identifier of an dialog box item.
//   hDlg   - window handle of the dialog box.
//   text   - string to use as the tooltip text.
// Returns:
//   The handle to the tooltip.
// Copied from: http://msdn.microsoft.com/en-us/library/hh298368(v=vs.85).aspx
/*
HWND createTooltip(int idCtrl, HWND hDlg, PTSTR text)
{
    if (!idCtrl || !hDlg || !text) {
        return (HWND)NULL;
    }
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    HWND hTip = ::CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hDlg, NULL, sys_getDllHandle(), NULL);
    if (!hCtrl || !hTip) {
        return (HWND)NULL;
    }
    TOOLINFO ti = { 0 };
    ti.cbSize = sizeof(ti);
    ti.hwnd = hDlg;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.uId = (UINT_PTR)hCtrl;
    ti.lpszText = text;
    ::SendMessage(hTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    return hTip;
}
*/

} // end namespace dlg

} // end namespace NppPlugin

