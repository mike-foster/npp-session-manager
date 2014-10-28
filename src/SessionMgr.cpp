/// @file
/*
    SessionMgr.cpp
    Copyright 2011,2013,2014 Michael Foster (http://mfoster.com/npp/)

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
#include "Properties.h"
#include <algorithm>
#include <strsafe.h>
#include <time.h>
#include <vector>

using std::vector;

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

#define NPP_BOOKMARK_MARGIN_ID 1

/// @class Session
class Session
{
    public:
    TCHAR name[SES_NAME_MAX_LEN];
    FILETIME modifiedTime;
    Session(TCHAR *sn, FILETIME mod)
    {
        ::StringCchCopy(name, SES_NAME_MAX_LEN - 1, sn);
        modifiedTime.dwLowDateTime = mod.dwLowDateTime;
        modifiedTime.dwHighDateTime = mod.dwHighDateTime;
    }
};

vector<Session> _sessions; ///< stores sessions read from disk
INT _sesCurIdx;            ///< current _sessions index
INT _sesPrvIdx;            ///< previous _sessions index
INT _bidFileOpened;        ///< bufferId from most recent NPPN_FILEOPENED
INT _bidBufferActivated;   ///< XXX experimental. bufferId from most recent NPPN_BUFFERACTIVATED
bool _sesIsDirty;          ///< XXX experimental. if true, current session needs to be saved. this should only be set true in cases where saving the session is deferred
bool _appReady;            ///< if false, plugin should do nothing
bool _sesLoading;          ///< if true, a session is loading
time_t _shutdownTimer;     ///< for determining if files are closing due to a shutdown
time_t _titlebarTimer;     ///< for updating the titlebar text

void onNppReady();
void removeBracketedPrefix(TCHAR *s);

} // end namespace

bool sortByAlpha(const Session s1, const Session s2);
bool sortByDate(const Session s1, const Session s2);

//------------------------------------------------------------------------------
/// @namespace NppPlugin.api Contains functions called only from DllMain.

namespace api {

void app_onLoad()
{
    _appReady = false;
    _sesLoading = false;
    _sesCurIdx = SI_DEFAULT;
    _sesPrvIdx = SI_NONE;
    _bidFileOpened = 0;
    _bidBufferActivated = 0;
    _sesIsDirty = false;
    _shutdownTimer = 0;
    _titlebarTimer = 0;
}

void app_onUnload()
{
    LOG("---------- STOP  %S %s", PLUGIN_FULL_NAME, RES_VERSION_S);
    _appReady = false;
    _sessions.clear();
}

void app_init()
{
    LOG("-------------- START %S %s with debug level %i", PLUGIN_FULL_NAME, RES_VERSION_S, gCfg.debug);
    app_readSessionDirectory();
}

const TCHAR* app_getName()
{
    return mnu_getMainMenuLabel();
}

/** Handles Notepad++ and Scintilla notifications. */
void app_onNotify(SCNotification *pscn)
{
    uptr_t bufferId = pscn->nmhdr.idFrom;
    unsigned int notificationCode = pscn->nmhdr.code;

    // Notepad++ notifications
    if (pscn->nmhdr.hwndFrom == sys_getNppHandle()) {
        if (gCfg.debug >= 10) {
            switch (notificationCode) {
                case NPPN_READY:           LOG("NPPN_READY"); break;
                case NPPN_SHUTDOWN:        LOG("NPPN_SHUTDOWN"); break;
                case NPPN_FILEBEFORESAVE:  LOGNN("NPPN_FILEBEFORESAVE"); break;
                case NPPN_FILESAVED:       LOGNN("NPPN_FILESAVED"); break;
                case NPPN_FILEBEFORELOAD:  LOGNN("NPPN_FILEBEFORELOAD"); break;
                case NPPN_FILELOADFAILED:  LOGNN("NPPN_FILELOADFAILED"); break;
                case NPPN_FILEBEFOREOPEN:  LOGNN("NPPN_FILEBEFOREOPEN"); break;
                case NPPN_FILEOPENED:      LOGNN("NPPN_FILEOPENED"); break;
                case NPPN_FILEBEFORECLOSE: LOGNN("NPPN_FILEBEFORECLOSE"); break;
                case NPPN_FILECLOSED:      LOGNN("NPPN_FILECLOSED"); break;
                case NPPN_LANGCHANGED:     LOGNN("NPPN_LANGCHANGED"); break;
                case NPPN_DOCORDERCHANGED: LOGNN("NPPN_DOCORDERCHANGED"); break; // Does not occur?
                case NPPN_BUFFERACTIVATED: LOGNN("NPPN_BUFFERACTIVATED"); break;
            }
        }
        switch (notificationCode) {
            case NPPN_READY:
                onNppReady();
                break;
            case NPPN_SHUTDOWN:
                _appReady = false;
                break;
            case NPPN_FILEOPENED:
                _sesIsDirty = true;
                _bidFileOpened = bufferId;
                break;
            case NPPN_FILESAVED:
                app_showSessionInNppBars();
                // intentional fall-thru
            case NPPN_LANGCHANGED:
            //case NPPN_DOCORDERCHANGED:
                if (_appReady && !_sesLoading && gCfg.autoSaveEnabled()) {
                    app_saveSession(_sesCurIdx);
                }
                break;
            //case NPPN_FILEBEFORECLOSE: // XXX experimental
            //    LOGG(10, "Shutdown %s be in progress", (bufferId != _bidBufferActivated) ? "MAY" : "may NOT");
            //    break;
            case NPPN_FILECLOSED:
                _sesIsDirty = true;
                if (_appReady && !_sesLoading && gCfg.autoSaveEnabled()) {
                    _shutdownTimer = ::time(NULL);
                    LOGG(10, "Save session in %i seconds if no shutdown", gCfg.getSaveDelay());
                }
                break;
            case NPPN_BUFFERACTIVATED:
                _bidBufferActivated = bufferId;
                if (_appReady && !_sesLoading) {
                    app_showSessionInNppBars();
                    if (_bidFileOpened == bufferId) { // buffer activated immediately after NPPN_FILEOPENED
                        if (gCfg.globalBookmarksEnabled()) {
                            prp::updateDocumentFromGlobal(_bidFileOpened);
                        }
                        if (gCfg.autoSaveEnabled()) {
                            app_saveSession(_sesCurIdx);
                        }
                    }
                }
                _bidFileOpened = 0;
                break;
        } // end switch
    }

    // Scintilla notifications
    else if (pscn->nmhdr.hwndFrom == sys_getSciHandle(1) || pscn->nmhdr.hwndFrom == sys_getSciHandle(2)) {
        if (gCfg.debug >= 10) {
            switch (notificationCode) {
                case SCN_SAVEPOINTREACHED: LOGSN("SCN_SAVEPOINTREACHED"); break;
                case SCN_SAVEPOINTLEFT:    LOGSN("SCN_SAVEPOINTLEFT"); break;
                case SCN_MARGINCLICK:      LOGSN("SCN_MARGINCLICK"); break;
            }
        }
        switch (notificationCode) {
            case SCN_SAVEPOINTLEFT:
                _sesIsDirty = true;
                if (gCfg.showInTitlebarEnabled()) {
                    _titlebarTimer = ::time(NULL);
                }
                break;
            case SCN_MARGINCLICK:
                if (_appReady && !_sesLoading && gCfg.autoSaveEnabled() && pscn->margin == NPP_BOOKMARK_MARGIN_ID) {
                    app_saveSession(_sesCurIdx);
                }
                break;
        }
    }

    // Timers
    if (_shutdownTimer > 0) {
        if (_appReady && !_sesLoading) {
            if (::time(NULL) - _shutdownTimer > gCfg.getSaveDelay()) {
                _shutdownTimer = 0;
                app_saveSession(_sesCurIdx);
            }
        }
        else {
            _shutdownTimer = 0;
        }
    }
    if (_titlebarTimer > 0) {
        if (::time(NULL) - _titlebarTimer > 1) {
            _titlebarTimer = 0;
            app_showSessionInNppBars();
        }
    }
}

