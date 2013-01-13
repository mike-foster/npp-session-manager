/*
    SessionMgr.cpp
    Copyright 2011,2013 Michael Foster (http://mfoster.com/npp/)

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
#include "Menu.h"
#include "Util.h"
#include <strsafe.h>
#include <time.h>
#include <vector>

using std::vector;

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

class Session
{
    public:
    TCHAR name[SES_MAX_LEN];
    Session(TCHAR *sn)
    {
        StringCchCopy(name, SES_MAX_LEN - 1, sn);
    }
};

vector<Session> _sessions;
INT _sesCurIdx; // the _sessions index of the current session
INT _sesPrvIdx; // the _sessions index of the previous session
bool _appReady;
bool _sesLoading;
time_t _sesTimer;

void onNppReady();
void removeBracketedPrefix(TCHAR *s);

} // end namespace

//------------------------------------------------------------------------------
// The api namespace contains functions called only from DllMain.

namespace api {

void app_onLoad()
{
    _appReady = false;
    _sesLoading = false;
    _sesCurIdx = SES_DEFAULT;
    _sesPrvIdx = SES_NONE;
}

void app_onUnload()
{
    _appReady = false;
    _sessions.clear();
}

void app_init()
{
    app_readSesDir();
}

const TCHAR* app_getName()
{
    return mnu_getMainMenuLabel();
}

/* Handles Notepad++ notifications. */
void app_onNotify(SCNotification *pscn)
{
    if (pscn->nmhdr.hwndFrom == sys_getNppHwnd()) {
        if (gCfg.debug) {
            switch (pscn->nmhdr.code) {
                case NPPN_READY:      msgBox(_T("NPPN_READY"), M_DBG); break;
                case NPPN_SHUTDOWN:   msgBox(_T("NPPN_SHUTDOWN"), M_DBG); break;
                case NPPN_FILESAVED:  msgBox(_T("NPPN_FILESAVED"), M_DBG); break;
                case NPPN_FILEOPENED: msgBox(_T("NPPN_FILEOPENED"), M_DBG); break;
                case NPPN_FILECLOSED: msgBox(_T("NPPN_FILECLOSED"), M_DBG); break;
                case NPPN_LANGCHANGED:     msgBox(_T("NPPN_LANGCHANGED"), M_DBG); break;
                case NPPN_DOCORDERCHANGED: msgBox(_T("NPPN_DOCORDERCHANGED"), M_DBG); break; // I'm not sure what this does
                case NPPN_BUFFERACTIVATED: msgBox(_T("NPPN_BUFFERACTIVATED"), M_DBG); break; // Occurs twice if file in other view
            }
        }
        switch (pscn->nmhdr.code) {
            case NPPN_READY:
                onNppReady();
                break;
            case NPPN_SHUTDOWN:
                _appReady = false;
                break;
            case NPPN_FILESAVED:
                app_showSesInNppBars();
                // allow fall-thru
            case NPPN_FILEOPENED:
            case NPPN_LANGCHANGED:
            //case NPPN_DOCORDERCHANGED:
                if (_appReady && !_sesLoading) {
                    if (gCfg.getAutoSave()) {
                        app_saveSession(_sesCurIdx);
                    }
                }
                break;
            case NPPN_FILECLOSED:
                if (_appReady && !_sesLoading) {
                    if (gCfg.getAutoSave()) {
                        _sesTimer = time(NULL);
                    }
                }
                break;
            case NPPN_BUFFERACTIVATED:
                app_showSesInNppBars();
                break;
        }
    }
    if (_sesTimer > 0) {
        if (_appReady && !_sesLoading) {
            if (time(NULL) - _sesTimer > gCfg.getSaveDelay()) {
                _sesTimer = 0;
                app_saveSession(_sesCurIdx);
            }
        }
        else {
            _sesTimer = 0;
        }
    }
}

LRESULT app_msgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return 1;
}

} // end namespace api

//------------------------------------------------------------------------------

/* Reads all session names from the session directory. If there is a current
   and/or previous session it is made current and/or previous again if it is
   in the new list. */
