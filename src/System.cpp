/*
    System.cpp
    Copyright 2011,2014 Michael Foster (http://mfoster.com/npp/)

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

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

#define INI_FILE_NAME _T("settings.ini")
#define HELP_FILE_NAME PLUGIN_DLL_NAME _T(".html")
#define PROPS_FILE_NAME _T("file-properties.xml")
#define PROPS_DEFAULT_CONTENT "<NotepadPlus><FileProperties></FileProperties></NotepadPlus>\r\n"

HWND _hNpp;
HWND _hSci1;
HWND _hSci2;
HINSTANCE _hDll;
UINT _nppVersion;
TCHAR _cfgDir[MAX_PATH_P1]; // includes trailing slash
TCHAR _iniFile[MAX_PATH_P1];
TCHAR _helpFile[MAX_PATH_P1];
TCHAR _defSesFile[MAX_PATH_P1];
char _propsFile[MAX_PATH_T2_P1];

} // end namespace

//------------------------------------------------------------------------------
// The api namespace contains functions called only from DllMain.

namespace api {

void sys_onLoad(HINSTANCE hDLLInstance)
{
    _hDll = hDLLInstance;
    // Make full pathname of plugin help file
    GetModuleFileName((HMODULE)_hDll, _helpFile, MAX_PATH);
    pth::remName(_helpFile);
    StringCchCat(_helpFile, MAX_PATH, _T("doc\\"));
    StringCchCat(_helpFile, MAX_PATH, HELP_FILE_NAME);
}

void sys_onUnload()
{
}

void sys_init(NppData nppd)
{
    _propsFile[0] = 0;

    // Save NPP window handles
    _hNpp = nppd._nppHandle;
    _hSci1 = nppd._scintillaMainHandle;
    _hSci2 = nppd._scintillaSecondHandle;

    // Get Npp version to verify plugin compatibility
    _nppVersion = SendMessage(_hNpp, NPPM_GETNPPVERSION, 0, 0);
    //if () // TODO

    // Get plugin config directory from NPP.
    SendMessage(_hNpp, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)_cfgDir);
    // Get SessionMgr config directory and create it if not present.
    pth::addSlash(_cfgDir);
    StringCchCat(_cfgDir, MAX_PATH, PLUGIN_DLL_NAME);
    StringCchCat(_cfgDir, MAX_PATH, _T("\\"));
    CreateDirectory(_cfgDir, NULL);

    // Get ini file pathname.
    StringCchCopy(_iniFile, MAX_PATH, _cfgDir);
    StringCchCat(_iniFile, MAX_PATH, INI_FILE_NAME);
    // Load the config file.
    gCfg.load();
    // Create sessions directory if it doesn't exist.
    CreateDirectory(gCfg.getSesDir(), NULL);

    /* XXX This was just an experiment. NPP doesn't support a command-line
    option dedicated to plugins, but it would be useful if it did.
    dbgLog(__FUNCTION__ ": cmdLine = '%S'", GetCommandLine()); */
}

} // end namespace api

//------------------------------------------------------------------------------

TCHAR* sys_getCfgDir()
{
    return _cfgDir;
}

TCHAR* sys_getIniFile()
{
    return _iniFile;
}

TCHAR* sys_getHelpFile()
{
    return _helpFile;
}

TCHAR* sys_getDefSesFile()
{
    // Create default session if not present.
    StringCchCopy(_defSesFile, MAX_PATH, _cfgDir);
    StringCchCat(_defSesFile, MAX_PATH, SES_DEFAULT_NAME);
    StringCchCat(_defSesFile, MAX_PATH, gCfg.getSesExt());
    createIfNotPresent(_defSesFile, SES_DEFAULT_CONTENTS);
    return _defSesFile;
}

char* sys_getPropsFile()
{
    if (!_propsFile[0]) {
        size_t num;
        TCHAR buf[MAX_PATH_T2_P1];
        StringCchCopy(buf, MAX_PATH_T2, _cfgDir);
        StringCchCat(buf, MAX_PATH_T2, PROPS_FILE_NAME);
        wcstombs_s(&num, _propsFile, MAX_PATH_T2, buf, _TRUNCATE);
        createIfNotPresent(buf, PROPS_DEFAULT_CONTENT);
    }

    return _propsFile;
}

HINSTANCE sys_getDllHwnd()
{
    return _hDll;
}

HWND sys_getNppHwnd()
{
    return _hNpp;
}

HWND sys_getSc1Hwnd()
{
    return _hSci1;
}

HWND sys_getSc2Hwnd()
{
    return _hSci2;
}

} // end namespace NppPlugin