LRESULT app_msgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return 1;
}

} // end namespace api

//------------------------------------------------------------------------------

/** Reads all session names from the session directory. If there is a current
    and/or previous session it is made current and/or previous again if it is
    in the new list. */
void app_readSessionDirectory()
{
    DWORD dwError=0;
    WIN32_FIND_DATA ffd;
    TCHAR sesFileSpec[MAX_PATH_P1];
    TCHAR sesName[SES_NAME_MAX_LEN];
    TCHAR sesCur[SES_NAME_MAX_LEN];
    TCHAR sesPrv[SES_NAME_MAX_LEN];
    HANDLE hFind = INVALID_HANDLE_VALUE;

    LOGF("");

    // Clear the sessions vector.
    sesCur[0] = 0;
    sesPrv[0] = 0;
    if (!_sessions.empty()) {
        // If a session is current/previous save its name.
        if (_sesCurIdx > SI_NONE) {
            ::StringCchCopy(sesCur, MAX_PATH, _sessions[_sesCurIdx].name);
        }
        if (_sesPrvIdx > SI_NONE) {
            ::StringCchCopy(sesPrv, MAX_PATH, _sessions[_sesPrvIdx].name);
        }
        _sessions.clear();
    }
    // Create the file spec.
    ::StringCchCopy(sesFileSpec, MAX_PATH, gCfg.getSesDir());
    ::StringCchCat(sesFileSpec, MAX_PATH, _T("*"));
    ::StringCchCat(sesFileSpec, MAX_PATH, gCfg.getSesExt());
    // Loop over files in the session directory, save each in the vector.
    hFind = ::FindFirstFile(sesFileSpec, &ffd);
    if (INVALID_HANDLE_VALUE == hFind) {
        _sesCurIdx = SI_DEFAULT;
        return;
    }
    _appReady = false;
    do {
        ::StringCchCopy(sesName, SES_NAME_MAX_LEN - 1, ffd.cFileName);
        pth::remExt(sesName);
        Session ses(sesName, ffd.ftLastWriteTime);
        _sessions.push_back(ses);
    }
    while (::FindNextFile(hFind, &ffd) != 0);
    dwError = ::GetLastError();
    ::FindClose(hFind);
    // Sort before restoring indexes
    if (gCfg.sortAlphaEnabled()) {
        std::sort(_sessions.begin(), _sessions.end(), sortByAlpha);
    }
    else {
        std::sort(_sessions.begin(), _sessions.end(), sortByDate);
    }
    // If a session was current/previous try to make it current/previous again.
    if (sesCur[0] != 0) {
        _sesCurIdx = app_getSessionIndex(sesCur);
    }
    if (sesPrv[0] != 0) {
        _sesPrvIdx = app_getSessionIndex(sesPrv);
    }
    if (dwError != ERROR_NO_MORE_FILES) {
        errBox(_T(__FUNCTION__), dwError);
    }
    _appReady = true;
}

