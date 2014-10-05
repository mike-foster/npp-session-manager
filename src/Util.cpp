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

//------------------------------------------------------------------------------

namespace NppPlugin {

// For title/options see the M_* constants.
INT msgBox(const TCHAR *msg, TCHAR *title, UINT options)
{
    return MessageBox(sys_getNppHwnd(), msg, title != NULL ? title : PLUGIN_FULL_NAME, options);
}

// errorCode from GetLastError
void errBox(TCHAR *lpszFunction, DWORD errorCode)
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));

    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        _T("%s failed with error %d: %s"),
        lpszFunction, errorCode, lpMsgBuf);

    msgBox((TCHAR*)lpDisplayBuf, M_ERR);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

void dbgLog(const char* format, ...)
{
    if (gCfg.debug && gCfg.logFile[0]) {
        FILE *fp;
        fopen_s(&fp, gCfg.logFile, "a+");
        if (fp) {
            va_list argptr;
            va_start(argptr, format);
            vfprintf(fp, format, argptr);
            va_end(argptr);
            fputc('\n', fp);
            fflush(fp);
            fclose(fp);
        }
    }
}

void createIfNotPresent(TCHAR *filename, const char *contents)
{
    BOOL suc;
    HANDLE hFile;
    DWORD len, bytes;

    hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        len = strlen(contents);
        suc = WriteFile(hFile, contents, len, &bytes, NULL);
        if (!suc || bytes != len) {
            TCHAR msg[MAX_PATH_P1];
            StringCchCopy(msg, MAX_PATH, _T("Failed creating file: "));
            StringCchCat(msg, MAX_PATH, filename);
            msgBox(msg, M_ERR);
        }
        CloseHandle(hFile);
    }
}

//------------------------------------------------------------------------------

namespace pth {

/* Removes the file name extension. */
TCHAR* remExt(TCHAR *p)
{
    size_t len;
    if (StringCchLength(p, MAX_PATH, &len) == S_OK) {
        while (len-- > 0) {
            if (*(p + len) == _T('.')) {
                *(p + len) = 0;
                break;
            }
        }
    }
    return p;
}

/* Removes the path, leaving only the file name. */
TCHAR* remPath(TCHAR *p)
{
    size_t len;
    TCHAR s[MAX_PATH_P1];
    if (StringCchLength(p, MAX_PATH, &len) == S_OK) {
        while (len-- > 0) {
            if (*(p + len) == _T('\\') || *(p + len) == _T('/')) {
                StringCchCopy(s, MAX_PATH, p + len + 1);
                StringCchCopy(p, MAX_PATH, s);
                break;
            }
        }
    }
    return p;
}

/* Removes the file name, leaving only the path and trailing slash. */
TCHAR* remName(TCHAR *p)
{
    size_t len;
    if (StringCchLength(p, MAX_PATH, &len) == S_OK) {
        while (len-- > 0) {
            if (*(p + len) == _T('\\') || *(p + len) == _T('/')) {
                *(p + len + 1) = 0;
                break;
            }
        }
    }
    return p;
}

/* Append a backslash if it is not already present on the end of the string. */
bool addSlash(TCHAR *p)
{
    size_t len;
    bool added = false;
    if (StringCchLength(p, MAX_PATH, &len) == S_OK) {
        TCHAR *s = p + len - 1;
        if (*s != _T('\\') && *s != _T('/')) {
            StringCchCat(p, MAX_PATH, _T("\\"));
            added = true;
        }
    }
    return added;
}

bool dirExists(TCHAR *p)
{
  DWORD a = GetFileAttributes(p);
  return (bool)(a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY));
}

} // end namespace pth

//------------------------------------------------------------------------------

