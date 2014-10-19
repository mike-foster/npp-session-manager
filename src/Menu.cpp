/*
    Menu.cpp
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

#define PLUGIN_MENU_NAME _T("&") PLUGIN_FULL_NAME
#define PLUGIN_ABOUT PLUGIN_FULL_NAME SPACE_STR PLUGIN_VERSION _T("\nA plugin for Notepad++\nhttp://mfoster.com/npp/")

// Menu callback functions
extern "C" {
    void cbSessions();
    void cbSettings();
    void cbSaveCurrent();
    void cbLoadPrevious();
    void cbHelp();
    void cbAbout();
};

// Menu config
TCHAR _menuMainLabel[MNU_MAX_NAME_LEN + 1];
FuncItem _menuItems[] = {
    { _T("&Sessions..."),   cbSessions,     0, false, NULL },
    { _T("Se&ttings..."),   cbSettings,     0, false, NULL },
    { _T("Sa&ve current"),  cbSaveCurrent,  0, false, NULL },
    { _T("Load &previous"), cbLoadPrevious, 0, false, NULL },
    { EMPTY_STR,            NULL,           0, false, NULL },
    { _T("&Help"),          cbHelp,         0, false, NULL },
    { _T("&About..."),      cbAbout,        0, false, NULL }
};

} // end namespace

//------------------------------------------------------------------------------
// The api namespace contains functions called only from DllMain.

namespace api {

void mnu_onLoad()
{
}

void mnu_onUnload()
{
}

void mnu_init()
{
    StringCchCopy(_menuMainLabel, MNU_MAX_NAME_LEN, PLUGIN_MENU_NAME);
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

TCHAR* mnu_getMainMenuLabel()
{
    return _menuMainLabel;
}

//------------------------------------------------------------------------------

namespace {

extern "C" void cbSessions()
{
    app_readSessionDirectory();
    DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_SES_DLG), sys_getNppHwnd(), dlgSes_msgProc);
}

extern "C" void cbSettings()
{
    DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_CFG_DLG), sys_getNppHwnd(), dlgCfg_msgProc);
}

extern "C" void cbSaveCurrent()
{
    app_saveSession(SES_CURRENT);
}

extern "C" void cbLoadPrevious()
{
    app_loadSession(SES_PREVIOUS);
}

extern "C" void cbHelp()
{
    HINSTANCE h = ShellExecute(NULL, _T("open"), sys_getHelpFile(), NULL, NULL, SW_SHOW);
    if ((int)h <= 32) {
        errBox(_T("Shell"), GetLastError());
    }
}

extern "C" void cbAbout()
{
    const size_t s = 7 * MAX_PATH;
    TCHAR m[s + 1], b[MAX_PATH_P1];
    StringCchCopy(m, s, PLUGIN_ABOUT);

    StringCchCat(m, s, _T("\n\nConfiguration directory:\n"));
    StringCchCat(m, s, sys_getCfgDir());
    StringCchCat(m, s, _T("\n\nSpecial thanks to...\n- Don Ho, for Notepad++\n- Dave Brotherstone, for PluginManager\n- Julien Audo, for ResEdit\n- Lee Thomason, for TinyXML2\n- Jens Lorenz, for the plugin template\n- Thell Fowler, for the plugin template\n- Members of the Notepad++ forums"));
    msgBox(m);
}

} // end namespace

} // end namespace NppPlugin
