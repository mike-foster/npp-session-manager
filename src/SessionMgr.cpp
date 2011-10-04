/*
    SessionMgr.cpp
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

#define PLUGIN_MENU_NAME _T("&") PLUGIN_FULL_NAME

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
bool _appReady;
bool _sesLoading;
time_t _sesTimer;

void onNppReady();

} // end namespace

//------------------------------------------------------------------------------
// The api namespace contains functions called only from DllMain.

namespace api {

void app_onLoad()
{
    _appReady = false;
    _sesLoading = false;
    _sesCurIdx = SES_DEFAULT;
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
    return PLUGIN_MENU_NAME;
}

/* Handle Notepad++ notifications. */
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
                case NPPN_DOCORDERCHANGED: msgBox(_T("NPPN_DOCORDERCHANGED"), M_DBG); break;
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
            case NPPN_FILEOPENED:
            case NPPN_LANGCHANGED:
            case NPPN_DOCORDERCHANGED:
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

/* Read all session names from the session directory. If there is a current
   session it is made current again if it is in the new list. */
void app_readSesDir()
{
    DWORD dwError=0;
    WIN32_FIND_DATA ffd;
    TCHAR sesFileSpec[MAX_PATH_1];
    TCHAR sesName[SES_MAX_LEN];
    TCHAR sesPrev[SES_MAX_LEN];
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // Clear the sessions vector.
    sesPrev[0] = 0;
    if (!_sessions.empty()) {
        // If a session is current save its name.
        if (_sesCurIdx > SES_NONE) {
            StringCchCopy(sesPrev, MAX_PATH, _sessions[_sesCurIdx].name);
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
    // If a session was current try to make it current again.
    if (sesPrev[0] != 0) {
        _sesCurIdx = app_getSesIndex(sesPrev);
    }
    if (dwError != ERROR_NO_MORE_FILES) {
        errBox(_T("app_readSesDir"), dwError);
    }
    _appReady = true;
}

/* Load the session at index si. Make it the current index unless lic is
   true. Close the previous session before loading si, unless lwc is true. */
void app_loadSession(INT si, bool lic, bool lwc)
{
    TCHAR sesFile[MAX_PATH_1];
    HWND hNpp = sys_getNppHwnd();

    if (!_appReady || _sesLoading) {
        return;
    }
    if (!lic && _sesCurIdx > SES_NONE) {
        app_saveSession(_sesCurIdx); // Save the current session before closing it
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
            gCfg.saveCurrent(_sessions[si].name); // Write new session name to ini file
        }
        else {
            _sesCurIdx = SES_DEFAULT;
            gCfg.saveCurrent(EMPTY_STR);
        }
    }
}

/* Save the session at index si. Make it the current index. */
void app_saveSession(INT si)
{
    if (!_appReady || _sesLoading) {
        return;
    }
    TCHAR sesFile[MAX_PATH_1];
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    app_getSesFile(si, sesFile);
    SendMessage(sys_getNppHwnd(), NPPM_SAVECURRENTSESSION, 0, (LPARAM)sesFile); // Save session
    _sesCurIdx = si > SES_NONE ? si : SES_DEFAULT;
}

bool app_validSesIndex(INT si)
{
    return (si >= 0 && si < (signed)_sessions.size());
}

INT app_getSesCount()
{
    return _sessions.size();
}

/* Return the index of name in _sessions, else SES_NONE if not found. If name is
   NULL return the current session's index. */
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

/* Return a pointer to the session name at index si in _sessions. If si is
   SES_CURRENT return a pointer to the current session's name. Else return a
   pointer to the default session name. */
const TCHAR* app_getSesName(INT si)
{
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    return app_validSesIndex(si) ? _sessions[si].name : SES_DEFAULT_NAME;
}

/* Copy into buf the full pathname of the session at index si. If si is
   SES_CURRENT copy the current session's pathname. Else copy the default
   session pathname. */
void app_getSesFile(INT si, TCHAR *buf)
{
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
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

//------------------------------------------------------------------------------

namespace {

void onNppReady()
{
    TCHAR name[MAX_PATH_1];
    name[0] = 0;
    _appReady = true;
    if (gCfg.getAutoLoad()) {
        gCfg.readCurrent(name);
        if (name[0] != 0) {
            app_loadSession(app_getSesIndex(name));
        }
    }
}

} // end namespace

} // end namespace NppPlugin
