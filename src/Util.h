/*
    Util.h
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

#ifndef NPP_PLUGIN_UTIL_H
#define NPP_PLUGIN_UTIL_H

//------------------------------------------------------------------------------

namespace NppPlugin {

#define EMPTY_STR _T("")
#define SPACE_STR _T(" ")
// msgBox title/options
#define M_DBG  PLUGIN_FULL_NAME SPACE_STR _T("Debug"), (MB_OK | MB_ICONWARNING)
#define M_ERR  PLUGIN_FULL_NAME SPACE_STR _T("Error"), (MB_OK | MB_ICONERROR)
#define M_WARN PLUGIN_FULL_NAME SPACE_STR _T("Warning"), (MB_OK | MB_ICONWARNING)
#define M_INFO PLUGIN_FULL_NAME, (MB_OK | MB_ICONINFORMATION)

INT msgBox(const TCHAR *m, TCHAR *title = NULL, UINT options = MB_OK);
void errBox(TCHAR *lpszFunction, DWORD errorCode);
void createIfNotPresent(TCHAR *filename, const char *contents);
inline const TCHAR* boolToStr(const bool b) { return b ? _T("true") : _T("false"); }
inline const bool uintToBool(UINT n) { return n == 0 ? false : true; }

//------------------------------------------------------------------------------

namespace pth {

TCHAR* remExt(TCHAR *p);
TCHAR* remPath(TCHAR *p);
TCHAR* remName(TCHAR *p);
bool addSlash(TCHAR *p);

} // end namespace pth

//------------------------------------------------------------------------------

namespace dlg {

bool centerWnd(HWND hWnd, HWND hParentWnd, INT xOffset = 0, INT yOffset = 0, bool bRepaint = FALSE);
bool setText(HWND hDlg, UINT idDlgCtrl, const TCHAR* pszText);
bool getText(HWND hDlg, UINT idDlgCtrl, TCHAR *buf);
bool edtModified(HWND hDlg, UINT idDlgCtrl);
bool setCheck(HWND hDlg, UINT idDlgCtrl, bool bChecked);
bool getCheck(HWND hDlg, UINT idDlgCtrl);
bool focus(HWND hDlg, UINT idDlgCtrl);
INT getLbSelData(HWND hDlg, UINT idDlgCtrl);
void adjToEdge(HWND hDlg, INT idCtrl, INT dlgW, INT dlgH, INT xRight, INT yBottom, INT wRight, INT hBottom);

} // end namespace dlg

} // end namespace NppPlugin

#endif // NPP_PLUGIN_UTIL_H
