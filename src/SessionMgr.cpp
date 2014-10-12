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
#include <algorithm>
#include <strsafe.h>
#include <time.h>
#include <vector>
#include "xml\tinyxml2.h"

using std::vector;

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

#define NPP_BOOKMARK_MARGIN_ID 1

// XML nodes
#define XN_NOTEPADPLUS "NotepadPlus" // root node
#define XN_SESSION "Session"
#define XN_MAINVIEW "mainView"
#define XN_SUBVIEW "subView"
#define XN_FILE "File"
#define XN_MARK "Mark"
#define XN_FILEPROPERTIES "FileProperties"
// XML attributes
#define XA_FILENAME "filename"
#define XA_LANG "lang"
#define XA_ENCODING "encoding"
#define XA_FIRSTVISIBLELINE "firstVisibleLine"
#define XA_LINE "line"

class Session
{
    public:
    TCHAR name[SES_MAX_LEN];
    FILETIME modifiedTime;
    Session(TCHAR *sn, FILETIME mod)
    {
        StringCchCopy(name, SES_MAX_LEN - 1, sn);
        modifiedTime.dwLowDateTime = mod.dwLowDateTime;
        modifiedTime.dwHighDateTime = mod.dwHighDateTime;
    }
};

vector<Session> _sessions;
INT _sesCurIdx; // the _sessions index of the current session
INT _sesPrvIdx; // the _sessions index of the previous session
INT _bidFileOpened; // bufferId from most recent NPPN_FILEOPENED
INT _bidBufferActivated; // XXX experimental. bufferId from most recent NPPN_BUFFERACTIVATED
bool _sesIsDirty; // XXX experimental. if true, current session needs to be saved. this should only be set true in cases where saving the session is deferred
bool _appReady;
bool _sesLoading;
time_t _shutdownTimer; // for determining if files are closing due to a shutdown
time_t _titlebarTimer; // for updating the titlebar text

void onNppReady(); 
void removeBracketedPrefix(TCHAR *s);
void app_updateGlobalFromSession(TCHAR *sesFile);
void app_updateSessionFromGlobal(TCHAR *sesFile);
void app_updateDocumentFromGlobal(INT bufferId);

} // end namespace

bool sortByAlpha(const Session s1, const Session s2);
bool sortByDate(const Session s1, const Session s2);

//------------------------------------------------------------------------------
// The api namespace contains functions called only from DllMain.

