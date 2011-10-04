/*
    Menu.cpp
    Copyright 2011 Michael Foster (http://mfoster.com/npp/)

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

#define PLUGIN_ABOUT     PLUGIN_FULL_NAME SPACE_STR PLUGIN_VERSION _T("\nA plugin for Notepad++\nhttp://mfoster.com/npp/")

// Menu callback functions
extern "C" {
    void cbSessions();
    void cbSettings();
    void cbSave();
    void cbHelp();
    void cbAbout();
};

// Menu items
const INT _mnuItmCnt = 6;
FuncItem _mnuItems[] = {
    { _T("&Sessions..."), cbSessions, 0, false, NULL },
    { _T("Se&ttings..."), cbSettings, 0, false, NULL },
    { _T("Sa&ve"),        cbSave,     0, false, NULL },
    { EMPTY_STR,          NULL,       0, false, NULL },
    { _T("&Help"),        cbHelp,     0, false, NULL },
    { _T("&About..."),    cbAbout,    0, false, NULL }
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
}

FuncItem* mnu_getItems(INT *pNum)
{
    *pNum = _mnuItmCnt;
    return _mnuItems;
}

} // end namespace api

//------------------------------------------------------------------------------

namespace {

extern "C" void cbSessions()
{
    app_readSesDir();
    DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_SES_DLG), sys_getNppHwnd(), dlgSes_msgProc);
}

extern "C" void cbSettings()
{
    DialogBox(sys_getDllHwnd(), MAKEINTRESOURCE(IDD_CFG_DLG), sys_getNppHwnd(), dlgCfg_msgProc);
}

extern "C" void cbSave()
{
    app_saveSession(SES_CURRENT);
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
    const size_t s = 6 * MAX_PATH;
    TCHAR m[s + 1], b[MAX_PATH_1];
    StringCchCopy(m, s, PLUGIN_ABOUT);
    StringCchCat(m, s, _T("\n\nHelp file:\n"));
    StringCchCat(m, s, sys_getHelpFile());
    StringCchCat(m, s, _T("\n\nSettings file:\n"));
    StringCchCat(m, s, sys_getIniFile());
    StringCchCat(m, s, _T("\n\nCurrent session file:\n"));
    app_getSesFile(SES_CURRENT, b);
    StringCchCat(m, s, b);
    msgBox(m);
}

} // end namespace

} // end namespace NppPlugin