void app_readSesDir()
{
    DWORD dwError=0;
    WIN32_FIND_DATA ffd;
    TCHAR sesFileSpec[MAX_PATH_1];
    TCHAR sesName[SES_MAX_LEN];
    TCHAR sesCur[SES_MAX_LEN];
    TCHAR sesPrv[SES_MAX_LEN];
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // Clear the sessions vector.
    sesCur[0] = 0;
    sesPrv[0] = 0;
    if (!_sessions.empty()) {
        // If a session is current/previous save its name.
        if (_sesCurIdx > SES_NONE) {
            StringCchCopy(sesCur, MAX_PATH, _sessions[_sesCurIdx].name);
        }
        if (_sesPrvIdx > SES_NONE) {
            StringCchCopy(sesPrv, MAX_PATH, _sessions[_sesPrvIdx].name);
        }
        _sessions.clear();
    }
    // Create the file spec.
    StringCchCopy(sesFileSpec, MAX_PATH, gCfg.getSesDir());
    StringCchCat(sesFileSpec, MAX_PATH, _T("*"));
    StringCchCat(sesFileSpec, MAX_PATH, gCfg.getSesExt());
    // Loop over files in the session directory, save each in the vector.
    hFind = FindFirstFile(sesFileSpec, &ffd);
    if (INVALID_HANDLE_VALUE == hFind) {
        _sesCurIdx = SES_DEFAULT;
        return;
    }
    _appReady = false;
    do {
        StringCchCopy(sesName, SES_MAX_LEN - 1, ffd.cFileName);
        pth::remExt(sesName);
        Session ses(sesName);
        _sessions.push_back(ses);
    }
    while (FindNextFile(hFind, &ffd) != 0);
    dwError = GetLastError();
    FindClose(hFind);
    // If a session was current/previous try to make it current/previous again.
    if (sesCur[0] != 0) {
        _sesCurIdx = app_getSesIndex(sesCur);
    }
    if (sesPrv[0] != 0) {
        _sesPrvIdx = app_getSesIndex(sesPrv);
    }
    if (dwError != ERROR_NO_MORE_FILES) {
        errBox(_T("app_readSesDir"), dwError);
    }
    _appReady = true;
}

/* Loads the session at index si. Makes it the current index unless lic is
   true. Closes the previous session before loading si, unless lwc is true. */
void app_loadSession(INT si)
{
    app_loadSession(si, gCfg.getLoadIntoCurrent(), gCfg.getLoadWithoutClosing());
}
void app_loadSession(INT si, bool lic, bool lwc)
{
    TCHAR sesFile[MAX_PATH_1];
    HWND hNpp = sys_getNppHwnd();

    if (!_appReady || _sesLoading) {
        return;
    }
    if (si == SES_PREVIOUS) {
        if (_sesPrvIdx <= SES_NONE) {
            return;
        }
        si = _sesPrvIdx;
    }
    if (!lic && _sesCurIdx > SES_NONE) {
        app_saveSession(_sesCurIdx); // Save the current session before closing it
        _sesPrvIdx = _sesCurIdx;
        gCfg.savePrevious(_sessions[_sesPrvIdx].name); // Write new previous session name to ini file
    }
    _sesLoading = true;
    app_getSesFile(si, sesFile);
    // Close all open files
    if (!lwc) {
        SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSEALL);
    }
    // Load session
    SendMessage(hNpp, NPPM_LOADSESSION, 0, (LPARAM)sesFile);
    _sesLoading = false;
    if (!lic) {
        if (si > SES_NONE) {
            _sesCurIdx = si;
            gCfg.saveCurrent(_sessions[si].name); // Write new current session name to ini file
            app_showSesInNppBars();
        }
        else {
            _sesCurIdx = SES_DEFAULT;
            gCfg.saveCurrent(EMPTY_STR);
        }
    }
}

/* Saves the session at index si. Makes it the current index. */
void app_saveSession(INT si)
{
    if (!_appReady || _sesLoading) {
        return;
    }
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SES_PREVIOUS) {
        si = _sesPrvIdx;
    }
    TCHAR sesFile[MAX_PATH_1];
    app_getSesFile(si, sesFile);
    SendMessage(sys_getNppHwnd(), NPPM_SAVECURRENTSESSION, 0, (LPARAM)sesFile); // Save session
    _sesCurIdx = si > SES_NONE ? si : SES_DEFAULT;
}

/* Returns true if session index si is valid, else false. */
bool app_validSesIndex(INT si)
{
    return (si >= 0 && si < (signed)_sessions.size());
}