/** Sorts the sessions vector ascending alphabetically. */
bool sortByAlpha(const Session s1, const Session s2)
{
    return ::lstrcmp(s1.name, s2.name) <= 0;
}

/** Sorts the sessions vector descending by the file's last modified time. For
    sessions that have the same last modified time it sorts ascending alphabetically. */
bool sortByDate(const Session s1, const Session s2)
{
    INT result = ::CompareFileTime(&s1.modifiedTime, &s2.modifiedTime);
    if (result < 0) {
        return false;
    }
    if (result > 0) {
        return true;
    }
    return sortByAlpha(s1, s2);
}

/** Loads the session at index si. Makes it the current index unless lic is
    true. Closes the previous session before loading si, unless lwc is true. */
void app_loadSession(INT si)
{
    app_loadSession(si, gCfg.loadIntoCurrentEnabled(), gCfg.loadWithoutClosingEnabled());
}
void app_loadSession(INT si, bool lic, bool lwc)
{
    TCHAR sesFile[MAX_PATH_P1];
    HWND hNpp = sys_getNppHandle();

    LOGF("%i, %i, %i", si, lic, lwc);

    if (!_appReady || _sesLoading) {
        return;
    }
    if (si == SI_PREVIOUS) {
        if (_sesPrvIdx <= SI_NONE) {
            return;
        }
        si = _sesPrvIdx;
    }

    if (!lic && _sesCurIdx > SI_NONE) {
        if (gCfg.autoSaveEnabled()) {
            app_saveSession(_sesCurIdx); // Save the current session before closing it
        }
        _sesPrvIdx = _sesCurIdx;
        gCfg.savePrevious(_sessions[_sesPrvIdx].name); // Write new previous session name to ini file
    }

    _sesLoading = true;

    app_getSessionFile(si, sesFile);
    if (gCfg.globalBookmarksEnabled()) {
        prp::updateSessionFromGlobal(sesFile);
    }

    // Close all open files
    if (!lwc) {
        LOGG(10, "Closing documents for session %i", _sesCurIdx);
        ::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSEALL);
    }
    LOGG(10, "Opening documents for session %i", si);
    // Load session
    ::SendMessage(hNpp, NPPM_LOADSESSION, 0, (LPARAM)sesFile);
    _sesLoading = false;
    if (!lic) {
        if (si > SI_NONE) {
            _sesCurIdx = si;
            gCfg.saveCurrent(_sessions[si].name); // Write new current session name to ini file
            app_showSessionInNppBars();
        }
        else {
            _sesCurIdx = SI_DEFAULT;
            gCfg.saveCurrent(EMPTY_STR);
        }
    }
}

