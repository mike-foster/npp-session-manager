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
    @copyright Copyright 2011-2015 Michael Foster <http://mfoster.com/npp/>
*/

#include "System.h"
#include "SessionMgr.h"
#include "Util.h"
#include <strsafe.h>

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

/** Writes a formatted UTF-8 string to the debug log file. In format, use "%s"
    for ASCII strings and "%S" for wide strings. */
void log(LPCSTR format, ...)
{
    if (gDbgLvl) {
        LPCWSTR logFile = cfg::getStr(kDebugLogFile);
        if (logFile && *logFile) {
            FILE *fp;
            ::_wfopen_s(&fp, logFile, L"a+");
            if (fp) {
                va_list argptr;
                va_start(argptr, format);
                ::vfprintf(fp, format, argptr); // Expects 'format' to be UTF-8, vfwprintf would write UTF-16
                va_end(argptr);
                ::fputwc(L'\n', fp);
                ::fflush(fp);
                ::fclose(fp);
            }
        }
    }
}

} // end namespace NppPlugin::msg

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
void createFileIfMissing(LPCWSTR pathname, LPCSTR contents)
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

} // end namespace NppPlugin::pth

//------------------------------------------------------------------------------

namespace str {

/** Writes src to dst with ampersands removed. */
void removeAmp(LPCWSTR src, LPWSTR dst)
{
    while (*src != 0) {
        if (*src != L'&') {
            *dst++ = *src;
        }
        ++src;
    }
    *dst = 0;
}

/** Writes src to dst with ampersands removed. */
void removeAmp(LPCSTR src, LPSTR dst)
{
    while (*src != 0) {
        if (*src != '&') {
            *dst++ = *src;
        }
        ++src;
    }
    *dst = 0;
}

/** Case-insensitive wildcard match. */
bool wildcardMatchI(LPCWSTR wild, LPCWSTR str)
{
    WCHAR lcWild[MAX_PATH], lcStr[MAX_PATH];

    ::StringCchCopyW(lcWild, MAX_PATH, wild);
    ::StringCchCopyW(lcStr, MAX_PATH, str);
    ::CharLower(lcWild);
    ::CharLower(lcStr);
    return wildcardMatch(lcWild, lcStr);
}

/** Originally written by Jack Handy and slightly modified by Mike Foster.
    http://www.codeproject.com/Articles/1088/Wildcard-string-compare-globbing */
bool wildcardMatch(LPCWSTR wild, LPCWSTR str)
{
    LPCWSTR cp = NULL, mp = NULL;

    while (*str && *wild != L'*') {
        if (*wild != *str && *wild != L'?') {
            return false;
        }
        wild++;
        str++;
    }

    while (*str) {
        if (*wild == L'*') {
            if (!*++wild) {
                return true;
            }
            mp = wild;
            cp = str + 1;
        }
        else if (*wild == *str || *wild == L'?') {
            wild++;
            str++;
        }
        else {
            wild = mp;
            str = cp++;
        }
    }

    while (*wild == L'*') {
        wild++;
    }

    return !*wild;
}

/** cStr must be zero-terminated.
    @return a pointer to an allocated buffer which caller must free, else NULL
    on error */
LPWSTR utf8ToUtf16(LPCSTR cStr)
{
    return utf8ToUtf16(cStr, NULL, 0);
}

/** cStr must be zero-terminated.
    @return if buf is NULL, a pointer to an allocated buffer which caller must
    free, else buf, or NULL on error */
LPWSTR utf8ToUtf16(LPCSTR cStr, LPWSTR buf, size_t bufLen)
{
    size_t wLen;
    LPWSTR wBuf = NULL;

    wLen = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, cStr, -1, NULL, 0);
    if (wLen > 0) {
        if (buf) {
            if (bufLen >= wLen) {
                if (::MultiByteToWideChar(CP_UTF8, 0, cStr, -1, buf, bufLen)) {
                    wBuf = buf;
                }
                else {
                    LOG("Failed converting \"%s\".", cStr);
                }
            }
            else {
                LOG("Provided buffer size (%i) is smaller than required (%i) for \"%s\".", bufLen, wLen, cStr);
            }
        }
        else {
            wBuf = (LPWSTR)sys_alloc(wLen * sizeof(WCHAR));
            if (wBuf) {
                if (!::MultiByteToWideChar(CP_UTF8, 0, cStr, -1, wBuf, wLen)) {
                    sys_free(wBuf);
                    wBuf = NULL;
                    LOG("Failed converting \"%s\".", cStr);
                }
            }
            else {
                LOG("Allocation failed for \"%s\".", cStr);
            }
        }
    }
    else {
        LOG("Invalid characters in \"%s\".", cStr);
    }

    return wBuf;
}

/** wStr must be zero-terminated.
    @return a pointer to an allocated buffer which caller must free, else NULL
    on error */
LPSTR utf16ToUtf8(LPCWSTR wStr)
{
    return utf16ToUtf8(wStr, NULL, 0);
}

/** wStr must be zero-terminated.
    @return if buf is NULL, a pointer to an allocated buffer which caller must
    free, else buf, or NULL on error */
