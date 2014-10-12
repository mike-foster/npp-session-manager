/*
    Config.cpp
    Copyright 2011-2014 Michael Foster (http://mfoster.com/npp/)

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
#include "Util.h"
#include <strsafe.h>
#include <shlobj.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

Config gCfg; // The global configuration/settings object.

//------------------------------------------------------------------------------

namespace {

#define INI_SESSION _T("session")
#define INI_SES_ASV _T("autoSave")
#define INI_SES_ASV_DV 1
#define INI_SES_ALD _T("autoLoad")
#define INI_SES_ALD_DV 0
#define INI_SES_GBM _T("globalBookmarks")
#define INI_SES_GBM_DV 1
#define INI_SES_LIC _T("loadIntoCurrent")
#define INI_SES_LIC_DV 0
#define INI_SES_LWC _T("loadWithoutClosing")
#define INI_SES_LWC_DV 0
#define INI_SES_SORT _T("sortOrder")
#define INI_SES_SORT_DV 0
#define INI_SES_SISB _T("showInStatusbar")
#define INI_SES_SISB_DV 0
#define INI_SES_SITB _T("showInTitlebar")
#define INI_SES_SITB_DV 0
#define INI_SES_SVD _T("saveDelay")
#define INI_SES_SVD_DV 3
#define INI_SES_DIR _T("directory")
#define INI_SES_DIR_DV EMPTY_STR
#define INI_SES_EXT _T("extension")
#define INI_SES_EXT_DV EMPTY_STR
#define INI_SES_CUR _T("current")
#define INI_SES_CUR_DV EMPTY_STR
#define INI_SES_PRV _T("previous")
#define INI_SES_PRV_DV EMPTY_STR

#define INI_MENU _T("menu")
#define INI_MNU_MAIN _T("main")
#define INI_MNU_SUB1 _T("sub1")
#define INI_MNU_SUB2 _T("sub2")
#define INI_MNU_SUB3 _T("sub3")
#define INI_MNU_SUB4 _T("sub4")
#define INI_MNU_SUB5 _T("sub5")
#define INI_MNU_SUB6 _T("sub6")

#define INI_DIALOG _T("dialog")
#define INI_DLG_SES_W _T("sessionsW")
#define INI_DLG_SES_W_DV 0
#define INI_DLG_SES_H _T("sessionsH")
#define INI_DLG_SES_H_DV 0
#define INI_DLG_CFG_W _T("settingsW")
#define INI_DLG_CFG_W_DV 0
#define INI_DLG_CFG_H _T("settingsH")
#define INI_DLG_CFG_H_DV 0

#define INI_DEBUG _T("debug")
#define INI_DBG_DBG _T("debug")
#define INI_DBG_DBG_DV 0
#define INI_DBG_LOG_FILE _T("logFile")

#define TMP_BUF_LEN 30
#define DEFAULT_SES_DIR _T("sessions\\")
#define DEFAULT_SES_EXT _T(".npp-session")
#define DEFAULT_INI_CONTENTS "[session]\r\nautoSave=1\r\nautoLoad=0\r\nglobalBookmarks=1\r\nloadIntoCurrent=0\r\nloadWithoutClosing=0\r\nsortOrder=1\r\nshowInTitlebar=0\r\nshowInStatusbar=0\r\nsaveDelay=3\r\ndirectory=\r\nextension=\r\ncurrent=\r\nprevious=\r\n\r\n[menu]\r\nitem1=\r\nitem2=\r\nitem3=\r\nitem4=\r\nitem5=\r\nitem6=\r\n\r\n[dialog]\r\nsessionsW=0\r\nsessionsH=0\r\nsettingsW=0\r\nsettingsH=0\r\n\r\n[debug]\r\ndebug=0\r\nlogFile=\r\n"

} // end namespace

//------------------------------------------------------------------------------

/* Reads properties from the ini file. */
void Config::load()
{
    TCHAR *iniFile = sys_getIniFile();
    createIfNotPresent(iniFile, DEFAULT_INI_CONTENTS);

    // session directory property
    _directory[0] = 0;
    GetPrivateProfileString(INI_SESSION, INI_SES_DIR, INI_SES_DIR_DV, _directory, MAX_PATH, iniFile);
    if (_directory[0] == 0) {
        StringCchCopy(_directory, MAX_PATH, sys_getCfgDir());
        StringCchCat(_directory, MAX_PATH, DEFAULT_SES_DIR);
    }
    pth::addSlash(_directory);

    // session extension property
    _extension[0] = 0;
    GetPrivateProfileString(INI_SESSION, INI_SES_EXT, INI_SES_EXT_DV, _extension, MAX_PATH, iniFile);
    if (_extension[0] == 0) {
        StringCchCopy(_extension, MAX_PATH, DEFAULT_SES_EXT);
    }

    // menu item names
    GetPrivateProfileString(INI_MENU, INI_MNU_MAIN, EMPTY_STR, _menuMainLabel, MNU_MAX_NAME_LEN, iniFile);
    GetPrivateProfileString(INI_MENU, INI_MNU_SUB1, EMPTY_STR, _menuSubLabels[0], MNU_MAX_NAME_LEN, iniFile);
    GetPrivateProfileString(INI_MENU, INI_MNU_SUB2, EMPTY_STR, _menuSubLabels[1], MNU_MAX_NAME_LEN, iniFile);
    GetPrivateProfileString(INI_MENU, INI_MNU_SUB3, EMPTY_STR, _menuSubLabels[2], MNU_MAX_NAME_LEN, iniFile);
    GetPrivateProfileString(INI_MENU, INI_MNU_SUB4, EMPTY_STR, _menuSubLabels[3], MNU_MAX_NAME_LEN, iniFile);
    _menuSubLabels[4][0] = 0;
    GetPrivateProfileString(INI_MENU, INI_MNU_SUB5, EMPTY_STR, _menuSubLabels[5], MNU_MAX_NAME_LEN, iniFile);
    GetPrivateProfileString(INI_MENU, INI_MNU_SUB6, EMPTY_STR, _menuSubLabels[6], MNU_MAX_NAME_LEN, iniFile);

    // boolean properties
    _autoSave = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_ASV, INI_SES_ASV_DV, iniFile));
    _autoLoad = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_ALD, INI_SES_ALD_DV, iniFile));
    _globalBookmarks = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_GBM, INI_SES_GBM_DV, iniFile));
    _loadIntoCurrent = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_LIC, INI_SES_LIC_DV, iniFile));
    _loadWithoutClosing = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_LWC, INI_SES_LWC_DV, iniFile));
    _showInStatusbar = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_SISB, INI_SES_SISB_DV, iniFile));
    _showInTitlebar = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_SITB, INI_SES_SITB_DV, iniFile));

    // integer properties
    _saveDelay = GetPrivateProfileInt(INI_SESSION, INI_SES_SVD, INI_SES_SVD_DV, iniFile);
    _sortOrder = GetPrivateProfileInt(INI_SESSION, INI_SES_SORT, INI_SES_SORT_DV, iniFile);
    debug = GetPrivateProfileInt(INI_DEBUG, INI_DBG_DBG, INI_DBG_DBG_DV, iniFile);

    logFile[0] = 0;
    if (debug) {
        size_t num;
        TCHAR buf[MAX_PATH_T2_P1];
        GetPrivateProfileString(INI_DEBUG, INI_DBG_LOG_FILE, EMPTY_STR, buf, MAX_PATH_T2, iniFile);
        wcstombs_s(&num, logFile, MAX_PATH_T2, buf, _TRUNCATE);
    }
}

