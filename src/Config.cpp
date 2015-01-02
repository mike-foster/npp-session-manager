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
    @file      Config.cpp
    @copyright Copyright 2011-2015 Michael Foster <http://mfoster.com/npp/>

    This is deprecated as of v1.2.
*/

#include "System.h"
#include "SessionMgr.h"
#include "Config.h"
#include "Util.h"
#include <strsafe.h>
#include <shlobj.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

Config gCfg; // The global configuration/settings object.

//------------------------------------------------------------------------------

namespace {

#define INI_SESSION L"session"
#define INI_SES_ASV L"autoSave"
#define INI_SES_ASV_DV 1
#define INI_SES_ALD L"autoLoad"
#define INI_SES_ALD_DV 0
#define INI_SES_GBM L"globalBookmarks"
#define INI_SES_GBM_DV 1
#define INI_SES_CTX L"useContextMenu"
#define INI_SES_CTX_DV 1
#define INI_SES_LIC L"loadIntoCurrent"
#define INI_SES_LIC_DV 0
#define INI_SES_LWC L"loadWithoutClosing"
#define INI_SES_LWC_DV 0
#define INI_SES_SORT L"sortOrder"
#define INI_SES_SORT_DV 1
#define INI_SES_SISB L"showInStatusbar"
#define INI_SES_SISB_DV 0
#define INI_SES_SITB L"showInTitlebar"
#define INI_SES_SITB_DV 0
#define INI_SES_SVD L"saveDelay"
#define INI_SES_SVD_DV 3
#define INI_SES_DIR L"directory"
#define INI_SES_DIR_DV EMPTY_STR
#define INI_SES_EXT L"extension"
#define INI_SES_EXT_DV EMPTY_STR
#define INI_SES_CUR L"current"
#define INI_SES_PRV L"previous"
#define INI_SES_DEF L"default"
#define INI_SES_DEF_DV SES_NAME_DEFAULT

#define INI_FILTER L"filter"
#define INI_FIL_EXP_PFX L"fil"

#define INI_MENU L"menu"
#define INI_MNU_MAIN L"main"
#define INI_MNU_SUB_PFX L"sub"
#define INI_MNU_FAV_PFX L"fav"

#define INI_DIALOG L"dialog"
#define INI_DLG_SES_W L"sessionsW"
#define INI_DLG_SES_W_DV 0
#define INI_DLG_SES_H L"sessionsH"
#define INI_DLG_SES_H_DV 0
#define INI_DLG_CFG_W L"settingsW"
#define INI_DLG_CFG_W_DV 0
#define INI_DLG_CFG_H L"settingsH"
#define INI_DLG_CFG_H_DV 0

#define INI_DEBUG L"debug"
#define INI_DBG_DBG L"debug"
#define INI_DBG_DBG_DV 0
#define INI_DBG_LOG_FILE L"logFile"

#define TMP_BUF_LEN 30
#define DEFAULT_SES_DIR L"sessions\\"
#define DEFAULT_SES_EXT L".npp-session"
#define DEFAULT_INI_CONTENTS "[session]\nautoSave=1\nautoLoad=0\nglobalBookmarks=1\nloadIntoCurrent=0\nloadWithoutClosing=0\nsortOrder=1\nshowInTitlebar=0\nshowInStatusbar=0\nsaveDelay=3\ndirectory=\nextension=\ncurrent=\nprevious=\ndefault=\n\n[menu]\n\n[filter]\n\n[dialog]\nsessionsW=0\nsessionsH=0\nsettingsW=0\nsettingsH=0\n\n[debug]\ndebug=0\nlogFile=\n"

} // end namespace

//------------------------------------------------------------------------------

