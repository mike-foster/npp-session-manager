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
    @copyright Copyright 2011,2014,2015 Michael Foster <http://mfoster.com/npp/>
*/

#include "System.h"
#include "SessionMgr.h"
#include "Util.h"
#include <strsafe.h>
//#include <shlobj.h> // for findNppCtxMnuFile

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

#define INI_FILE_NAME L"settings.ini"
#define CFG_FILE_NAME L"settings.xml"
#define CTX_FILE_NAME L"contextMenu.xml"
#define GLB_FILE_NAME L"global.xml"
#define GLB_DEFAULT_CONTENT "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<NotepadPlus><FileProperties></FileProperties></NotepadPlus>\n"
#define BAK_DIR_NAME L"backup"
#define BAK_SES_DIR_NAME L"sessions"

HWND _hNpp;
HWND _hSci1;
HWND _hSci2;
HANDLE _hHeap;
HINSTANCE _hDll;
UINT _nppVersion;
UINT _winVersion;
LPWSTR _cfgDir;  ///< SessionMgr's config directory, includes trailing slash
LPWSTR _cfgFile; ///< pathname of settings.xml
LPWSTR _glbFile; ///< pathname of global.xml
LPWSTR _ctxFile; ///< pathname of NPP's contextMenu.xml file

//void findNppCtxMnuFile();
void backupFiles();
void backupConfigFiles(LPCWSTR backupDir);
void backupSessionFiles(LPCWSTR backupDir);

} // end namespace

//------------------------------------------------------------------------------

namespace api {

void sys_onLoad(HINSTANCE hDLLInstance)
{
    _hDll = hDLLInstance;
}

void sys_onUnload()
{
    sys_free(_ctxFile);
    sys_free(_glbFile);
    sys_free(_cfgFile);
    sys_free(_cfgDir);
}

void sys_init(NppData nppd)
{
    // Save NPP handles
    _hNpp = nppd._nppHandle;
    _hSci1 = nppd._scintillaMainHandle;
    _hSci2 = nppd._scintillaSecondHandle;
    _hHeap = ::GetProcessHeap();

    // Allocate buffers
    _cfgDir = (LPWSTR)sys_alloc(MAX_PATH * sizeof WCHAR);
    _cfgFile = (LPWSTR)sys_alloc(MAX_PATH * sizeof WCHAR);
    _glbFile = (LPWSTR)sys_alloc(MAX_PATH * sizeof WCHAR);
    _ctxFile = (LPWSTR)sys_alloc(MAX_PATH * sizeof WCHAR);

    _nppVersion = ::SendMessage(_hNpp, NPPM_GETNPPVERSION, 0, 0);
    _winVersion = ::SendMessage(_hNpp, NPPM_GETWINDOWSVERSION, 0, 0);

    // Get NPP's "plugins\Config" directory.
    ::SendMessage(_hNpp, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)_ctxFile);

    // Get SessionMgr config directory and create it if missing.
    ::StringCchCopyW(_cfgDir, MAX_PATH, _ctxFile);
    pth::appendSlash(_cfgDir, MAX_PATH);
    ::StringCchCatW(_cfgDir, MAX_PATH, PLUGIN_DLL_NAME);
    ::StringCchCatW(_cfgDir, MAX_PATH, L"\\");
    ::CreateDirectoryW(_cfgDir, NULL);

    // Get the global.xml file pathname and create it if missing.
    ::StringCchCopyW(_glbFile, MAX_PATH, _cfgDir);
    ::StringCchCatW(_glbFile, MAX_PATH, GLB_FILE_NAME);
    pth::createFileIfMissing(_glbFile, GLB_DEFAULT_CONTENT);