/* Writes current gCfg values to the ini file. */
bool Config::save()
{
    TCHAR buf[TMP_BUF_LEN + 1];
    TCHAR *iniFile = sys_getIniFile();

    _itot_s((INT)_autoSave, buf, TMP_BUF_LEN, 10);
    if (WritePrivateProfileString(INI_SESSION, INI_SES_ASV, buf, iniFile)) {
        _itot_s((INT)_autoLoad, buf, TMP_BUF_LEN, 10);
        if (WritePrivateProfileString(INI_SESSION, INI_SES_ALD, buf, iniFile)) {
            _itot_s((INT)_globalBookmarks, buf, TMP_BUF_LEN, 10);
            if (WritePrivateProfileString(INI_SESSION, INI_SES_GBM, buf, iniFile)) {
                _itot_s((INT)_loadIntoCurrent, buf, TMP_BUF_LEN, 10);
                if (WritePrivateProfileString(INI_SESSION, INI_SES_LIC, buf, iniFile)) {
                    _itot_s((INT)_loadWithoutClosing, buf, TMP_BUF_LEN, 10);
                    if (WritePrivateProfileString(INI_SESSION, INI_SES_LWC, buf, iniFile)) {
                        _itot_s((INT)_sortOrder, buf, TMP_BUF_LEN, 10);
                        if (WritePrivateProfileString(INI_SESSION, INI_SES_SORT, buf, iniFile)) {
                            _itot_s((INT)_showInStatusbar, buf, TMP_BUF_LEN, 10);
                            if (WritePrivateProfileString(INI_SESSION, INI_SES_SISB, buf, iniFile)) {
                                _itot_s((INT)_showInTitlebar, buf, TMP_BUF_LEN, 10);
                                if (WritePrivateProfileString(INI_SESSION, INI_SES_SITB, buf, iniFile)) {
                                    if (WritePrivateProfileString(INI_SESSION, INI_SES_DIR, _directory, iniFile)) {
                                        if (WritePrivateProfileString(INI_SESSION, INI_SES_EXT, _extension, iniFile)) {
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
    SHOW_ERROR;
    return false;
}

//------------------------------------------------------------------------------

/* Reads current session name from ini file into s. */
void Config::readCurrent(TCHAR *s)
{
    GetPrivateProfileString(INI_SESSION, INI_SES_CUR, INI_SES_CUR_DV, s, MAX_PATH, sys_getIniFile());
}

/* Writes current session name s to ini file. */
BOOL Config::saveCurrent(TCHAR *s)
{
    BOOL status = WritePrivateProfileString(INI_SESSION, INI_SES_CUR, s, sys_getIniFile());
    if (status == 0) {
        SHOW_ERROR;
    }
    return status;
}

/* Reads previous session name from ini file into s. */
void Config::readPrevious(TCHAR *s)
{
    GetPrivateProfileString(INI_SESSION, INI_SES_PRV, INI_SES_PRV_DV, s, MAX_PATH, sys_getIniFile());
}

/* Writes previous session name s to ini file. */
BOOL Config::savePrevious(TCHAR *s)
{
    BOOL status = WritePrivateProfileString(INI_SESSION, INI_SES_PRV, s, sys_getIniFile());
    if (status == 0) {
        SHOW_ERROR;
    }
    return status;
}

/* Reads width and height of Sessions dialog from ini file into w and h. */
void Config::readSesDlgSize(INT *w, INT *h)
{
    TCHAR *iniFile = sys_getIniFile();
    *w = GetPrivateProfileInt(INI_DIALOG, INI_DLG_SES_W, INI_DLG_SES_W_DV, iniFile);
    *h = GetPrivateProfileInt(INI_DIALOG, INI_DLG_SES_H, INI_DLG_SES_H_DV, iniFile);
}

/* Writes width and height of Sessions dialog to ini file. */
void Config::saveSesDlgSize(INT w, INT h)
{
    if (saveDlgSize(true, w, h) == FALSE) {
        SHOW_ERROR;
    }
}

/* Reads width and height of Settings dialog from ini file into w and h. */
void Config::readCfgDlgSize(INT *w, INT *h)
{
    TCHAR *iniFile = sys_getIniFile();
    *w = GetPrivateProfileInt(INI_DIALOG, INI_DLG_CFG_W, INI_DLG_CFG_W_DV, iniFile);
    *h = GetPrivateProfileInt(INI_DIALOG, INI_DLG_CFG_H, INI_DLG_CFG_H_DV, iniFile);
}

/* Writes width and height of Settings dialog to ini file. */
void Config::saveCfgDlgSize(INT w, INT h)
{
    if (saveDlgSize(false, w, h) == FALSE) {
        SHOW_ERROR;
    }
}

/* Writes width and height of Sessions or Settings dialog to ini file. */
BOOL Config::saveDlgSize(bool ses, INT w, INT h)
{
    TCHAR
        buf[TMP_BUF_LEN + 1],
        *iniFile = sys_getIniFile(),
        *wProp = INI_DLG_CFG_W,
        *hProp = INI_DLG_CFG_H;

    if (ses) {
        wProp = INI_DLG_SES_W;
        hProp = INI_DLG_SES_H;
    }
    _itot_s(w, buf, TMP_BUF_LEN, 10);
    BOOL status = WritePrivateProfileString(INI_DIALOG, wProp, buf, iniFile);
    if (status != FALSE) {
        _itot_s(h, buf, TMP_BUF_LEN, 10);
        status = WritePrivateProfileString(INI_DIALOG, hProp, buf, iniFile);
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

bool Config::setSesDir(TCHAR *p)
{
    TCHAR buf[MAX_PATH_P1];
    if (!p || !*p) {
        StringCchCopy(buf, MAX_PATH, sys_getCfgDir());
        StringCchCat(buf, MAX_PATH, DEFAULT_SES_DIR);
    }
    else {
        StringCchCopy(buf, MAX_PATH, p);
        pth::addSlash(buf);
        if (!pth::dirExists(buf)) {
            if (SHCreateDirectoryEx(NULL, buf, NULL) != ERROR_SUCCESS ) {
                SHOW_ERROR;
                return false; // ses dir not changed
            }
        }
    }
    StringCchCopy(_directory, MAX_PATH, buf);
    return true;
}

void Config::setSesExt(TCHAR *p)
{
    if (!p || !*p) {
        StringCchCopy(_extension, MAX_PATH, DEFAULT_SES_EXT);
    }
    else {
        _extension[0] = _T('\0');
        if (*p != _T('.')) {
            _extension[0] = _T('.');
            _extension[1] = _T('\0');
        }
        StringCchCat(_extension, MAX_PATH, p);
    }
}

void Config::setSaveDelay(TCHAR *p)
{
    _saveDelay = _tstoi(p);
    if (_saveDelay == 0) {
        _saveDelay = INI_SES_SVD_DV;
    }
}

void Config::getSaveDelay(TCHAR *buf, INT len)
{
    _itot_s(_saveDelay, buf, len, 10);
}

void Config::getMenuLabel(int idx, TCHAR *buf)
{
    if (idx == -1) {
        if (_menuMainLabel[0] != 0) {
            StringCchCopy(buf, MNU_MAX_NAME_LEN, _menuMainLabel);
        }
    }
    else if (idx >= 0 && idx < MNU_MAX_ITEMS && _menuSubLabels[idx][0] != 0) {
        StringCchCopy(buf, MNU_MAX_NAME_LEN, _menuSubLabels[idx]);
    }
}

} // end namespace NppPlugin

