/*
    Config.cpp
    Copyright 2011,2012 Michael Foster (http://mfoster.com/npp/)

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
#define INI_SES_DIR _T("directory")
#define INI_SES_DIR_DV EMPTY_STR
#define INI_SES_EXT _T("extension")
#define INI_SES_EXT_DV EMPTY_STR
#define INI_SES_ASV _T("autoSave")
#define INI_SES_ASV_DV 1
#define INI_SES_ALD _T("autoLoad")
#define INI_SES_ALD_DV 0
#define INI_SES_SVD _T("saveDelay")
#define INI_SES_SVD_DV 3
#define INI_SES_ELIC _T("enableLIC")
#define INI_SES_ELIC_DV 0
#define INI_SES_DLWC _T("disableLWC")
#define INI_SES_DLWC_DV 0
#define INI_SES_CUR _T("current")
#define INI_SES_CUR_DV EMPTY_STR
#define INI_DEBUG _T("debug")
#define INI_DBG_DBG _T("debug")
#define INI_DBG_DBG_DV 0

#define TMP_BUF_LEN 20
#define DEFAULT_SES_DIR _T("sessions\\")
#define DEFAULT_SES_EXT _T(".npp-session")
#define DEFAULT_INI_CONTENTS "[session]\r\ndirectory=\r\nextension=\r\nautoSave=1\r\nautoLoad=0\r\nenableLIC=0\r\ndisableLWC=0\r\nsaveDelay=3\r\ncurrent=\r\n"

} // end namespace

//------------------------------------------------------------------------------

/* Read properties from the ini file. */
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
    // boolean properties
    _autoSave = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_ASV, INI_SES_ASV_DV, iniFile));
    _autoLoad = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_ALD, INI_SES_ALD_DV, iniFile));
    _enableLIC = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_ELIC, INI_SES_ELIC_DV, iniFile));
    _disableLWC = uintToBool(GetPrivateProfileInt(INI_SESSION, INI_SES_DLWC, INI_SES_DLWC_DV, iniFile));
    debug = uintToBool(GetPrivateProfileInt(INI_DEBUG, INI_DBG_DBG, INI_DBG_DBG_DV, iniFile));
    // integer properties
    _saveDelay = GetPrivateProfileInt(INI_SESSION, INI_SES_SVD, INI_SES_SVD_DV, iniFile);
}

/* Write current gCfg values to the ini file. */
bool Config::save()
{
    TCHAR buf[TMP_BUF_LEN + 1];
    TCHAR *iniFile = sys_getIniFile();

    _itot_s((INT)_autoSave, buf, TMP_BUF_LEN, 10);
    if (WritePrivateProfileString(INI_SESSION, INI_SES_ASV, buf, iniFile)) {
        _itot_s((INT)_autoLoad, buf, TMP_BUF_LEN, 10);
        if (WritePrivateProfileString(INI_SESSION, INI_SES_ALD, buf, iniFile)) {
            _itot_s((INT)_enableLIC, buf, TMP_BUF_LEN, 10);
            if (WritePrivateProfileString(INI_SESSION, INI_SES_ELIC, buf, iniFile)) {
                _itot_s((INT)_disableLWC, buf, TMP_BUF_LEN, 10);
                if (WritePrivateProfileString(INI_SESSION, INI_SES_DLWC, buf, iniFile)) {
                    if (WritePrivateProfileString(INI_SESSION, INI_SES_DIR, _directory, iniFile)) {
                        if (WritePrivateProfileString(INI_SESSION, INI_SES_EXT, _extension, iniFile)) {
                            _itot_s(_saveDelay, buf, TMP_BUF_LEN, 10);
                            if (WritePrivateProfileString(INI_SESSION, INI_SES_SVD, buf, iniFile)) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    errBox(_T("Config::save"), GetLastError());
    return false;
}

/* Read current session name from ini file into s. */
void Config::readCurrent(TCHAR *s)
{
    GetPrivateProfileString(INI_SESSION, INI_SES_CUR, INI_SES_CUR_DV, s, MAX_PATH, sys_getIniFile());
}

/* Write current session name s to ini file. */
BOOL Config::saveCurrent(TCHAR *s)
{
    BOOL status = WritePrivateProfileString(INI_SESSION, INI_SES_CUR, s, sys_getIniFile());
    if (status == 0) {
        errBox(_T("Config::saveCurrent"), GetLastError());
    }
    return status;
}

bool Config::setSesDir(TCHAR *p)
{
    TCHAR buf[MAX_PATH_1];
    if (!p || !*p) {
        StringCchCopy(buf, MAX_PATH, sys_getCfgDir());
        StringCchCat(buf, MAX_PATH, DEFAULT_SES_DIR);
    }
    else {
        //msgBox(p, M_DBG); // DEBUG
        StringCchCopy(buf, MAX_PATH, p);
        pth::addSlash(buf);
        if (!pth::dirExists(buf)) {
            if (SHCreateDirectoryEx(NULL, buf, NULL) != ERROR_SUCCESS ) {
                errBox(_T("Config::setSesDir"), GetLastError());
                return false; // ses dir not changed
            }
        }
    }
    StringCchCopy(_directory, MAX_PATH, buf);
    //msgBox(_directory, M_DBG); // DEBUG
    return true;
}

void Config::setSesExt(TCHAR *p)
{
    if (!p || !*p) {
        StringCchCopy(_extension, MAX_PATH, DEFAULT_SES_EXT);
    }
    else {
        //msgBox(p, M_DBG); // DEBUG
        _extension[0] = _T('\0');
        if (*p != _T('.')) {
            _extension[0] = _T('.');
            _extension[1] = _T('\0');
        }
        StringCchCat(_extension, MAX_PATH, p);
        //msgBox(_extension, M_DBG); // DEBUG
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

} // end namespace NppPlugin