namespace dlg {

bool setText(HWND hDlg, UINT idDlgCtrl, const TCHAR* pszText)
{
    HWND hDlgItem = GetDlgItem(hDlg, idDlgCtrl);
    if (hDlgItem) {
        return SetWindowText(hDlgItem, pszText) ? true : false;
    }
    return false;
}

bool getText(HWND hDlg, UINT idDlgCtrl, TCHAR *buf)
{
    HWND hEdit = GetDlgItem(hDlg, idDlgCtrl);
    if (hEdit) {
        //buf[0] = SES_MAX_LEN;
        //SendMessage(hEdit, EM_GETLINE, 0, (LPARAM)buf);
        buf[0] = 0;
        GetWindowText(hEdit, (LPTSTR)buf, SES_MAX_LEN);
        return true;
    }
    return false;
}

bool edtModified(HWND hDlg, UINT idDlgCtrl)
{
    bool modified = false;
    HWND hEdit = GetDlgItem(hDlg, idDlgCtrl);
    if (hEdit) {
        if (SendMessage(hEdit, EM_GETMODIFY, 0, 0)) {
            modified = true;
        }
    }
    return modified;
}

bool setCheck(HWND hDlg, UINT idDlgCtrl, bool bChecked)
{
    HWND hCheckBox = GetDlgItem(hDlg, idDlgCtrl);
    if (hCheckBox) {
        SendMessage(hCheckBox, BM_SETCHECK, (WPARAM) (bChecked ? BST_CHECKED : BST_UNCHECKED), 0);
        return true;
    }
    return false;
}

bool getCheck(HWND hDlg, UINT idDlgCtrl)
{
    HWND hCheckBox = GetDlgItem(hDlg, idDlgCtrl);
    if (hCheckBox) {
        return (SendMessage(hCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED) ? true : false;
    }
    return false;
}

bool focus(HWND hDlg, UINT idDlgCtrl)
{
    HWND h = GetDlgItem(hDlg, idDlgCtrl);
    if (h) {
        // which is correct???
        SetFocus(h);
        //SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)h, TRUE);
        //PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)h, TRUE);
        return true;
    }
    return false;
}

INT getLbSelData(HWND hDlg, UINT idDlgCtrl)
{
    INT i = SES_NONE;
    HWND hLst;
    hLst = GetDlgItem(hDlg, idDlgCtrl);
    if (hLst) {
        i = (INT)SendMessage(hLst, LB_GETCURSEL, 0, 0);
        i = (INT)SendMessage(hLst, LB_GETITEMDATA, i, 0);
    }
    return i;
}

INT getLbIdxByData(HWND hDlg, UINT idDlgCtrl, INT data)
{
    HWND hLst;
    INT count, i, d, idx = -1;
    hLst = GetDlgItem(hDlg, idDlgCtrl);
    if (hLst) {
        count = (INT)SendMessage(hLst, LB_GETCOUNT, 0, 0);
        if (count != LB_ERR) {
            for (i = 0; i < count; ++i) {
                d = (INT)SendMessage(hLst, LB_GETITEMDATA, i, 0);
                if (d == data) {
                    idx = i;
                    break;
                }
            }
        }
    }
    return idx;
}

/* Centers window hWnd relative to window hParentWnd with the given sizes
   and offsets. */
bool centerWnd(HWND hWnd, HWND hParentWnd, INT xOffset, INT yOffset, INT width, INT height, bool bRepaint)
{
    RECT rect, rectParent;
    INT  x, y;
    if (hParentWnd == NULL) {
        hParentWnd = GetParent(hWnd);
    }
    GetWindowRect(hParentWnd, &rectParent);
    GetWindowRect(hWnd, &rect);
    width = width > 0 ? width : rect.right - rect.left;
    height = height > 0 ? height : rect.bottom - rect.top;
    x = ((rectParent.right - rectParent.left) - width) / 2;
    x += rectParent.left + xOffset;
    y = ((rectParent.bottom - rectParent.top) - height) / 2;
    y += rectParent.top + yOffset;
    return MoveWindow(hWnd, x, y, width, height, bRepaint) ? true : false;
}

/* Sets the control's position and size relative to the right and bottom edges
   of the dialog. */
void adjToEdge(HWND hDlg, INT idCtrl, INT dlgW, INT dlgH, INT xRight, INT yBottom, INT wRight, INT hBottom)
{
    HWND hCtrl = GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        RECT r;
        POINT p;
        GetWindowRect(hCtrl, &r);
        p.x = r.left;
        p.y = r.top;
        if (ScreenToClient(hDlg, &p)) {
            INT w = wRight < 0 ? r.right - r.left : dlgW - p.x - wRight;
            INT h = hBottom < 0 ? r.bottom - r.top : dlgH - p.y - hBottom;
            INT x = xRight < 0 ? p.x : dlgW - w - xRight;
            INT y = yBottom < 0 ? p.y : dlgH - h - yBottom;
            MoveWindow(hCtrl, x, y, w, h, true);
            ShowWindow(hCtrl, SW_SHOW);
        }
    }
}

} // end namespace dlg

} // end namespace NppPlugin