/** Reads properties from the ini file. */
void Config::load()
{
    LPWSTR iniFile = sys_getIniFile();

    if (pth::fileExists(iniFile)) {
        iniFileLoaded = true;
    }
    else {
        // we must have already upgraded to xml
        iniFileLoaded = false;
        return;
    }

    // session directory property
    _directory[0] = 0;
    ::GetPrivateProfileStringW(INI_SESSION, INI_SES_DIR, INI_SES_DIR_DV, _directory, MAX_PATH, iniFile);
    if (_directory[0] == 0) {
        ::StringCchCopyW(_directory, MAX_PATH, sys_getCfgDir());
        ::StringCchCatW(_directory, MAX_PATH, DEFAULT_SES_DIR);
    }
    pth::appendSlash(_directory, MAX_PATH);

    // session extension property
    _extension[0] = 0;
    ::GetPrivateProfileStringW(INI_SESSION, INI_SES_EXT, INI_SES_EXT_DV, _extension, MAX_PATH, iniFile);
    if (_extension[0] == 0) {
        ::StringCchCopyW(_extension, MAX_PATH, DEFAULT_SES_EXT);
    }

    // session filters
    loadFilters();

    // default session name
    ::GetPrivateProfileStringW(INI_SESSION, INI_SES_DEF, INI_SES_DEF_DV, _defaultName, SES_NAME_BUF_LEN, iniFile);

    // session marks
    loadMarks();

    // boolean properties
    _autoSave = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_ASV, INI_SES_ASV_DV, iniFile));
    _autoLoad = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_ALD, INI_SES_ALD_DV, iniFile));
    _globalBookmarks = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_GBM, INI_SES_GBM_DV, iniFile));
    _useContextMenu = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_CTX, INI_SES_CTX_DV, iniFile));
    _loadIntoCurrent = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_LIC, INI_SES_LIC_DV, iniFile));
    _loadWithoutClosing = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_LWC, INI_SES_LWC_DV, iniFile));
    _showInStatusbar = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_SISB, INI_SES_SISB_DV, iniFile));
    _showInTitlebar = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_SITB, INI_SES_SITB_DV, iniFile));

    // integer properties
    _saveDelay = ::GetPrivateProfileIntW(INI_SESSION, INI_SES_SVD, INI_SES_SVD_DV, iniFile);
    _sortOrder = ::GetPrivateProfileIntW(INI_SESSION, INI_SES_SORT, INI_SES_SORT_DV, iniFile);
    debug = ::GetPrivateProfileIntW(INI_DEBUG, INI_DBG_DBG, INI_DBG_DBG_DV, iniFile);

    logFile[0] = 0;
    if (debug) {
        ::GetPrivateProfileStringW(INI_DEBUG, INI_DBG_LOG_FILE, EMPTY_STR, logFile, MAX_PATH, iniFile);
    }
}

/** Writes current gCfg values to the ini file. */
bool Config::save()
{
    WCHAR buf[TMP_BUF_LEN + 1];
    LPWSTR iniFile = sys_getIniFile();

    if (saveFilters()) {
        ::_itow_s((INT)_autoSave, buf, TMP_BUF_LEN, 10);
        if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_ASV, buf, iniFile)) {
            ::_itow_s((INT)_autoLoad, buf, TMP_BUF_LEN, 10);
            if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_ALD, buf, iniFile)) {
                ::_itow_s((INT)_globalBookmarks, buf, TMP_BUF_LEN, 10);
                if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_GBM, buf, iniFile)) {
                    ::_itow_s((INT)_useContextMenu, buf, TMP_BUF_LEN, 10);
                    if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_CTX, buf, iniFile)) {
                        ::_itow_s((INT)_loadIntoCurrent, buf, TMP_BUF_LEN, 10);
                        if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_LIC, buf, iniFile)) {
                            ::_itow_s((INT)_loadWithoutClosing, buf, TMP_BUF_LEN, 10);
                            if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_LWC, buf, iniFile)) {
                                ::_itow_s((INT)_sortOrder, buf, TMP_BUF_LEN, 10);
                                if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_SORT, buf, iniFile)) {
                                    ::_itow_s((INT)_showInStatusbar, buf, TMP_BUF_LEN, 10);
                                    if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_SISB, buf, iniFile)) {
                                        ::_itow_s((INT)_showInTitlebar, buf, TMP_BUF_LEN, 10);
                                        if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_SITB, buf, iniFile)) {
                                            if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_DIR, _directory, iniFile)) {
                                                if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_EXT, _extension, iniFile)) {
                                                    return true;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    DWORD le = ::GetLastError();
    msg::error(le, L"%s: Error writing to settings file \"%s\".", _W(__FUNCTION__), iniFile);
    return false;
}

//------------------------------------------------------------------------------

/** Reads current session name from ini file into s. */
void Config::readCurrentName(LPWSTR s)
{
    ::GetPrivateProfileStringW(INI_SESSION, INI_SES_CUR, _defaultName, s, MAX_PATH, sys_getIniFile());
}

/** Writes current session name s to ini file. */
BOOL Config::saveCurrentName(LPWSTR s)
{
    BOOL status = ::WritePrivateProfileStringW(INI_SESSION, INI_SES_CUR, s, sys_getIniFile());
    if (status == 0) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error writing current to settings file.", _W(__FUNCTION__));
    }
    return status;
}