/** Saves the session at index si. Makes it the current index. */
void app_saveSession(INT si)
{
    LOGF("%i", si);

    if (!_appReady || _sesLoading) {
        return;
    }
    LOGG(10, "Session %s dirty", _sesIsDirty ? "IS" : "is NOT"); // XXX experimental
    if (si == SI_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SI_PREVIOUS) {
        si = _sesPrvIdx;
    }
    TCHAR sesFile[MAX_PATH_P1];
    app_getSessionFile(si, sesFile);

    ::SendMessage(sys_getNppHandle(), NPPM_SAVECURRENTSESSION, 0, (LPARAM)sesFile); // Save session
    _sesCurIdx = si > SI_NONE ? si : SI_DEFAULT;
    if (gCfg.globalBookmarksEnabled()) {
        prp::updateGlobalFromSession(sesFile);
    }
    _sesIsDirty = false;
}

/** Returns true if session index si is valid, else false. */
bool app_isValidSessionIndex(INT si)
{
    return (si >= 0 && si < (signed)_sessions.size());
}

/** Returns the number of items in the _sessions vector. */
INT app_getSessionCount()
{
    return _sessions.size();
}

/** Returns the index of name in _sessions, else SI_NONE if not found.
    If name is NULL returns the current session's index. */
INT app_getSessionIndex(TCHAR *name)
{
    if (name == NULL) {
        return _sesCurIdx;
    }
    INT i = 0;
    vector<Session>::iterator it;
    for (it = _sessions.begin(); it < _sessions.end(); ++it) {
        if (::lstrcmp(it->name, name) == 0) {
            return i;
        }
        ++i;
    }
    return SI_NONE;
}

/** Returns the current _sessions index. */
INT app_getCurrentIndex()
{
    return _sesCurIdx;
}

/** Returns the previous _sessions index. */
INT app_getPreviousIndex()
{
    return _sesPrvIdx;
}

/** Sets the previous _sessions index to SI_NONE and updates the ini file and NPP bars. */
void app_resetPreviousIndex()
{
    _sesPrvIdx = SI_NONE;
    gCfg.savePrevious(SES_NAME_NONE);
    app_showSessionInNppBars();
}

/** Assigns newName to the session at index si. If si is current or previous
    updates the ini file and NPP bars then returns true. */
bool app_renameSession(INT si, TCHAR *newName)
{
    bool curOrPrv = false;
    if (app_isValidSessionIndex(si)) {
        ::StringCchCopy(_sessions[si].name, SES_NAME_MAX_LEN - 1, newName);
        if (si == _sesCurIdx) {
            curOrPrv = true;
            gCfg.saveCurrent(newName);
        }
        else if (si == _sesPrvIdx) {
            curOrPrv = true;
            gCfg.savePrevious(newName);
        }
        if (curOrPrv) {
            app_showSessionInNppBars();
        }
    }
    return curOrPrv;
}

