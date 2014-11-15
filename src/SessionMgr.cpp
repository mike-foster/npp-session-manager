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
    @file      SessionMgr.cpp
    @copyright Copyright 2011,2013,2014 Michael Foster <http://mfoster.com/npp/>
*/

#include "System.h"
#include "SessionMgr.h"
#include "Config.h"
#include "Menu.h"
#include "Util.h"
#include "Properties.h"
#include "SessionMgrApi.h"
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
    LPWSTR name;
    FILETIME modified;
    Session(LPCWSTR sesName, FILETIME modTime)
    {
        size_t len;
        if (::StringCchLengthW(sesName, SES_NAME_BUF_LEN, &len) == S_OK) {
            name = (LPWSTR)sys_alloc((len + 1) * sizeof(WCHAR));
            if (name) {
                ::StringCchCopyW(name, SES_NAME_BUF_LEN, sesName);
            }
        }
        modified.dwLowDateTime = modTime.dwLowDateTime;
        modified.dwHighDateTime = modTime.dwHighDateTime;
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
void removeBracketedPrefix(LPWSTR s);
void resetSessions();

} // end namespace

bool sortByAlpha(const Session s1, const Session s2);
bool sortByDate(const Session s1, const Session s2);

//------------------------------------------------------------------------------

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
    resetSessions();
}

void app_init()
{
    LOG("-------------- START %S %s with debug level %i", PLUGIN_FULL_NAME, RES_VERSION_S, gCfg.debug);
    app_readSessionDirectory();
}

LPCWSTR app_getName()
{
    return mnu_getMainMenuLabel();
}

/** Handles Notepad++ and Scintilla notifications and processes timers. */
void app_onNotify(SCNotification *pscn)
{
    uptr_t bufferId = pscn->nmhdr.idFrom;
    unsigned int notificationCode = pscn->nmhdr.code;

    // Notepad++
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

    // Scintilla
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

/** Session Manager API. Handles messages from NPP or a plugin.
    @since v0.8.4.1
    @see   SessionMgrApi.h */
LRESULT app_msgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    if (Message == NPPM_MSGTOPLUGIN) {
        INT si;
        SessionMgrApiData *api = (SessionMgrApiData*)lParam;
        if (gCfg.debug >= 10) {
            switch (api->message) {
                case SESMGRM_SES_LOAD:     LOG("SESMGRM_SES_LOAD"); break;
                case SESMGRM_SES_LOAD_PRV: LOG("SESMGRM_SES_LOAD_PRV"); break;
                case SESMGRM_SES_LOAD_DEF: LOG("SESMGRM_SES_LOAD_DEF"); break;
                case SESMGRM_SES_SAVE:     LOG("SESMGRM_SES_SAVE"); break;
                case SESMGRM_SES_GET_NAME: LOG("SESMGRM_SES_GET_NAME"); break;
                case SESMGRM_SES_GET_FQN:  LOG("SESMGRM_SES_GET_FQN"); break;
                case SESMGRM_CFG_GET_DIR:  LOG("SESMGRM_CFG_GET_DIR"); break;
                case SESMGRM_CFG_GET_EXT:  LOG("SESMGRM_CFG_GET_EXT"); break;
                case SESMGRM_CFG_SET_DIR:  LOG("SESMGRM_CFG_SET_DIR"); break;
                case SESMGRM_CFG_SET_EXT:  LOG("SESMGRM_CFG_SET_EXT"); break;
            }
        }
        if (!_appReady || _sesLoading) {
            LOGG(10, "SESMGR_BUSY");
            api->iData = SESMGR_BUSY;
            return 1;
        }
        switch (api->message) {
            case SESMGRM_SES_LOAD:
                si = app_getSessionIndex((LPWSTR)api->wData);
                if (si <= SI_NONE) {
                    api->iData = SESMGR_ERROR; // session not found in current list
                }
                else {
                    app_loadSession(si);
                }
                break;
            case SESMGRM_SES_LOAD_PRV:
                app_loadSession(SI_PREVIOUS);
                break;
            case SESMGRM_SES_LOAD_DEF:
                app_loadSession(SI_DEFAULT);
                break;
            case SESMGRM_SES_SAVE:
                app_saveSession(SI_CURRENT);
                break;
            case SESMGRM_SES_GET_NAME:
                ::StringCchCopyW((LPWSTR)api->wData, MAX_PATH, app_getSessionName(SI_CURRENT));
                break;
            case SESMGRM_SES_GET_FQN:
                app_getSessionFile(SI_CURRENT, (LPWSTR)api->wData);
                break;
            case SESMGRM_CFG_GET_DIR:
                ::StringCchCopyW((LPWSTR)api->wData, MAX_PATH, gCfg.getSesDir());
                break;
            case SESMGRM_CFG_SET_DIR:
                if (!gCfg.setSesDir((LPWSTR)api->wData)) {
                    api->iData = SESMGR_ERROR; // error creating directory
                }
                else {
                    gCfg.save();
                    app_readSessionDirectory();
                }
                break;
            case SESMGRM_CFG_GET_EXT:
                ::StringCchCopyW((LPWSTR)api->wData, MAX_PATH, gCfg.getSesExt());
                break;
            case SESMGRM_CFG_SET_EXT:
                gCfg.setSesExt((LPWSTR)api->wData);
                gCfg.save();
                app_readSessionDirectory();
                break;
        }
        if (api->iData == SESMGR_NULL) {
            api->iData = SESMGR_OK;
        }
    }
    return 1;
}

} // end namespace api

