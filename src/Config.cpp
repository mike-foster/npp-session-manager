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
    @copyright Copyright 2011-2014 Michael Foster <http://mfoster.com/npp/>

    The configuration (settings) object.
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
#define INI_SES_LIC L"loadIntoCurrent"
#define INI_SES_LIC_DV 0
#define INI_SES_LWC L"loadWithoutClosing"
#define INI_SES_LWC_DV 0
#define INI_SES_SORT L"sortOrder"
#define INI_SES_SORT_DV 0
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
#define INI_SES_CUR_DV EMPTY_STR
#define INI_SES_PRV L"previous"
#define INI_SES_PRV_DV EMPTY_STR

#define INI_MENU L"menu"
#define INI_MNU_MAIN L"main"
#define INI_MNU_SUB1 L"sub1"
#define INI_MNU_SUB2 L"sub2"
#define INI_MNU_SUB3 L"sub3"
#define INI_MNU_SUB4 L"sub4"
#define INI_MNU_SUB5 L"sub5"
#define INI_MNU_SUB6 L"sub6"

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
#define DEFAULT_INI_CONTENTS "[session]\nautoSave=1\nautoLoad=0\nglobalBookmarks=1\nloadIntoCurrent=0\nloadWithoutClosing=0\nsortOrder=1\nshowInTitlebar=0\nshowInStatusbar=0\nsaveDelay=3\ndirectory=\nextension=\ncurrent=\nprevious=\n\n[menu]\nitem1=\nitem2=\nitem3=\nitem4=\nitem5=\nitem6=\n\n[dialog]\nsessionsW=0\nsessionsH=0\nsettingsW=0\nsettingsH=0\n\n[debug]\ndebug=0\nlogFile=\n"

} // end namespace

//------------------------------------------------------------------------------

/** Reads properties from the ini file. */
void Config::load()
{
    LPWSTR iniFile = sys_getIniFile();
    pth::createFileIfMissing(iniFile, DEFAULT_INI_CONTENTS);

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

    // menu item names
    ::GetPrivateProfileStringW(INI_MENU, INI_MNU_MAIN, EMPTY_STR, _menuMainLabel, MNU_MAX_NAME_LEN, iniFile);
    ::GetPrivateProfileStringW(INI_MENU, INI_MNU_SUB1, EMPTY_STR, _menuSubLabels[0], MNU_MAX_NAME_LEN, iniFile);
    ::GetPrivateProfileStringW(INI_MENU, INI_MNU_SUB2, EMPTY_STR, _menuSubLabels[1], MNU_MAX_NAME_LEN, iniFile);
    ::GetPrivateProfileStringW(INI_MENU, INI_MNU_SUB3, EMPTY_STR, _menuSubLabels[2], MNU_MAX_NAME_LEN, iniFile);
    ::GetPrivateProfileStringW(INI_MENU, INI_MNU_SUB4, EMPTY_STR, _menuSubLabels[3], MNU_MAX_NAME_LEN, iniFile);
    _menuSubLabels[4][0] = 0;
    ::GetPrivateProfileStringW(INI_MENU, INI_MNU_SUB5, EMPTY_STR, _menuSubLabels[5], MNU_MAX_NAME_LEN, iniFile);
    ::GetPrivateProfileStringW(INI_MENU, INI_MNU_SUB6, EMPTY_STR, _menuSubLabels[6], MNU_MAX_NAME_LEN, iniFile);

    // boolean properties
    _autoSave = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_ASV, INI_SES_ASV_DV, iniFile));
    _autoLoad = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_ALD, INI_SES_ALD_DV, iniFile));
    _globalBookmarks = uintToBool(::GetPrivateProfileIntW(INI_SESSION, INI_SES_GBM, INI_SES_GBM_DV, iniFile));
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

    ::_itow_s((INT)_autoSave, buf, TMP_BUF_LEN, 10);
    if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_ASV, buf, iniFile)) {
        ::_itow_s((INT)_autoLoad, buf, TMP_BUF_LEN, 10);
        if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_ALD, buf, iniFile)) {
            ::_itow_s((INT)_globalBookmarks, buf, TMP_BUF_LEN, 10);
            if (::WritePrivateProfileStringW(INI_SESSION, INI_SES_GBM, buf, iniFile)) {
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
    DWORD le = ::GetLastError();
    msg::error(le, L"%s: Error writing to settings file \"%s\".", _W(__FUNCTION__), iniFile);
    return false;
}

//------------------------------------------------------------------------------

/** Reads current session name from ini file into s. */
void Config::readCurrent(LPWSTR s)
{
    ::GetPrivateProfileStringW(INI_SESSION, INI_SES_CUR, INI_SES_CUR_DV, s, MAX_PATH, sys_getIniFile());
}

/** Writes current session name s to ini file. */
BOOL Config::saveCurrent(LPWSTR s)
{
    BOOL status = ::WritePrivateProfileStringW(INI_SESSION, INI_SES_CUR, s, sys_getIniFile());
    if (status == 0) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error writing to settings file.", _W(__FUNCTION__));
    }
    return status;
}

/** Reads previous session name from ini file into s. */
void Config::readPrevious(LPWSTR s)
{
    ::GetPrivateProfileStringW(INI_SESSION, INI_SES_PRV, INI_SES_PRV_DV, s, MAX_PATH, sys_getIniFile());
}

/** Writes previous session name s to ini file. */
BOOL Config::savePrevious(LPWSTR s)
{
    BOOL status = ::WritePrivateProfileStringW(INI_SESSION, INI_SES_PRV, s, sys_getIniFile());
    if (status == 0) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error writing to settings file.", _W(__FUNCTION__));
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
}

void Config::setSaveDelay(LPWSTR p)
{
    _saveDelay = ::_wtoi(p);
    if (_saveDelay == 0) {
        _saveDelay = INI_SES_SVD_DV;
    }
}

void Config::getSaveDelay(LPWSTR buf, INT len)
{
    ::_itow_s(_saveDelay, buf, len, 10);
}

void Config::getMenuLabel(int idx, LPWSTR buf)
{
    if (idx == -1) {
        if (_menuMainLabel[0] != 0) {
            ::StringCchCopyW(buf, MNU_MAX_NAME_LEN, _menuMainLabel);
        }
    }
    else if (idx >= 0 && idx < MNU_MAX_ITEMS && _menuSubLabels[idx][0] != 0) {
        ::StringCchCopyW(buf, MNU_MAX_NAME_LEN, _menuSubLabels[idx]);
    }
}

} // end namespace NppPlugin