namespace api {

void app_onLoad()
{
    _appReady = false;
    _sesLoading = false;
    _sesCurIdx = SES_DEFAULT;
    _sesPrvIdx = SES_NONE;
    _bidFileOpened = 0;
    _bidBufferActivated = 0;
    _sesIsDirty = false;
    _shutdownTimer = 0;
    _titlebarTimer = 0;
}

void app_onUnload()
{
    LOG("---------- STOP %S %s", PLUGIN_FULL_NAME, RES_VERSION_S);
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

/* Handles Notepad++ and Scintilla notifications. */
void app_onNotify(SCNotification *pscn)
{
    uptr_t bufferId = pscn->nmhdr.idFrom;
    unsigned int notificationCode = pscn->nmhdr.code;

    // Notepad++ notifications
    if (pscn->nmhdr.hwndFrom == sys_getNppHwnd()) {
        if (gCfg.debug >= 10) {
            switch (notificationCode) {
                case NPPN_READY:           LOG("NPPN_READY"); break;
                case NPPN_SHUTDOWN:        LOG("NPPN_SHUTDOWN"); break; // We need NPPN_BEFORESHUTDOWN
                case NPPN_FILEBEFORESAVE:  LOG("NPPN_FILEBEFORESAVE \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_FILESAVED:       LOG("NPPN_FILESAVED      \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_FILEBEFORELOAD:  LOG("NPPN_FILEBEFORELOAD \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_FILELOADFAILED:  LOG("NPPN_FILELOADFAILED \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_FILEBEFOREOPEN:  LOG("NPPN_FILEBEFOREOPEN \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_FILEOPENED:      LOG("NPPN_FILEOPENED     \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_FILEBEFORECLOSE: LOG("NPPN_FILEBEFORECLOSE\t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_FILECLOSED:      LOG("NPPN_FILECLOSED     \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_LANGCHANGED:     LOG("NPPN_LANGCHANGED    \t%8i\t%i", bufferId, _bidBufferActivated); break;
                case NPPN_DOCORDERCHANGED: LOG("NPPN_DOCORDERCHANGED\t%8i\t%i", bufferId, _bidBufferActivated); break; // Does not occur?
                case NPPN_BUFFERACTIVATED: LOG("NPPN_BUFFERACTIVATED\t%8i\t%i", bufferId, _bidBufferActivated); break;
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
                if (_appReady && !_sesLoading && gCfg.getAutoSave()) {
                    app_saveSession(_sesCurIdx);
                }
                break;
            //case NPPN_FILEBEFORECLOSE: // XXX experimental
            //    LOGG(10, "Shutdown %s be in progress", (bufferId != _bidBufferActivated) ? "MAY" : "may NOT");
            //    break;
            case NPPN_FILECLOSED:
                _sesIsDirty = true;
                if (_appReady && !_sesLoading && gCfg.getAutoSave()) {
                    _shutdownTimer = time(NULL);
                    LOGG(10, "Save session in %i seconds if no shutdown", gCfg.getSaveDelay());
                }
                break;
            case NPPN_BUFFERACTIVATED:
                _bidBufferActivated = bufferId;
                if (_appReady && !_sesLoading) {
                    app_showSessionInNppBars();
                    if (_bidFileOpened == bufferId) { // buffer activated immediately after NPPN_FILEOPENED
                        if (gCfg.getGlobalBookmarks()) {
                            app_updateDocumentFromGlobal(_bidFileOpened);
                        }
                        if (gCfg.getAutoSave()) {
                            app_saveSession(_sesCurIdx);
                        }
                    }
                }
                _bidFileOpened = 0;
                break;
        } // end switch
    }

    // Scintilla notifications
    else if (pscn->nmhdr.hwndFrom == sys_getSc1Hwnd() || pscn->nmhdr.hwndFrom == sys_getSc2Hwnd()) {
        if (gCfg.debug >= 10) {
            switch (notificationCode) {
                case SCN_SAVEPOINTREACHED: LOG("SCN_SAVEPOINTREACHED\t        \t%i", _bidBufferActivated); break;
                case SCN_SAVEPOINTLEFT:    LOG("SCN_SAVEPOINTLEFT   \t        \t%i", _bidBufferActivated); break;
                case SCN_MARGINCLICK:      LOG("SCN_MARGINCLICK     \t        \t%i", _bidBufferActivated); break;
            }
        }
        switch (notificationCode) {
            case SCN_SAVEPOINTLEFT:
                _sesIsDirty = true;
                if (gCfg.getShowInTitlebar()) {
                    _titlebarTimer = time(NULL);
                }
                break;
            case SCN_MARGINCLICK:
                if (_appReady && !_sesLoading && gCfg.getAutoSave() && pscn->margin == NPP_BOOKMARK_MARGIN_ID) {
                    app_saveSession(_sesCurIdx);
                }
                break;
        }
    }

    // Timers
    if (_shutdownTimer > 0) {
        if (_appReady && !_sesLoading) {
            if (time(NULL) - _shutdownTimer > gCfg.getSaveDelay()) {
                _shutdownTimer = 0;
                app_saveSession(_sesCurIdx);
            }
        }
        else {
            _shutdownTimer = 0;
        }
    }
    if (_titlebarTimer > 0) {
        if (time(NULL) - _titlebarTimer > 1) {
            _titlebarTimer = 0;
            app_showSessionInNppBars();
        }
    }
}

LRESULT app_msgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    // Would it be better to handle the timers here? It doesn't seem to make any difference.

    return 1;
}

} // end namespace api

//------------------------------------------------------------------------------

/* Reads all session names from the session directory. If there is a current
   and/or previous session it is made current and/or previous again if it is
   in the new list. */
void app_readSessionDirectory()
{
    DWORD dwError=0;
    WIN32_FIND_DATA ffd;
    TCHAR sesFileSpec[MAX_PATH_P1];
    TCHAR sesName[SES_MAX_LEN];
    TCHAR sesCur[SES_MAX_LEN];
    TCHAR sesPrv[SES_MAX_LEN];
    HANDLE hFind = INVALID_HANDLE_VALUE;

    LOGF("");

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
        Session ses(sesName, ffd.ftLastWriteTime);
        _sessions.push_back(ses);
    }
    while (FindNextFile(hFind, &ffd) != 0);
    dwError = GetLastError();
    FindClose(hFind);
    // Sort before restoring indexes
    if (gCfg.isSortAlpha()) {
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

/* Sorts the sessions vector ascending alphabetically. */
bool sortByAlpha(const Session s1, const Session s2)
{
    return lstrcmp(s1.name, s2.name) <= 0;
}

/* Sorts the sessions vector descending by the file's last modified time. For
   sessions that have the same last modified time it sorts ascending alphabetically. */
bool sortByDate(const Session s1, const Session s2)
{
    INT result = CompareFileTime(&s1.modifiedTime, &s2.modifiedTime);
    if (result < 0) {
        return false;
    }
    if (result > 0) {
        return true;
    }
    return sortByAlpha(s1, s2);
}

/* Loads the session at index si. Makes it the current index unless lic is
   true. Closes the previous session before loading si, unless lwc is true. */
void app_loadSession(INT si)
{
    app_loadSession(si, gCfg.getLoadIntoCurrent(), gCfg.getLoadWithoutClosing());
}
void app_loadSession(INT si, bool lic, bool lwc)
{
    TCHAR sesFile[MAX_PATH_P1];
    HWND hNpp = sys_getNppHwnd();

    LOGF("%i, %i, %i", si, lic, lwc);

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

    app_getSessionFile(si, sesFile);
    if (gCfg.getGlobalBookmarks()) {
        app_updateSessionFromGlobal(sesFile);
    }

    // Close all open files
    if (!lwc) {
        LOGG(10, "Closing documents for session %i", _sesCurIdx);
        SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSEALL);
    }
    LOGG(10, "Opening documents for session %i", si);
    // Load session
    SendMessage(hNpp, NPPM_LOADSESSION, 0, (LPARAM)sesFile);
    _sesLoading = false;
    if (!lic) {
        if (si > SES_NONE) {
            _sesCurIdx = si;
            gCfg.saveCurrent(_sessions[si].name); // Write new current session name to ini file
            app_showSessionInNppBars();
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
    LOGF("%i", si);

    if (!_appReady || _sesLoading) {
        return;
    }
    LOGG(10, "Session %s dirty", _sesIsDirty ? "IS" : "is NOT"); // XXX experimental
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SES_PREVIOUS) {
        si = _sesPrvIdx;
    }
    TCHAR sesFile[MAX_PATH_P1];
    app_getSessionFile(si, sesFile);

    SendMessage(sys_getNppHwnd(), NPPM_SAVECURRENTSESSION, 0, (LPARAM)sesFile); // Save session
    _sesCurIdx = si > SES_NONE ? si : SES_DEFAULT;
    if (gCfg.getGlobalBookmarks()) {
        app_updateGlobalFromSession(sesFile);
    }
    _sesIsDirty = false;
}

/* Returns true if session index si is valid, else false. */
bool app_isValidSessionIndex(INT si)
{
    return (si >= 0 && si < (signed)_sessions.size());
}

/* Returns the number of items in the _sessions vector. */
INT app_getSessionCount()
{
    return _sessions.size();
}

/* Returns the index of name in _sessions, else SES_NONE if not found.
   If name is NULL returns the current session's index. */
INT app_getSessionIndex(TCHAR *name)
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
const TCHAR* app_getSessionName(INT si)
{
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SES_PREVIOUS) {
        si = _sesPrvIdx;
    }
    return app_isValidSessionIndex(si) ? _sessions[si].name : SES_DEFAULT_NAME;
}

/* Copies into buf the full pathname of the session at index si. If si is
   SES_CURRENT or SES_PREVIOUS copies the current or previous session's
   pathname. Else copies the default session pathname. */
void app_getSessionFile(INT si, TCHAR *buf)
{
    if (si == SES_CURRENT) {
        si = _sesCurIdx;
    }
    else if (si == SES_PREVIOUS) {
        si = _sesPrvIdx;
    }
    if (app_isValidSessionIndex(si)) {
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
void app_showSessionInNppBars()
{
    if (!_appReady) {
        return;
    }

    const int maxLen1 = MAX_PATH;
    const int maxLen2 = MAX_PATH_T2;
    TCHAR buf1[MAX_PATH_P1];
    TCHAR buf2[MAX_PATH_T2_P1];
    bool sbar = gCfg.getShowInStatusbar();
    bool tbar = gCfg.getShowInTitlebar();

    if (sbar || tbar) {
        LOGF("");
    }

    if (sbar) {
        StringCchCopy(buf1, maxLen1, _T("session : "));
        StringCchCat(buf1, maxLen1, app_getSessionName(SES_CURRENT));
        StringCchCat(buf1, maxLen1, _T("    previous : "));
        StringCchCat(buf1, maxLen1, app_getSessionName(SES_PREVIOUS));
        SendMessage(sys_getNppHwnd(), NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)buf1);
    }

    if (tbar) {
        GetWindowText(sys_getNppHwnd(), buf1, maxLen1);
        removeBracketedPrefix(buf1);
        StringCchCopy(buf2, maxLen2, _T("["));
        StringCchCat(buf2, maxLen2, app_getSessionName(SES_CURRENT));
        StringCchCat(buf2, maxLen2, _T("] "));
        StringCchCat(buf2, maxLen2, buf1);
        SendMessage(sys_getNppHwnd(), WM_SETTEXT, 0, (LPARAM)buf2);
    }
}

//------------------------------------------------------------------------------

namespace {

void onNppReady()
{
    TCHAR name[MAX_PATH_P1];
    name[0] = 0;
    _appReady = true;
    if (gCfg.getAutoLoad()) {
        gCfg.readPrevious(name);
        _sesPrvIdx = app_getSessionIndex(name);
        gCfg.readCurrent(name);
        if (name[0] != 0) {
            app_loadSession(app_getSessionIndex(name));
        }
    }
}

/* We need to remove any existing prefixes before adding a new one. */
void removeBracketedPrefix(TCHAR *s)
{
    size_t i = 0, len;
    const int maxLen = MAX_PATH_T2;
    TCHAR buf[MAX_PATH_T2_P1];

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

/* TODO: app_updateGlobalFromSession, app_updateSessionFromGlobal and
   app_updateDocumentFromGlobal need to be refactored, perhaps into a class.

Example session file:

<NotepadPlus>
    <Session activeView="0">
        <mainView activeIndex="0">
            <File firstVisibleLine="444" xOffset="0" scrollWidth="1696" startPos="14583" endPos="14583" selMode="0" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\SessionMgr.cpp">
                <Mark line="312" />
                <Mark line="466" />
            </File>
        </mainView>
        <subView activeIndex="0">
            <File firstVisibleLine="451" xOffset="0" scrollWidth="1168" startPos="12528" endPos="12528" selMode="0" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\xml\tinyxml2.h">
                <Mark line="483" />
            </File>
        </subView>
    </Session>
</NotepadPlus>

Example file-properties.xml file:

<NotepadPlus>
    <FileProperties>
        <File firstVisibleLine="444" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\SessionMgr.cpp">
            <Mark line="312" />
            <Mark line="466" />
        </File>
        <File firstVisibleLine="451" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\xml\tinyxml2.h">
            <Mark line="483" />
        </File>
    </FileProperties>
</NotepadPlus>
*/

/* Updates global file properties from local (session) file properties.
   After a session is saved, the global bookmarks, firstVisibleLine, language
   and encoding are updated from the session properties. */
void app_updateGlobalFromSession(TCHAR *sesFile)
{
    const char *p;
    tinyxml2::XMLError err;

    LOGF("%S", sesFile);

    // Load the properties file (global file properties)
    tinyxml2::XMLDocument globalDoc;
    err = globalDoc.LoadFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *globalPropsEle, *globalFileEle, *globalMarkEle;
    tinyxml2::XMLHandle globalDocHnd(&globalDoc);
    globalPropsEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).ToElement();

    // Load the session file (file properties local to a session)
    size_t num;
    char mbSesFile[MAX_PATH_T2];
    wcstombs_s(&num, mbSesFile, MAX_PATH_T2, sesFile, _TRUNCATE);
    tinyxml2::XMLDocument localDoc;
    err = localDoc.LoadFile(mbSesFile);
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the session file: '%s'.", err, mbSesFile);
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *localViewEle, *localFileEle, *localMarkEle;
    tinyxml2::XMLHandle localDocHnd(&localDoc);

    // Iterate over the local View elements
    localViewEle = localDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SESSION).FirstChildElement(XN_MAINVIEW).ToElement();
    while (localViewEle) {
        // Iterate over the local File elements
        localFileEle = localViewEle->FirstChildElement(XN_FILE);
        while (localFileEle) {
            // Find the global File element corresponding to the current local File element
            p = localFileEle->Attribute(XA_FILENAME);
            LOGG(30, "File = %s", p);
            globalFileEle = globalPropsEle->FirstChildElement(XN_FILE);
            while (globalFileEle) {
                if (globalFileEle->Attribute(XA_FILENAME, p)) {
                    break; // found it
                }
                globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
            }
            if (!globalFileEle) { // not found so create one
                globalFileEle = globalDoc.NewElement(XN_FILE);
                globalFileEle->SetAttribute(XA_FILENAME, p);
            }
            globalPropsEle->InsertFirstChild(globalFileEle); // an existing element will get moved to the top
            // Update global File attributes with values from the current local File attributes
            globalFileEle->SetAttribute(XA_LANG, localFileEle->Attribute(XA_LANG));
            globalFileEle->SetAttribute(XA_ENCODING, localFileEle->Attribute(XA_ENCODING));
            globalFileEle->SetAttribute(XA_FIRSTVISIBLELINE, localFileEle->Attribute(XA_FIRSTVISIBLELINE));
            globalFileEle->DeleteChildren();
            LOGG(30, "lang = '%s', encoding = '%s', firstVisibleLine = %s", localFileEle->Attribute(XA_LANG), localFileEle->Attribute(XA_ENCODING), localFileEle->Attribute(XA_FIRSTVISIBLELINE));
            // Iterate over the local Mark elements for the current local File element
            localMarkEle = localFileEle->FirstChildElement(XN_MARK);
            while (localMarkEle) {
                globalMarkEle = globalDoc.NewElement(XN_MARK);
                globalFileEle->InsertEndChild(globalMarkEle);
                // Update global Mark attributes with values from the current local Mark attributes
                globalMarkEle->SetAttribute(XA_LINE, localMarkEle->Attribute(XA_LINE));
                LOGG(30, "Mark = %s", localMarkEle->Attribute(XA_LINE));
                localMarkEle = localMarkEle->NextSiblingElement(XN_MARK);
            }
            localFileEle = localFileEle->NextSiblingElement(XN_FILE);
        }
        localViewEle = localViewEle->NextSiblingElement(XN_SUBVIEW);
    }

    // Save changes to the properties file
    err = globalDoc.SaveFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i saving the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
    }
}

/* Updates local (session) file properties from global file properties.
   When a session is about to be loaded, the session bookmarks, language and
   encoding are updated from the global properties, then the session is loaded. */
void app_updateSessionFromGlobal(TCHAR *sesFile)
{
    const char *p;
    bool save = false;
    tinyxml2::XMLError err;

    LOGF("%S", sesFile);

    // Load the properties file (global file properties)
    tinyxml2::XMLDocument globalDoc;
    err = globalDoc.LoadFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *globalPropsEle, *globalFileEle, *globalMarkEle;
    tinyxml2::XMLHandle globalDocHnd(&globalDoc);
    globalPropsEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).ToElement();

    // Load the session file (file properties local to a session)
    size_t num;
    char mbSesFile[MAX_PATH_T2];
    wcstombs_s(&num, mbSesFile, MAX_PATH_T2, sesFile, _TRUNCATE);
    tinyxml2::XMLDocument localDoc;
    err = localDoc.LoadFile(mbSesFile);
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the session file: '%s'.", err, mbSesFile);
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *localViewEle, *localFileEle, *localMarkEle;
    tinyxml2::XMLHandle localDocHnd(&localDoc);

    // Iterate over the local View elements
    localViewEle = localDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SESSION).FirstChildElement(XN_MAINVIEW).ToElement();
    while (localViewEle) {
        // Iterate over the local File elements
        localFileEle = localViewEle->FirstChildElement(XN_FILE);
        while (localFileEle) {
            // Find the global File element corresponding to the current local File element
            p = localFileEle->Attribute(XA_FILENAME);
            LOGG(30, "File = %s", p);
            globalFileEle = globalPropsEle->FirstChildElement(XN_FILE);
            while (globalFileEle) {
                if (globalFileEle->Attribute(XA_FILENAME, p)) {
                    break; // found it
                }
                globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
            }
            if (globalFileEle) {
                save = true;
                // Update current local File attributes with values from the global File attributes
                localFileEle->SetAttribute(XA_LANG, globalFileEle->Attribute(XA_LANG));
                localFileEle->SetAttribute(XA_ENCODING, globalFileEle->Attribute(XA_ENCODING));
                localFileEle->DeleteChildren();
                LOGG(30, "lang = '%s', encoding = '%s'", globalFileEle->Attribute(XA_LANG), globalFileEle->Attribute(XA_ENCODING));
                // Iterate over the global Mark elements for the current global File element
                globalMarkEle = globalFileEle->FirstChildElement(XN_MARK);
                while (globalMarkEle) {
                    localMarkEle = localDoc.NewElement(XN_MARK);
                    localFileEle->InsertEndChild(localMarkEle);
                    // Update local Mark attributes with values from the current global Mark attributes
                    localMarkEle->SetAttribute(XA_LINE, globalMarkEle->Attribute(XA_LINE));
                    LOGG(30, "Mark = %s", globalMarkEle->Attribute(XA_LINE));
                    globalMarkEle = globalMarkEle->NextSiblingElement(XN_MARK);
                }
            }
            //else {
            //    XXX not found
            //    This indicates global needs to be updated from this session,
            //    but we can't call app_updateGlobalFromSession here.
            //}
            localFileEle = localFileEle->NextSiblingElement(XN_FILE);
        }
        localViewEle = localViewEle->NextSiblingElement(XN_SUBVIEW);
    }

    // Save changes to the session file
    if (save) {
        err = localDoc.SaveFile(mbSesFile);
        if (err != tinyxml2::XML_SUCCESS) {
            LOG("Error %i saving the session file: '%s'.", err, mbSesFile);
            SHOW_ERROR;
        }
    }
}

/* Updates document properties from global file properties.
   When an existing document is added to a session, its bookmarks and
   firstVisibleLine are updated from the global properties, then the session
   is saved. */
void app_updateDocumentFromGlobal(INT bufferId)
{
    size_t num;
    INT line, pos;
    bool isMainView;
    TCHAR pathname[MAX_PATH_P1];
    char mbPathname[MAX_PATH_T2_P1];
    HWND hNpp = sys_getNppHwnd();

    LOGF("%i", bufferId);

    // Get pathname for bufferId
    SendMessage(hNpp, NPPM_GETFULLPATHFROMBUFFERID, bufferId, (LPARAM)pathname);
    wcstombs_s(&num, mbPathname, MAX_PATH_T2, pathname, _TRUNCATE);
    LOGG(20, "File = %s", mbPathname);

    // Load the properties file (global file properties)
    tinyxml2::XMLDocument globalDoc;
    tinyxml2::XMLError err = globalDoc.LoadFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *globalFileEle, *globalMarkEle;
    tinyxml2::XMLHandle globalDocHnd(&globalDoc);
    globalFileEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).FirstChildElement(XN_FILE).ToElement();

    // Find the global File element corresponding to mbPathname
    while (globalFileEle) {
        if (globalFileEle->Attribute(XA_FILENAME, mbPathname)) {
            break; // found it
        }
        globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
    }
    if (!globalFileEle) { // not found
        return;
    }

    // TODO: If I knew how I would set lang and encoding here

    // Determine containing view and tab for bufferId
    pos = SendMessage(hNpp, NPPM_GETPOSFROMBUFFERID, bufferId, 0);
    LOGG(20, "Pos = 0x%X", pos);
    isMainView = (pos & (1 << 30)) == 0;

    // Iterate over the global Mark elements and set them in the active document
    globalMarkEle = globalFileEle->FirstChildElement(XN_MARK);
    while (globalMarkEle) {
        line = globalMarkEle->IntAttribute(XA_LINE);
        // go to line and set mark
        if (isMainView) {
            SendMessage(sys_getSc1Hwnd(), SCI_GOTOLINE, line, 0);
        }
        else {
            SendMessage(sys_getSc2Hwnd(), SCI_GOTOLINE, line, 0);
        }
        SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_SEARCH_TOGGLE_BOOKMARK);
        LOGG(20, "Mark = %i", line);
        globalMarkEle = globalMarkEle->NextSiblingElement(XN_MARK);
    }

    // Move cursor to the last known firstVisibleLine
    line = globalFileEle->IntAttribute(XA_FIRSTVISIBLELINE);
    if (isMainView) {
        SendMessage(sys_getSc1Hwnd(), SCI_GOTOLINE, line, 0);
    }
    else {
        SendMessage(sys_getSc2Hwnd(), SCI_GOTOLINE, line, 0);
    }
    LOGG(20, "firstVisibleLine = %i", line);
}

} // end namespace

} // end namespace NppPlugin