    // Get the settings.xml file pathname and load the configuration.
    ::StringCchCopyW(_cfgFile, MAX_PATH, _cfgDir);
    ::StringCchCatW(_cfgFile, MAX_PATH, CFG_FILE_NAME);
    cfg::loadSettings();
    // Create sessions directory if missing.
    ::CreateDirectoryW(cfg::getStr(kSessionDirectory), NULL);
    // Create default session file if missing.
    app_confirmDefaultSession();

    // Get NPP's contextMenu.xml pathname (two directories up from "plugins\Config").
    LPWSTR p = ::wcsstr(_ctxFile, L"plugins\\Config");
    ::StringCchCopyW(p, MAX_PATH, CTX_FILE_NAME);

    // Backup existing config and session files.
    if (cfg::getBool(kBackupOnStartup)) {
        backupFiles();
    }
}

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

LPWSTR sys_getCfgDir()
{
    return _cfgDir;
}

LPWSTR sys_getSettingsFile()
{
    return _cfgFile;
}

LPWSTR sys_getGlobalFile()
{
    return _glbFile;
}

LPCWSTR sys_getNppCtxMnuFile()
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

/** @return NPP version with major in hiword and minor in loword */
DWORD sys_getNppVer()
{
    return _nppVersion;
}

/** @return a winVer enum
    @see npp/Notepad_plus_msgs.h */
UINT sys_getWinVer()
{
    return _winVersion;
}

LPVOID sys_alloc(INT bytes)
{
    LPVOID p = ::HeapAlloc(_hHeap, HEAP_ZERO_MEMORY, bytes);
    if (p == NULL) {
        LOG("Error allocating %u bytes.", bytes);
        msg::show(L"Memory allocation failed.", M_ERR);
    }
    return p;
}

void sys_free(LPVOID p)
{
    if (p) ::HeapFree(_hHeap, 0, p);
}

//------------------------------------------------------------------------------

