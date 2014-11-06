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
    @file      Util.h
    @copyright Copyright 2011-2014 Michael Foster <http://mfoster.com/npp/>
*/

#ifndef NPP_PLUGIN_UTIL_H
#define NPP_PLUGIN_UTIL_H

//------------------------------------------------------------------------------

namespace NppPlugin {

/* Debug log levels:
  0     = no logging
  >=1   = LOG and LOGF
  1-9   = LOGE
  10    = LOGG
  11-19 = LOGE
  20    = LOGG
  21-29 = LOGE
  30    = LOGG
  31-39 = LOGE
*/
#define LOG(fmt, ...) msg::log(__FUNCTION__ ": " fmt, __VA_ARGS__)
#define LOGF(fmt, ...) msg::log(__FUNCTION__ "(" fmt ")", __VA_ARGS__)
#define LOGG(lvl, fmt, ...) if (gCfg.debug >= lvl) { LOG(fmt, __VA_ARGS__); }
#define LOGE(lvl, fmt, ...) if (gCfg.debug == lvl) { LOG(fmt, __VA_ARGS__); }
#define LOGNN(ntf) LOG("%-20s\t%8i\t%i", ntf, bufferId, _bidBufferActivated)
#define LOGSN(ntf) LOG("%-20s\t%8s\t%i", ntf, "", _bidBufferActivated)
#define __W(x) L ## x
#define _W(x) __W(x)
#define EMPTY_STR L""
#define SPACE_STR L" "
// msg::show title and options
#define M_DBG  PLUGIN_FULL_NAME SPACE_STR L"Debug", (MB_OK | MB_ICONWARNING)
#define M_ERR  PLUGIN_FULL_NAME SPACE_STR L"Error", (MB_OK | MB_ICONERROR)
#define M_WARN PLUGIN_FULL_NAME SPACE_STR L"Warning", (MB_OK | MB_ICONWARNING)
#define M_INFO PLUGIN_FULL_NAME, (MB_OK | MB_ICONINFORMATION)

inline LPCWSTR boolToStr(const bool b) { return b ? L"true" : L"false"; }
inline const bool uintToBool(UINT n) { return n == 0 ? false : true; }

//------------------------------------------------------------------------------
/** @namespace NppPlugin.msg Contains functions for displaying error and
    informational messages to the user and for logging to the debug log file. */

namespace msg {

INT show(LPCWSTR msg, LPWSTR title = NULL, UINT options = MB_OK);
void error(DWORD lastError, LPCWSTR format, ...);
void log(const char *format, ...);

} // end namespace msg

//------------------------------------------------------------------------------
/** @namespace NppPlugin.pth Contains functions for manipulating file paths and
    creating new files. */

namespace pth {

errno_t removeExt(LPWSTR buf, size_t bufLen);
errno_t removeName(LPWSTR buf, size_t bufLen);
errno_t removePath(LPWSTR buf, size_t bufLen);
void appendSlash(LPWSTR buf, size_t bufLen);
bool dirExists(LPCWSTR path);
bool fileExists(LPCWSTR pathname);
void createFileIfMissing(LPCWSTR pathname, const char *contents);

} // end namespace pth

//------------------------------------------------------------------------------
/// @namespace NppPlugin.dlg Contains functions for managing dialog controls.

namespace dlg {

bool setText(HWND hDlg, UINT idCtrl, LPCWSTR text);
bool getText(HWND hDlg, UINT idCtrl, LPWSTR buf, INT bufLen);
bool edtModified(HWND hDlg, UINT idCtrl);
bool setCheck(HWND hDlg, UINT idCtrl, bool bChecked);
bool getCheck(HWND hDlg, UINT idCtrl);
bool focus(HWND hDlg, UINT idCtrl);
INT getLbSelData(HWND hDlg, UINT idCtrl);
INT getLbIdxByData(HWND hDlg, UINT idCtrl, INT data);
void redrawControl(HWND hDlg, HWND hCtrl);
bool centerWnd(HWND hWnd, HWND hParentWnd, INT xOffset = 0, INT yOffset = 0, INT width = 0, INT height = 0, bool bRepaint = FALSE);
void adjToEdge(HWND hDlg, INT idCtrl, INT dlgW, INT dlgH, INT toChange, INT duoRight, INT duoBottom, bool redraw = false);
//bool setSbText(HWND hDlg, UINT idCtrl, INT nPart, LPCWSTR text);
//bool setSbtText(HWND hDlg, UINT idCtrl, INT nPart, LPCWSTR text);
//HWND createStatusBar(HWND hwndParent, int idStatus, HINSTANCE hinst, int cParts);
//HWND createTooltip(int toolID, HWND hDlg, PTSTR text);

} // end namespace dlg

} // end namespace NppPlugin

#endif // NPP_PLUGIN_UTIL_H