/** Returns a pointer to the session name at index si in _sessions. If si is
    SI_CURRENT or SI_PREVIOUS returns a pointer to the current or previous
    session's name. Else returns a pointer to the 'none' session name. */
const TCHAR* app_getSessionName(INT si)
{
    if (si == SI_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SI_PREVIOUS) {
        si = _sesPrvIdx;
    }
    // TODO: was SES_NAME_DEFAULT, need to confirm this change doesn't cause a problem
    return app_isValidSessionIndex(si) ? _sessions[si].name : SES_NAME_NONE;
}

/** Copies into buf the full pathname of the session at index si. If si is
    SI_CURRENT or SI_PREVIOUS copies the current or previous session's
    pathname. Else copies the default session pathname. */
void app_getSessionFile(INT si, TCHAR *buf)
{
    if (si == SI_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SI_PREVIOUS) {
        si = _sesPrvIdx;
    }
    if (app_isValidSessionIndex(si)) {
        ::StringCchCopy(buf, MAX_PATH, gCfg.getSesDir());
        ::StringCchCat(buf, MAX_PATH, _sessions[si].name);
        ::StringCchCat(buf, MAX_PATH, gCfg.getSesExt());
    }
    else {
        ::StringCchCopy(buf, MAX_PATH, sys_getDefSesFile());
    }
}

/** Displays the current and previous session names in the status bar if that
    setting is enabled. Displays the current session name in the title bar if
    that setting is enabled. */
void app_showSessionInNppBars()
{
    if (!_appReady) {
        return;
    }

    const int maxLen1 = MAX_PATH;
    const int maxLen2 = MAX_PATH_T2;
    TCHAR buf1[MAX_PATH_P1];
    TCHAR buf2[MAX_PATH_T2_P1];
    bool sbar = gCfg.showInStatusbarEnabled();
    bool tbar = gCfg.showInTitlebarEnabled();

    if (sbar || tbar) {
        LOGF("");
    }

    if (sbar) {
        ::StringCchCopy(buf1, maxLen1, _T("session : "));
        ::StringCchCat(buf1, maxLen1, app_getSessionName(SI_CURRENT));
        ::StringCchCat(buf1, maxLen1, _T("    previous : "));
        ::StringCchCat(buf1, maxLen1, app_getSessionName(SI_PREVIOUS));
        ::SendMessage(sys_getNppHandle(), NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)buf1);
    }

    if (tbar) {
        ::GetWindowText(sys_getNppHandle(), buf1, maxLen1);
        removeBracketedPrefix(buf1);
        ::StringCchCopy(buf2, maxLen2, _T("["));
        ::StringCchCat(buf2, maxLen2, app_getSessionName(SI_CURRENT));
        ::StringCchCat(buf2, maxLen2, _T("] "));
        ::StringCchCat(buf2, maxLen2, buf1);
        ::SendMessage(sys_getNppHandle(), WM_SETTEXT, 0, (LPARAM)buf2);
    }
}

//------------------------------------------------------------------------------

namespace {

void onNppReady()
{
    TCHAR name[MAX_PATH_P1];
    name[0] = 0;
    _appReady = true;
    if (gCfg.autoLoadEnabled()) {
        gCfg.readPrevious(name);
        _sesPrvIdx = app_getSessionIndex(name);
        gCfg.readCurrent(name);
        if (name[0] != 0) {
            app_loadSession(app_getSessionIndex(name));
        }
    }
}

/** Removes existing prefixes before adding a new one. */
void removeBracketedPrefix(TCHAR *s)
{
    size_t i = 0, len;
    const int maxLen = MAX_PATH_T2;
    TCHAR buf[MAX_PATH_T2_P1];

    if (::StringCchLength(s, maxLen, &len) == S_OK) {
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
        ::StringCchCopy(buf, maxLen, s + i);
        ::StringCchCopy(s, maxLen, buf);
    }
}

} // end namespace

} // end namespace NppPlugin