//------------------------------------------------------------------------------

/** Reads all session names from the session directory. If there is a current
    and/or previous session it is made current and/or previous again if it is
    in the new list. */
void app_readSessionDirectory()
{
    HANDLE hFind;
    WIN32_FIND_DATAW ffd;
    WCHAR sesFileSpec[MAX_PATH];
    WCHAR sesName[SES_NAME_BUF_LEN];
    WCHAR sesCur[SES_NAME_BUF_LEN];
    WCHAR sesPrv[SES_NAME_BUF_LEN];

    LOGF("");

    // Clear the sessions vector.
    sesCur[0] = 0;
    sesPrv[0] = 0;
    if (!_sessions.empty()) {
        // If a session is current/previous save its name.
        if (_sesCurIdx > SI_NONE) {
            ::StringCchCopyW(sesCur, MAX_PATH, _sessions[_sesCurIdx].name);
        }
        if (_sesPrvIdx > SI_NONE) {
            ::StringCchCopyW(sesPrv, MAX_PATH, _sessions[_sesPrvIdx].name);
        }
        resetSessions();
    }
    // Create the file spec.
    ::StringCchCopyW(sesFileSpec, MAX_PATH, gCfg.getSesDir());
    ::StringCchCatW(sesFileSpec, MAX_PATH, L"*");
    ::StringCchCatW(sesFileSpec, MAX_PATH, gCfg.getSesExt());
    // Loop over files in the session directory, save each in the vector.
    hFind = ::FindFirstFileW(sesFileSpec, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        _sesCurIdx = SI_DEFAULT;
        return;
    }
    _appReady = false;
    do {
        ::StringCchCopyW(sesName, SES_NAME_BUF_LEN, ffd.cFileName);
        pth::removeExt(sesName, SES_NAME_BUF_LEN);
        Session ses(sesName, ffd.ftLastWriteTime);
        _sessions.push_back(ses);
    }
    while (::FindNextFileW(hFind, &ffd) != 0);
    DWORD lastError = ::GetLastError();
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
    if (lastError != ERROR_NO_MORE_FILES) {
        msg::error(lastError, L"%s: Error reading session files \"%s\".", _W(__FUNCTION__), sesFileSpec);
    }
    _appReady = true;
}

/** Sorts the sessions vector ascending alphabetically. */
bool sortByAlpha(const Session s1, const Session s2)
{
    return ::lstrcmpW(s1.name, s2.name) <= 0;
}

/** Sorts the sessions vector descending by the file's last modified time. For
    sessions that have the same last modified time it sorts ascending alphabetically. */