LPSTR utf16ToUtf8(LPCWSTR wStr, LPSTR buf, size_t bufLen)
{
    size_t cLen;
    LPSTR cBuf = NULL;

    cLen = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wStr, -1, NULL, 0, NULL, NULL);
    if (cLen > 0) {
        if (buf) {
            if (bufLen >= cLen) {
                if (::WideCharToMultiByte(CP_UTF8, 0, wStr, -1, buf, bufLen, NULL, NULL)) {
                    cBuf = buf;
                }
                else {
                    LOG("Failed converting \"%s\".", wStr);
                }
            }
            else {
                LOG("Provided buffer size (%i) is smaller than required (%i) for \"%s\".", bufLen, cLen, wStr);
            }
        }
        else {
            cBuf = (LPSTR)sys_alloc(cLen * sizeof(CHAR));
            if (cBuf) {
                if (!::WideCharToMultiByte(CP_UTF8, 0, wStr, -1, cBuf, cLen, NULL, NULL)) {
                    sys_free(cBuf);
                    cBuf = NULL;
                    LOG("Failed converting \"%s\".", wStr);
                }
            }
            else {
                LOG("Allocation failed for \"%s\".", wStr);
            }
        }
    }
    else {
        LOG("Invalid characters in \"%s\".", wStr);
    }

    return cBuf;
}

} // end namespace NppPlugin::str

//------------------------------------------------------------------------------

namespace dlg {

void setText(HWND hDlg, UINT idCtrl, LPCWSTR text)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        ::SetWindowTextW(hCtrl, text);
        redrawControl(hDlg, hCtrl);
    }
}

void getText(HWND hDlg, UINT idCtrl, LPWSTR buf, size_t bufLen)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        buf[0] = 0;
        ::GetWindowTextW(hCtrl, buf, bufLen);
        //::SendMessage(hCtrl, WM_GETTEXT, bufLen, (LPARAM)buf);
    }
}

bool edtModified(HWND hDlg, UINT idCtrl)
{
    bool modified = false;
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        if (::SendMessage(hCtrl, EM_GETMODIFY, 0, 0)) {
            modified = true;
        }
    }
    return modified;
}

void setCheck(HWND hDlg, UINT idCtrl, bool bChecked)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        ::SendMessage(hCtrl, BM_SETCHECK, (WPARAM) (bChecked ? BST_CHECKED : BST_UNCHECKED), 0);
    }
}

bool getCheck(HWND hDlg, UINT idCtrl)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        return (::SendMessage(hCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED) ? true : false;
    }
    return false;
}

void focus(HWND hDlg, UINT idCtrl, bool inInit)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        if (inInit) {
            ::SetFocus(hCtrl);
        }
        else {
            ::SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hCtrl, TRUE);
        }
    }
}

void lbReplaceSelItem(HWND hDlg, UINT idCtrl, LPCWSTR text, LPARAM data)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        ::SendMessage(hCtrl, WM_SETREDRAW, FALSE, 0);
        WPARAM lbItemIdx = ::SendMessage(hCtrl, LB_GETCURSEL, 0, 0);
        if (::SendMessage(hCtrl, LB_DELETESTRING, lbItemIdx, 0) != LB_ERR) {
            if (::SendMessage(hCtrl, LB_INSERTSTRING, lbItemIdx, (LPARAM)text) != LB_ERR) {
                ::SendMessage(hCtrl, LB_SETITEMDATA, lbItemIdx, (LPARAM)data);
                ::SendMessage(hCtrl, WM_SETREDRAW, TRUE, 0);
                ::SendMessage(hCtrl, LB_SETCURSEL, lbItemIdx, 0);
            }
        }
    }
}

LRESULT getLbSelData(HWND hDlg, UINT idCtrl)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        WPARAM i = ::SendMessage(hCtrl, LB_GETCURSEL, 0, 0);
        return ::SendMessage(hCtrl, LB_GETITEMDATA, i, 0);
    }
    return NULL;
}

void getCbSelText(HWND hDlg, UINT idCtrl, LPWSTR buf)
{
    HWND hCtrl = ::GetDlgItem(hDlg, idCtrl);
    if (hCtrl) {
        WPARAM i = ::SendMessage(hCtrl, CB_GETCURSEL, 0, 0);
        ::SendMessage(hCtrl, CB_GETLBTEXT, i, (LPARAM)buf);
    }
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
void centerWnd(HWND hWnd, HWND hParentWnd, INT xOffset, INT yOffset, INT width, INT height, bool bRepaint)
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
    ::MoveWindow(hWnd, x, y, width, height, bRepaint);
}

/** Sets the control's position and/or size relative to the right and bottom
    edges of the dialog.
    @param toChange  X=1, Y=2, W=4, H=8
    @param duoRight  offset from dialog right edge in dialog units
    @param duoBottom offset from dialog bottom edge in dialog units
    @param redraw    if true, the control will be redrawn, sometimes needed for
                     the last row of controls on a dialog
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

} // end namespace NppPlugin::dlg

} // end namespace NppPlugin

