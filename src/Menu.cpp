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
    @file      Menu.cpp
    @copyright Copyright 2011,2013,2014 Michael Foster <http://mfoster.com/npp/>

    The NPP "Plugins" menu entries for Session Manager.
*/

#include "System.h"
#include "SessionMgr.h"
#include "Config.h"
#include "DlgSessions.h"
#include "DlgSettings.h"
#include "Menu.h"
#include "Util.h"
#include "res\resource.h"
#include <strsafe.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

#define PLUGIN_MENU_NAME L"&" PLUGIN_FULL_NAME
#define PLUGIN_ABOUT PLUGIN_FULL_NAME SPACE_STR PLUGIN_VERSION L"\nA plugin for Notepad++\nhttp://mfoster.com/npp/"
#define HELP_FILE_SUFFIX L"doc\\" PLUGIN_DLL_NAME L".html"

/// Menu callback functions
extern "C" {
    void cbSessions();
    void cbSettings();
    void cbSaveCurrent();
    void cbLoadPrevious();
    void cbHelp();
    void cbAbout();
};

/// Menu config
WCHAR _menuMainLabel[MNU_MAX_NAME_LEN + 1];
FuncItem _menuItems[] = {
    { L"&Sessions...",   cbSessions,     0, false, NULL },
    { L"Se&ttings...",   cbSettings,     0, false, NULL },
    { L"Sa&ve current",  cbSaveCurrent,  0, false, NULL },
    { L"Load &previous", cbLoadPrevious, 0, false, NULL },
    { EMPTY_STR,         NULL,           0, false, NULL },
    { L"&Help",          cbHelp,         0, false, NULL },
    { L"&About...",      cbAbout,        0, false, NULL }
};

} // end namespace

//------------------------------------------------------------------------------

namespace api {

void mnu_onLoad()
{
}

void mnu_onUnload()
{
}

void mnu_init()
{
    ::StringCchCopyW(_menuMainLabel, MNU_MAX_NAME_LEN, PLUGIN_MENU_NAME);
    gCfg.getMenuLabel(-1, _menuMainLabel);
    for (int i = 0; i < MNU_MAX_ITEMS; ++i) {
        gCfg.getMenuLabel(i, _menuItems[i]._itemName);
    }
}

FuncItem* mnu_getItems(INT *pNum)
{
    *pNum = MNU_MAX_ITEMS;
    return _menuItems;
}

} // end namespace api

//------------------------------------------------------------------------------

LPWSTR mnu_getMainMenuLabel()
{
    return _menuMainLabel;
}

//------------------------------------------------------------------------------

namespace {

extern "C" void cbSessions()
{
    app_readSessionDirectory();
    ::DialogBox(sys_getDllHandle(), MAKEINTRESOURCE(IDD_SES_DLG), sys_getNppHandle(), dlgSes_msgProc);
}

extern "C" void cbSettings()
{
    ::DialogBox(sys_getDllHandle(), MAKEINTRESOURCE(IDD_CFG_DLG), sys_getNppHandle(), dlgCfg_msgProc);
}

extern "C" void cbSaveCurrent()
{
    app_saveSession(SI_CURRENT);
}

extern "C" void cbLoadPrevious()
{
    app_loadSession(SI_PREVIOUS);
}

extern "C" void cbHelp()
{
    WCHAR helpFile[MAX_PATH];

    ::GetModuleFileNameW((HMODULE)sys_getDllHandle(), helpFile, MAX_PATH);
    pth::removeName(helpFile, MAX_PATH);
    ::StringCchCatW(helpFile, MAX_PATH, HELP_FILE_SUFFIX);

    HINSTANCE h = ::ShellExecuteW(NULL, L"open", helpFile, NULL, NULL, SW_SHOW);
    if ((int)h <= 32) {
        DWORD le = ::GetLastError();
        msg::error(le, L"%s: Error shelling to default browser.", _W(__FUNCTION__));
    }
}

extern "C" void cbAbout()
{
    const size_t s = 7 * MAX_PATH;
    WCHAR m[s + 1], b[MAX_PATH];

    ::StringCchCopyW(m, s, PLUGIN_ABOUT);
    ::StringCchCatW(m, s, L"\n\nConfiguration directory:\n");
    ::StringCchCatW(m, s, sys_getCfgDir());
    ::StringCchCatW(m, s, L"\n\nSpecial thanks to...\n- Don Ho, for Notepad++\n- Dave Brotherstone, for PluginManager\n- Julien Audo, for ResEdit\n- Lee Thomason, for TinyXML2\n- Nemanja Trifunovic, for UTF8-CPP\n- Jens Lorenz and Thell Fowler, for example code\n- Users at the plugin forum, for testing and feedback\n- You, for using Session Manager");
    msg::show(m);
}

} // end namespace

} // end namespace NppPlugin
