/// @file
/*
    Util.cpp
    Copyright 2011-2014 Michael Foster (http://mfoster.com/npp/)

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
#include "Util.h"
#include <strsafe.h>
//#include <commctrl.h> // XXX experimental for statusbar and tooltips

//------------------------------------------------------------------------------

namespace NppPlugin {

/** Displays a simple message box. For title/options see the M_* constants. */
INT msgBox(const TCHAR *msg, TCHAR *title, UINT options)
{
    return ::MessageBox(sys_getNppHandle(), msg, title != NULL ? title : PLUGIN_FULL_NAME, options);
}

/** Displays an error box. If errorCode is 0 GetLastError is called for it. */
void errBox(TCHAR *lpszFunction, DWORD errorCode)
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    if (errorCode == 0) {
        errorCode = ::GetLastError();
    }

    ::FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)::LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));

    ::StringCchPrintf((LPTSTR)lpDisplayBuf,
        ::LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        _T("%s failed with error %d: %s"),
        lpszFunction, errorCode, lpMsgBuf);

    msgBox((TCHAR*)lpDisplayBuf, M_ERR);

    ::LocalFree(lpMsgBuf);
    ::LocalFree(lpDisplayBuf);
}

/** Writes a formatted string to the debug log file. */
void dbgLog(const char* format, ...)
{
    if (gCfg.debug && gCfg.logFile[0]) {
        FILE *fp;
        ::fopen_s(&fp, gCfg.logFile, "a+");
        if (fp) {
            va_list argptr;
            va_start(argptr, format);
            ::vfprintf(fp, format, argptr);
            va_end(argptr);
            ::fputc('\n', fp);
            ::fflush(fp);
            ::fclose(fp);
        }
    }
}

void createIfNotPresent(TCHAR *filename, const char *contents)
{
    BOOL suc;
    HANDLE hFile;
    DWORD len, bytes;

    hFile = ::CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        len = ::strlen(contents);
        suc = ::WriteFile(hFile, contents, len, &bytes, NULL);
        if (!suc || bytes != len) {
            TCHAR msg[MAX_PATH_P1];
            ::StringCchCopy(msg, MAX_PATH, _T("Failed creating file: "));
            ::StringCchCat(msg, MAX_PATH, filename);
            msgBox(msg, M_ERR);
        }
        ::CloseHandle(hFile);
    }
}

//------------------------------------------------------------------------------
/// @namespace NppPlugin.pth Contains functions for manipulating paths.

namespace pth {

/** Removes the file name extension. */
TCHAR* remExt(TCHAR *p)
{
    size_t len;
    if (::StringCchLength(p, MAX_PATH, &len) == S_OK) {
        while (len-- > 0) {
            if (*(p + len) == _T('.')) {
                *(p + len) = 0;
                break;
            }
        }
    }
    return p;
}

/** Removes the path, leaving only the file name. */
TCHAR* remPath(TCHAR *p)
{
    size_t len;
    TCHAR s[MAX_PATH_P1];
    if (::StringCchLength(p, MAX_PATH, &len) == S_OK) {
        while (len-- > 0) {
            if (*(p + len) == _T('\\') || *(p + len) == _T('/')) {
                ::StringCchCopy(s, MAX_PATH, p + len + 1);
                ::StringCchCopy(p, MAX_PATH, s);
                break;
            }
        }
    }
    return p;
}

/** Removes the file name, leaving only the path and trailing slash. */
TCHAR* remName(TCHAR *p)
{
    size_t len;
    if (::StringCchLength(p, MAX_PATH, &len) == S_OK) {
        while (len-- > 0) {
            if (*(p + len) == _T('\\') || *(p + len) == _T('/')) {
                *(p + len + 1) = 0;
                break;
            }
        }
    }
    return p;
}

/** Append a backslash if it is not already present on the end of the string. */
bool addSlash(TCHAR *p)
{
    size_t len;
    bool added = false;
    if (::StringCchLength(p, MAX_PATH, &len) == S_OK) {
        TCHAR *s = p + len - 1;
        if (*s != _T('\\') && *s != _T('/')) {
            ::StringCchCat(p, MAX_PATH, _T("\\"));
            added = true;
        }
    }
    return added;
}

bool dirExists(TCHAR *p)
{
  DWORD a = ::GetFileAttributes(p);
  return (bool)(a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY));
}

} // end namespace pth

//------------------------------------------------------------------------------
/// @namespace NppPlugin.dlg Contains functions for managing dialog controls.

namespace dlg {

bool setText(HWND hDlg, UINT idCtrl, const TCHAR* text)
{
    bool status = false;
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        status = ::SetWindowText(hCtrl, text) ? true : false;
        redrawControl(hDlg, hCtrl);
    }
    return status;
}

bool getText(HWND hDlg, UINT idCtrl, TCHAR *buf)
{
    HWND hEdit = ::GetDlgItem(hDlg, idCtrl);
    if (hEdit) {
        buf[0] = 0;
        ::GetWindowText(hEdit, (LPTSTR)buf, SES_NAME_MAX_LEN);
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
        SetFocus(h);
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
        hParentWnd = GetParent(hWnd);
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
    @param last      if true, the control will be redrawn, sometimes needed for the last row of controls on a dialog
*/
void adjToEdge(HWND hDlg, INT idCtrl, INT dlgW, INT dlgH, INT toChange, INT duoRight, INT duoBottom, bool last)
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
        if (last) {
            redrawControl(hDlg, hCtrl);
        }
    }
}

/*
bool setSbText(HWND hDlg, UINT idCtrl, INT nPart, const TCHAR* text)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        return ::SendMessage(hCtrl, SB_SETTEXT, (SBT_NOBORDERS << 8) | nPart, (LPARAM)text) ? true : false;
    }
    return false;
}

bool setSbtText(HWND hDlg, UINT idCtrl, INT nPart, const TCHAR* text)
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