/* Returns the number of items in the _sessions vector. */
INT app_getSesCount()
{
    return _sessions.size();
}

/* Returns the index of name in _sessions, else SES_NONE if not found.
   If name is NULL returns the current session's index. */
INT app_getSesIndex(TCHAR *name)
{
    if (name == NULL) {
        return _sesCurIdx;
    }
    INT i = 0;
    vector<Session>::iterator it;
    for (it = _sessions.begin(); it < _sessions.end(); ++it) {
        if (lstrcmp(it->name, name) == 0) {
            return i;
        }
        ++i;
    }
    return SES_NONE;
}

/* Returns a pointer to the session name at index si in _sessions. If si is
   SES_CURRENT or SES_PREVIOUS returns a pointer to the current or previous
   session's name. Else returns a pointer to the default session name. */
const TCHAR* app_getSesName(INT si)
{
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SES_PREVIOUS) {
        si = _sesPrvIdx;
    }
    return app_validSesIndex(si) ? _sessions[si].name : SES_DEFAULT_NAME;
}

/* Copies into buf the full pathname of the session at index si. If si is
   SES_CURRENT or SES_PREVIOUS copies the current or previous session's
   pathname. Else copies the default session pathname. */
void app_getSesFile(INT si, TCHAR *buf)
{
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SES_PREVIOUS) {
        si = _sesPrvIdx;
    }
    if (app_validSesIndex(si)) {
        StringCchCopy(buf, MAX_PATH, gCfg.getSesDir());
        StringCchCat(buf, MAX_PATH, _sessions[si].name);
        StringCchCat(buf, MAX_PATH, gCfg.getSesExt());
    }
    else {
        StringCchCopy(buf, MAX_PATH, sys_getDefSesFile());
    }
}

/* Displays the current and previous session names in the status bar if that
   setting is enabled. Displays the current session name in the title bar if
   that setting is enabled. */
void app_showSesInNppBars()
{
    const int maxLen1 = MAX_PATH;
    const int maxLen2 = 2 * MAX_PATH;
    TCHAR buf1[maxLen1 + 1];
    TCHAR buf2[maxLen2 + 1];

    if (gCfg.getShowInStatusbar()) {
        StringCchCopy(buf1, maxLen1, _T("session : "));
        StringCchCat(buf1, maxLen1, app_getSesName(SES_CURRENT));
        StringCchCat(buf1, maxLen1, _T("    previous : "));
        StringCchCat(buf1, maxLen1, app_getSesName(SES_PREVIOUS));
        SendMessage(sys_getNppHwnd(), NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)buf1);
    }

    if (gCfg.getShowInTitlebar()) {
        GetWindowText(sys_getNppHwnd(), buf1, maxLen1);
        removeBracketedPrefix(buf1);
        StringCchCopy(buf2, maxLen2, _T("["));
        StringCchCat(buf2, maxLen2, app_getSesName(SES_CURRENT));
        StringCchCat(buf2, maxLen2, _T("] "));
        StringCchCat(buf2, maxLen2, buf1);
        SendMessage(sys_getNppHwnd(), WM_SETTEXT, 0, (LPARAM)buf2);
    }
}

//------------------------------------------------------------------------------

namespace {

void onNppReady()
{
    TCHAR name[MAX_PATH_1];
    name[0] = 0;
    _appReady = true;
    if (gCfg.getAutoLoad()) {
        gCfg.readPrevious(name);
        _sesPrvIdx = app_getSesIndex(name);
        gCfg.readCurrent(name);
        if (name[0] != 0) {
            app_loadSession(app_getSesIndex(name));
        }
    }
}

/* We need to remove any existing prefixes before adding a new one. */
void removeBracketedPrefix(TCHAR *s)
{
    size_t i = 0, len;
    const int maxLen = 2 * MAX_PATH;
    TCHAR buf[maxLen + 1];

    if (StringCchLength(s, maxLen, &len) == S_OK) {
        while (i < len && *(s + i) == _T('[')) {
            while (i < len && *(s + i) != _T(']')) {
                ++i;
            }
            if (i < len && *(s + i) == _T(']')) {
                ++i;
            }
            while (i < len && *(s + i) == _T(' ')) {
                ++i;
            }
        }
        StringCchCopy(buf, maxLen, s + i);
        StringCchCopy(s, maxLen, buf);
    }
}

} // end namespace

} // end namespace NppPlugin