/** Reads previous session name from ini file into s. */
void Config::readPreviousName(LPWSTR s)
{
    ::GetPrivateProfileStringW(INI_SESSION, INI_SES_PRV, _defaultName, s, MAX_PATH, sys_getIniFile());
}

/** Writes previous session name s to ini file. */
BOOL Config::savePreviousName(LPWSTR s)
{
    BOOL status = ::WritePrivateProfileStringW(INI_SESSION, INI_SES_PRV, s, sys_getIniFile());
    if (status == 0) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error writing previous to settings file.", _W(__FUNCTION__));
    }
    return status;
}

/** Writes default session name s to ini file. */
BOOL Config::saveDefaultName(LPWSTR s)
{
    ::StringCchCopyW(_defaultName, SES_NAME_BUF_LEN, s);
    BOOL status = ::WritePrivateProfileStringW(INI_SESSION, INI_SES_DEF, _defaultName, sys_getIniFile());
    if (status == 0) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error writing default to settings file.", _W(__FUNCTION__));
    }
    return status;
}

/** Reads width and height of Sessions dialog from ini file into w and h. */
void Config::readSesDlgSize(INT *w, INT *h)
{
    LPWSTR iniFile = sys_getIniFile();
    *w = ::GetPrivateProfileIntW(INI_DIALOG, INI_DLG_SES_W, INI_DLG_SES_W_DV, iniFile);
    *h = ::GetPrivateProfileIntW(INI_DIALOG, INI_DLG_SES_H, INI_DLG_SES_H_DV, iniFile);
}

/** Writes width and height of Sessions dialog to ini file. */
void Config::saveSesDlgSize(INT w, INT h)
{
    if (saveDlgSize(true, w, h) == FALSE) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error writing to settings file.", _W(__FUNCTION__));
    }
}

/** Reads width and height of Settings dialog from ini file into w and h. */
void Config::readCfgDlgSize(INT *w, INT *h)
{
    LPWSTR iniFile = sys_getIniFile();
    *w = ::GetPrivateProfileIntW(INI_DIALOG, INI_DLG_CFG_W, INI_DLG_CFG_W_DV, iniFile);
    *h = ::GetPrivateProfileIntW(INI_DIALOG, INI_DLG_CFG_H, INI_DLG_CFG_H_DV, iniFile);
}

/** Writes width and height of Settings dialog to ini file. */
void Config::saveCfgDlgSize(INT w, INT h)
{
    if (saveDlgSize(false, w, h) == FALSE) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error writing to settings file.", _W(__FUNCTION__));
    }
}

/** Writes width and height of Sessions or Settings dialog to ini file. */
BOOL Config::saveDlgSize(bool ses, INT w, INT h)
{
    WCHAR
        buf[TMP_BUF_LEN + 1],
        *iniFile = sys_getIniFile(),
        *wProp = INI_DLG_CFG_W,
        *hProp = INI_DLG_CFG_H;

    if (ses) {
        wProp = INI_DLG_SES_W;
        hProp = INI_DLG_SES_H;
    }
    ::_itow_s(w, buf, TMP_BUF_LEN, 10);
    BOOL status = ::WritePrivateProfileStringW(INI_DIALOG, wProp, buf, iniFile);
    if (status != FALSE) {
        ::_itow_s(h, buf, TMP_BUF_LEN, 10);
        status = ::WritePrivateProfileStringW(INI_DIALOG, hProp, buf, iniFile);
    }
    return status;
}

BOOL Config::saveSortOrder(INT order)
{
    WCHAR buf[TMP_BUF_LEN + 1];
    _sortOrder = order;
    ::_itow_s((INT)_sortOrder, buf, TMP_BUF_LEN, 10);
    return ::WritePrivateProfileStringW(INI_SESSION, INI_SES_SORT, buf, sys_getIniFile());
}

//------------------------------------------------------------------------------

void Config::setShowInStatusbar(bool v)
{
    _showInStatusbar = v;
    if (v) {
        app_showSessionInNppBars();
    }
}

void Config::setShowInTitlebar(bool v)
{
    _showInTitlebar = v;
    if (v) {
        app_showSessionInNppBars();
    }
}