bool sortByDate(const Session s1, const Session s2)
{
    INT result = ::CompareFileTime(&s1.modified, &s2.modified);
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
    WCHAR sesFile[MAX_PATH];
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
        ::SendMessageW(hNpp, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSEALL);
    }
    LOGG(10, "Opening documents for session %i", si);
    // Load session
    ::SendMessageW(hNpp, NPPM_LOADSESSION, 0, (LPARAM)sesFile);
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
    WCHAR sesFile[MAX_PATH];
    app_getSessionFile(si, sesFile);

    ::SendMessageW(sys_getNppHandle(), NPPM_SAVECURRENTSESSION, 0, (LPARAM)sesFile); // Save session
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
INT app_getSessionIndex(LPWSTR name)
{
    if (name == NULL) {
        return _sesCurIdx;
    }
    INT i = 0;
    vector<Session>::iterator it;
    for (it = _sessions.begin(); it < _sessions.end(); ++it) {
        if (::lstrcmpW(it->name, name) == 0) {
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
bool app_renameSession(INT si, LPWSTR newName)
{
    bool curOrPrv = false;
    if (app_isValidSessionIndex(si)) {
        ::StringCchCopyW(_sessions[si].name, SES_NAME_BUF_LEN, newName);
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
    session's name. Else returns a pointer to the 'none' or 'default' session name. */
LPCWSTR app_getSessionName(INT si)
{
    if (si == SI_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SI_PREVIOUS) {
        si = _sesPrvIdx;
    }
    if (si == SI_DEFAULT) {
        return SES_NAME_DEFAULT;
    }
    return app_isValidSessionIndex(si) ? _sessions[si].name : SES_NAME_NONE;
}

/** Copies into buf the full pathname of the session at index si. If si is
    SI_CURRENT or SI_PREVIOUS copies the current or previous session's
    pathname. Else copies the default session pathname. */
void app_getSessionFile(INT si, LPWSTR buf)
{
    if (si == SI_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SI_PREVIOUS) {
        si = _sesPrvIdx;
    }
    if (app_isValidSessionIndex(si)) {
        ::StringCchCopyW(buf, MAX_PATH, gCfg.getSesDir());
        ::StringCchCatW(buf, MAX_PATH, _sessions[si].name);
        ::StringCchCatW(buf, MAX_PATH, gCfg.getSesExt());
    }
    else {
        ::StringCchCopyW(buf, MAX_PATH, sys_getCfgDir());
        ::StringCchCatW(buf, MAX_PATH, SES_NAME_DEFAULT);
        ::StringCchCatW(buf, MAX_PATH, gCfg.getSesExt());
        pth::createFileIfMissing(buf, SES_DEFAULT_CONTENTS);
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

    WCHAR buf1[MAX_PATH];
    WCHAR buf2[MAX_PATH];
    bool sbar = gCfg.showInStatusbarEnabled();
    bool tbar = gCfg.showInTitlebarEnabled();

    if (sbar || tbar) {
        LOGF("");
    }
    if (sbar) {
        ::StringCchCopyW(buf1, MAX_PATH, L"session : ");
        ::StringCchCatW(buf1, MAX_PATH, app_getSessionName(SI_CURRENT));
        ::StringCchCatW(buf1, MAX_PATH, L"    previous : ");
        ::StringCchCatW(buf1, MAX_PATH, app_getSessionName(SI_PREVIOUS));
        ::SendMessageW(sys_getNppHandle(), NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)buf1);
    }
    if (tbar) {
        ::GetWindowTextW(sys_getNppHandle(), buf1, MAX_PATH);
        removeBracketedPrefix(buf1);
        ::StringCchCopyW(buf2, MAX_PATH, L"[");
        ::StringCchCatW(buf2, MAX_PATH, app_getSessionName(SI_CURRENT));
        ::StringCchCatW(buf2, MAX_PATH, L"] ");
        ::StringCchCatW(buf2, MAX_PATH, buf1);
        ::SendMessageW(sys_getNppHandle(), WM_SETTEXT, 0, (LPARAM)buf2);
    }
}

//------------------------------------------------------------------------------

namespace {

void onNppReady()
{
    WCHAR name[MAX_PATH];
    name[0] = 0;
    _appReady = true;
    if (gCfg.autoLoadEnabled()) {
        gCfg.readPrevious(name);
        _sesPrvIdx = app_getSessionIndex(name);
        gCfg.readCurrent(name);
        if (name[0] != 0) {
            /* On startup, pass lic=false because there is no current session here,
            and pass lwc=true so we don't close files opened via the NPP command line. */
            app_loadSession(app_getSessionIndex(name), false, true);
        }
    }
}

/** Removes existing prefixes before adding a new one. */
void removeBracketedPrefix(LPWSTR s)
{
    size_t i = 0, len;
    WCHAR buf[MAX_PATH];

    if (::StringCchLengthW(s, MAX_PATH, &len) == S_OK) {
        while (i < len && *(s + i) == L'[') {
            while (i < len && *(s + i) != L']') {
                ++i;
            }
            if (i < len && *(s + i) == L']') {
                ++i;
            }
            while (i < len && *(s + i) == L' ') {
                ++i;
            }
        }
        ::StringCchCopyW(buf, MAX_PATH, s + i);
        ::StringCchCopyW(s, MAX_PATH, buf);
    }
}

void resetSessions()
{
    vector<Session>::iterator it;
    for (it = _sessions.begin(); it < _sessions.end(); ++it) {
        sys_free(it->name);
    }
    _sessions.clear();
}

} // end namespace

} // end namespace NppPlugin
