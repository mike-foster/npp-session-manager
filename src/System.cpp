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
    @file      System.cpp
    @copyright Copyright 2011,2014 Michael Foster <http://mfoster.com/npp/>
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

#define INI_FILE_NAME L"settings.ini"
#define PROPS_FILE_NAME L"global.xml"
#define PROPS_DEFAULT_CONTENT "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<NotepadPlus><FileProperties></FileProperties></NotepadPlus>\n"

HWND _hNpp;
HWND _hSci1;
HWND _hSci2;
HANDLE _hHeap;
HINSTANCE _hDll;
//UINT _nppVersion;
LPWSTR _cfgDir;    ///< SessionMgr's config directory, includes trailing slash
LPWSTR _iniFile;   ///< pathname of settings.ini
LPWSTR _ctxFile;   ///< pathname of NPP's contextMenu.xml file
LPWSTR _propsFile; ///< pathname of global.xml

} // end namespace

//------------------------------------------------------------------------------

namespace api {

void sys_onLoad(HINSTANCE hDLLInstance)
{
    _hDll = hDLLInstance;
}

void sys_onUnload()
{
    sys_free(_cfgDir);
    sys_free(_iniFile);
    sys_free(_ctxFile);
    sys_free(_propsFile);
}

void sys_init(NppData nppd)
{
    // Save NPP handles
    _hNpp = nppd._nppHandle;
    _hSci1 = nppd._scintillaMainHandle;
    _hSci2 = nppd._scintillaSecondHandle;
    _hHeap = ::GetProcessHeap();

    // Allocate buffers
    _cfgDir = (LPWSTR)sys_alloc(MAX_PATH);
    _iniFile = (LPWSTR)sys_alloc(MAX_PATH);
    _ctxFile = (LPWSTR)sys_alloc(MAX_PATH);
    _propsFile = (LPWSTR)sys_alloc(MAX_PATH);

    // Get Npp version to verify plugin compatibility
    //_nppVersion = ::SendMessage(_hNpp, NPPM_GETNPPVERSION, 0, 0);
    //if () // TODO?

    // Get NPP's contextMenu.xml pathname.
    ::SendMessage(_hNpp, NPPM_GETNPPDIRECTORY, MAX_PATH, (LPARAM)_ctxFile);
    pth::appendSlash(_ctxFile, MAX_PATH);
    ::StringCchCatW(_ctxFile, MAX_PATH, L"contextMenu.xml");
    // Get plugin config directory from NPP.
    ::SendMessage(_hNpp, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)_cfgDir);
    // Get SessionMgr config directory and create it if not present.
    pth::appendSlash(_cfgDir, MAX_PATH);
    ::StringCchCatW(_cfgDir, MAX_PATH, PLUGIN_DLL_NAME);
    ::StringCchCatW(_cfgDir, MAX_PATH, L"\\");
    ::CreateDirectoryW(_cfgDir, NULL);

    // Get ini file pathname.
    ::StringCchCopyW(_iniFile, MAX_PATH, _cfgDir);
    ::StringCchCatW(_iniFile, MAX_PATH, INI_FILE_NAME);
    // Load the config file.
    gCfg.load();
    // Create sessions directory if it doesn't exist.
    ::CreateDirectoryW(gCfg.getSesDir(), NULL);
    // Create default session if it doesn't exist.
    app_confirmDefaultSession();
}

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

LPWSTR sys_getCfgDir()
{
    return _cfgDir;
}

LPWSTR sys_getIniFile()
{
    return _iniFile;
}

LPWSTR sys_getPropsFile()
{
    if (!_propsFile[0]) {
        ::StringCchCopyW(_propsFile, MAX_PATH, _cfgDir);
        ::StringCchCatW(_propsFile, MAX_PATH, PROPS_FILE_NAME);
        pth::createFileIfMissing(_propsFile, PROPS_DEFAULT_CONTENT);
    }
    return _propsFile;
}

LPCWSTR sys_getContextMenuFile()
{
    return _ctxFile;
}

HINSTANCE sys_getDllHandle()
{
    return _hDll;
}

HWND sys_getNppHandle()
{
    return _hNpp;
}

HWND sys_getSciHandle(INT v)
{
    if (v == 1) return _hSci1;
    else if (v == 2) return _hSci2;
    else return NULL;
}

LPVOID sys_alloc(INT bytes)
{
    LPVOID p = ::HeapAlloc(_hHeap, HEAP_ZERO_MEMORY, bytes);
    if (p == NULL) {
        LOG("Error allocating %i bytes.", bytes);
        msg::show(L"Memory allocation failed.", M_ERR);
    }
    return p;
}

void sys_free(LPVOID p)
{
    if (p) ::HeapFree(_hHeap, 0, p);
}

} // end namespace NppPlugin