bool Config::setSesDir(LPWSTR p)
{
    WCHAR buf[MAX_PATH];
    if (!p || !*p) {
        ::StringCchCopyW(buf, MAX_PATH, sys_getCfgDir());
        ::StringCchCatW(buf, MAX_PATH, DEFAULT_SES_DIR);
    }
    else {
        ::StringCchCopyW(buf, MAX_PATH, p);
        pth::appendSlash(buf, MAX_PATH);
        if (!pth::dirExists(buf)) {
            if (::SHCreateDirectoryExW(NULL, buf, NULL) != ERROR_SUCCESS ) {
                DWORD le = ::GetLastError();
                msg::error(le, L"%s: Error creating directory \"%s\".", _W(__FUNCTION__), buf);
                return false; // ses dir not changed
            }
        }
    }
    ::StringCchCopyW(_directory, MAX_PATH, buf);
    app_confirmDefaultSession();
    return true;
}

void Config::setSesExt(LPWSTR p)
{
    if (!p || !*p) {
        ::StringCchCopyW(_extension, MAX_PATH, DEFAULT_SES_EXT);
    }
    else {
        _extension[0] = L'\0';
        if (*p != L'.') {
            _extension[0] = L'.';
            _extension[1] = L'\0';
        }
        ::StringCchCatW(_extension, MAX_PATH, p);
    }
    app_confirmDefaultSession();
}

//------------------------------------------------------------------------------

/** Gets the main or the 1-based prpIdx'th sub value from the settings file and
    copies it to buf. */
void Config::getMenuLabel(INT prpIdx, LPWSTR buf)
{
    WCHAR lblBuf[MNU_MAX_NAME_LEN], prpBuf[6], numBuf[3], *iniFile = sys_getIniFile();

    if (prpIdx == -1) {
        ::GetPrivateProfileStringW(INI_MENU, INI_MNU_MAIN, NULL, lblBuf, MNU_MAX_NAME_LEN, iniFile);
        if (lblBuf[0] != 0) {
            ::StringCchCopyW(buf, MNU_MAX_NAME_LEN, lblBuf);
        }
    }
    else if (prpIdx > 0 && prpIdx < MNU_BASE_MAX_ITEMS) {
        ::StringCchCopyW(prpBuf, 6, INI_MNU_SUB_PFX);
        ::_itow_s(prpIdx, numBuf, 3, 10);
        ::StringCchCatW(prpBuf, 6, numBuf);
        ::GetPrivateProfileStringW(INI_MENU, prpBuf, NULL, lblBuf, MNU_MAX_NAME_LEN, iniFile);
        if (lblBuf[0] != 0) {
            ::StringCchCopyW(buf, MNU_MAX_NAME_LEN, lblBuf);
        }
    }
}

/** Gets the 1-based prpIdx'th fav value from the settings file and copies it to buf. */
bool Config::getFavMenuLabel(INT prpIdx, LPWSTR buf)
{
    bool status = false;
    WCHAR lblBuf[MNU_MAX_NAME_LEN], prpBuf[6], numBuf[3], *iniFile = sys_getIniFile();

    if (prpIdx >= 1 && prpIdx <= MNU_MAX_FAVS) {
        ::StringCchCopyW(prpBuf, 6, INI_MNU_FAV_PFX);
        ::_itow_s(prpIdx, numBuf, 3, 10);
        ::StringCchCatW(prpBuf, 6, numBuf);
        ::GetPrivateProfileStringW(INI_MENU, prpBuf, NULL, lblBuf, MNU_MAX_NAME_LEN, iniFile);
        if (lblBuf[0] != 0) {
            ::StringCchCopyW(buf, MNU_MAX_NAME_LEN, lblBuf);
            status = true;
        }
    }
    return status;
}

/** Deletes all favorites from the settings file. */
void Config::deleteFavorites()
{
    WCHAR prpBuf[6], numBuf[3], *iniFile = sys_getIniFile();

    mnu_clearFavorites();
    for (INT prpIdx = 1; prpIdx <= MNU_MAX_FAVS; ++prpIdx) {
        ::StringCchCopyW(prpBuf, 6, INI_MNU_FAV_PFX);
        ::_itow_s(prpIdx, numBuf, 3, 10);
        ::StringCchCatW(prpBuf, 6, numBuf);
        ::WritePrivateProfileStringW(INI_MENU, prpBuf, NULL, iniFile);
    }
}