namespace {

/* Gets the pathname of NPP's contextMenu.xml. Now not used. Just assume the
   file is two directories up from "plugins\Config".
void findNppCtxMnuFile()
{
    ITEMIDLIST *pidl;
    bool isLocal = false;
    WCHAR tmp[MAX_PATH], nppDir[MAX_PATH];

    ::SendMessage(_hNpp, NPPM_GETNPPDIRECTORY, MAX_PATH, (LPARAM)nppDir);
    pth::appendSlash(nppDir, MAX_PATH);
    ::StringCchCopyW(tmp, MAX_PATH, nppDir);
    ::StringCchCatW(tmp, MAX_PATH, L"doLocalConf.xml");
    if (pth::fileExists(tmp)) {
        isLocal = true;
        LOGG(12, "Found doLocalConf.xml");
    }

    // See NppParameters::load in NPP's Parameters.cpp...
    if (isLocal && _winVersion >= WV_VISTA) {
        if (::SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl) == S_OK) {
            if (::SHGetPathFromIDList(pidl, tmp)) {
                LOGG(12, "prg=\"%S\", npp=\"%S\"", tmp, nppDir);
                if  (::_wcsnicmp(tmp, nppDir, wcslen(tmp)) == 0) {
                    isLocal = false;
                }
            }
            else LOGG(12, "SHGetPathFromIDList failed for CSIDL_PROGRAM_FILES");
            //::CoTaskMemFree(pidl);
        }
        else LOGG(12, "SHGetSpecialFolderLocation failed for CSIDL_PROGRAM_FILES");
    }
    ::StringCchCopyW(_ctxFile, MAX_PATH, nppDir);
    if (!isLocal) {
        if (::SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl) == S_OK) {
            if (::SHGetPathFromIDList(pidl, tmp)) {
                pth::appendSlash(tmp, MAX_PATH);
                ::StringCchCatW(tmp, MAX_PATH, L"Notepad++");
                if (pth::dirExists(tmp)) {
                    LOGG(12, "Found \"%S\"", tmp);
                    ::StringCchCopyW(_ctxFile, MAX_PATH, tmp);
                }
                else LOGG(12, "Could not find \"%S\"", tmp);
            }
            else LOGG(12, "SHGetPathFromIDList failed for CSIDL_APPDATA");
            //::CoTaskMemFree(pidl);
        }
        else LOGG(12, "SHGetSpecialFolderLocation failed for CSIDL_APPDATA");
    }

    pth::appendSlash(_ctxFile, MAX_PATH);
    ::StringCchCatW(_ctxFile, MAX_PATH, CTX_FILE_NAME);
    LOGG(12, "Will use \"%S\"", _ctxFile);
}
*/

void backupFiles()
{
    WCHAR backupDir[MAX_PATH];

    // Create main backup directory if missing and copy config files.
    ::StringCchCopyW(backupDir, MAX_PATH, _cfgDir);
    ::StringCchCatW(backupDir, MAX_PATH, BAK_DIR_NAME);
    if (!pth::dirExists(backupDir)) {
        ::CreateDirectoryW(backupDir, NULL);
    }
    if (pth::dirExists(backupDir)) {
        ::StringCchCatW(backupDir, MAX_PATH, L"\\");
        backupConfigFiles(backupDir);
        // Create backup directory for session files if missing and copy session files.
        ::StringCchCatW(backupDir, MAX_PATH, BAK_SES_DIR_NAME);
        if (!pth::dirExists(backupDir)) {
            ::CreateDirectoryW(backupDir, NULL);
        }
        if (pth::dirExists(backupDir)) {
            ::StringCchCatW(backupDir, MAX_PATH, L"\\");
            backupSessionFiles(backupDir);
        }
    }
}

void backupConfigFiles(LPCWSTR backupDir)
{
    WCHAR dstFile[MAX_PATH];

    // Copy settings.xml
    if (pth::fileExists(_cfgFile)) {
        ::StringCchCopyW(dstFile, MAX_PATH, backupDir);
        ::StringCchCatW(dstFile, MAX_PATH, CFG_FILE_NAME);
        ::CopyFileW(_cfgFile, dstFile, FALSE);
    }
    // Copy global.xml
    if (pth::fileExists(_glbFile)) {
        ::StringCchCopyW(dstFile, MAX_PATH, backupDir);
        ::StringCchCatW(dstFile, MAX_PATH, GLB_FILE_NAME);
        ::CopyFileW(_glbFile, dstFile, FALSE);
    }
    // Copy NPP's contextMenu.xml
    if (pth::fileExists(_ctxFile)) {
        ::StringCchCopyW(dstFile, MAX_PATH, backupDir);
        ::StringCchCatW(dstFile, MAX_PATH, CTX_FILE_NAME);
        ::CopyFileW(_ctxFile, dstFile, FALSE);
    }
}

void backupSessionFiles(LPCWSTR backupDir)
{
    HANDLE hFind;
    WIN32_FIND_DATAW ffd;
    WCHAR fileSpec[MAX_PATH];
    WCHAR srcFile[MAX_PATH];
    WCHAR dstFile[MAX_PATH];

    // Create the file spec.
    ::StringCchCopyW(fileSpec, MAX_PATH, cfg::getStr(kSessionDirectory));
    ::StringCchCatW(fileSpec, MAX_PATH, L"*.*");
    // Loop over files in the session directory and copy them to the backup directory.
    hFind = ::FindFirstFileW(fileSpec, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        ::StringCchCopyW(srcFile, MAX_PATH, cfg::getStr(kSessionDirectory));
        ::StringCchCatW(srcFile, MAX_PATH, ffd.cFileName);
        ::StringCchCopyW(dstFile, MAX_PATH, backupDir);
        ::StringCchCatW(dstFile, MAX_PATH, ffd.cFileName);
        ::CopyFileW(srcFile, dstFile, FALSE);
    }
    while (::FindNextFileW(hFind, &ffd) != 0);
    ::FindClose(hFind);
}

} // end namespace

} // end namespace NppPlugin