/** Writes favName to the 1-based prpIdx'th fav property in the settings file. */
void Config::addFavorite(INT prpIdx, LPCWSTR favName)
{
    WCHAR prpBuf[6], numBuf[3];

    mnu_addFavorite(prpIdx, favName);
    ::StringCchCopyW(prpBuf, 6, INI_MNU_FAV_PFX);
    ::_itow_s(prpIdx, numBuf, 3, 10);
    ::StringCchCatW(prpBuf, 6, numBuf);
    ::WritePrivateProfileStringW(INI_MENU, prpBuf, favName, sys_getIniFile());
}

/** Reads the session mark characters from the settings file or uses defaults. */
void Config::loadMarks()
{
    WCHAR *iniFile = sys_getIniFile();
    WCHAR *props[] = {L"currentMark", L"currentFavMark", L"previousMark", L"previousFavMark", L"defaultMark", L"defaultFavMark", L"favoriteMark"};
    WCHAR numStr[7], *defaults[] = {
        L"9674", // white diamond
        L"9830", // black diamond
        L"9702", // white bullet
        L"8226", // black bullet
        L"9653", // white triangle
        L"9652", // black triangle
        L"183"   // middle dot
    };
    for (INT i = 0; i < 7; ++i) {
        ::GetPrivateProfileStringW(INI_SESSION, props[i], NULL, numStr, 7, iniFile);
        if (numStr[0] == 0) {
            ::StringCchCopyW(numStr, 7, defaults[i]);
        }
        markChars[i][0] = ::_wtoi(numStr);
        markChars[i][1] = L'\t';
        markChars[i][2] = 0;
    }
}

//------------------------------------------------------------------------------

/** SessionFilter constructor. */
SessionFilter::SessionFilter(LPCWSTR filter)
{
    ::StringCchCopyW(exp, FILTER_BUF_LEN, filter);
}

/** Reads filters from the settings file. */
void Config::loadFilters()
{
    INT i;
    WCHAR expBuf[FILTER_BUF_LEN], prpBuf[6], numBuf[3], *iniFile = sys_getIniFile();

    for (i = 1; i < FILTERS_MAX; ++i) {
        ::StringCchCopyW(prpBuf, 6, INI_FIL_EXP_PFX);
        ::_itow_s(i, numBuf, 3, 10);
        ::StringCchCatW(prpBuf, 6, numBuf);
        ::GetPrivateProfileStringW(INI_FILTER, prpBuf, NULL, expBuf, FILTER_BUF_LEN, iniFile);
        if (expBuf[0] == 0) {
            break;
        }
        SessionFilter fil(expBuf);
        _filters.push_back(fil);
    }
    if (_filters.empty()) {
        SessionFilter fil(L"*");
        _filters.push_back(fil);
    }
}

/** Writes filters to the settings file.
    @return TRUE on success else FALSE */
BOOL Config::saveFilters()
{
    INT i = 1;
    BOOL status = TRUE;
    WCHAR prpBuf[6], numBuf[3], *iniFile = sys_getIniFile();

    for (list<SessionFilter>::const_iterator it = _filters.begin(); it != _filters.end(); ++it) {
        ::StringCchCopyW(prpBuf, FILTER_BUF_LEN, INI_FIL_EXP_PFX);
        ::_itow_s(i, numBuf, 3, 10);
        ::StringCchCatW(prpBuf, FILTER_BUF_LEN, numBuf);
        status = ::WritePrivateProfileStringW(INI_FILTER, prpBuf, it->exp, iniFile);
        if (status == FALSE) {
            break;
        }
        ++i;
    }
    return status;
}

/** @return the filter string at the 1-based index position in the list */
LPCWSTR Config::getFilter(INT index)
{
    INT i = 1;
    for (list<SessionFilter>::const_iterator it = _filters.begin(); it != _filters.end(); ++it) {
        if (i == index) {
            return it->exp;
        }
        ++i;
    }
    return NULL;
}

/** Adds a filter to the list then saves the list to the settings file. If
    filter is already in the list it is removed, unless it is at the top of
    the list, and the new filter is added at the top. */
void Config::addFilter(LPCWSTR filter)
{
    INT i = 1;
    if (filter && *filter) {
        for (list<SessionFilter>::iterator it = _filters.begin(); it != _filters.end(); ++it) {
            if (::wcscmp(it->exp, filter) == 0) {
                if (i == 1) {
                    return; // filter is already at the top of the list
                }
                _filters.erase(it);
                break;
            }
            ++i;
        }
        SessionFilter fil(filter);
        _filters.push_front(fil);
        saveFilters();
    }
}

} // end namespace NppPlugin

