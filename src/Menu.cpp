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
    @copyright Copyright 2011,2013-2015 Michael Foster <http://mfoster.com/npp/>

    Session Manager creates a submenu in the Notepad++ Plugins menu. Those items
    can be customized via settings. Favorite sessions are listed after the About
    item.
*/

#include "System.h"
#include "SessionMgr.h"
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
    void cbFav1();
    void cbFav2();
    void cbFav3();
    void cbFav4();
    void cbFav5();
    void cbFav6();
    void cbFav7();
    void cbFav8();
    void cbFav9();
    void cbFav10();
    void cbFav11();
    void cbFav12();
    void cbFav13();
    void cbFav14();
    void cbFav15();
    void cbFav16();
    void cbFav17();
    void cbFav18();
    void cbFav19();
    void cbFav20();
};

/// Menu config
INT _menuItemsCount = MNU_BASE_MAX_ITEMS;
WCHAR _menuMainLabel[MNU_MAX_NAME_LEN + 1];
FuncItem _menuItems[] = {
    { EMPTY_STR, cbSessions,     0, false, NULL },
    { EMPTY_STR, cbSettings,     0, false, NULL },
    { EMPTY_STR, cbSaveCurrent,  0, false, NULL },
    { EMPTY_STR, cbLoadPrevious, 0, false, NULL },
    { EMPTY_STR, NULL,           0, false, NULL },
    { EMPTY_STR, cbHelp,         0, false, NULL },
    { EMPTY_STR, cbAbout,        0, false, NULL },
    { EMPTY_STR, NULL,           0, false, NULL },
    { EMPTY_STR, cbFav1,         0, false, NULL },
    { EMPTY_STR, cbFav2,         0, false, NULL },
    { EMPTY_STR, cbFav3,         0, false, NULL },
    { EMPTY_STR, cbFav4,         0, false, NULL },
    { EMPTY_STR, cbFav5,         0, false, NULL },
    { EMPTY_STR, cbFav6,         0, false, NULL },
    { EMPTY_STR, cbFav7,         0, false, NULL },
    { EMPTY_STR, cbFav8,         0, false, NULL },
    { EMPTY_STR, cbFav9,         0, false, NULL },
    { EMPTY_STR, cbFav10,        0, false, NULL },
    { EMPTY_STR, cbFav11,        0, false, NULL },
    { EMPTY_STR, cbFav12,        0, false, NULL },
    { EMPTY_STR, cbFav13,        0, false, NULL },
    { EMPTY_STR, cbFav14,        0, false, NULL },
    { EMPTY_STR, cbFav15,        0, false, NULL },
    { EMPTY_STR, cbFav16,        0, false, NULL },
    { EMPTY_STR, cbFav17,        0, false, NULL },
    { EMPTY_STR, cbFav18,        0, false, NULL },
    { EMPTY_STR, cbFav19,        0, false, NULL },
    { EMPTY_STR, cbFav20,        0, false, NULL }
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
    INT mnuIdx, cfgIdx = 0;

    // plugin menu
    cfg::getStr(kMenuLabelMain, _menuMainLabel, MNU_MAX_NAME_LEN);
    cfg::getStr(kMenuLabelSub1, _menuItems[0]._itemName, MNU_MAX_NAME_LEN);
    cfg::getStr(kMenuLabelSub2, _menuItems[1]._itemName, MNU_MAX_NAME_LEN);
    cfg::getStr(kMenuLabelSub3, _menuItems[2]._itemName, MNU_MAX_NAME_LEN);
    cfg::getStr(kMenuLabelSub4, _menuItems[3]._itemName, MNU_MAX_NAME_LEN);
    cfg::getStr(kMenuLabelSub5, _menuItems[5]._itemName, MNU_MAX_NAME_LEN);
    cfg::getStr(kMenuLabelSub6, _menuItems[6]._itemName, MNU_MAX_NAME_LEN);
    // favorites
    cfgIdx = 0;
    for (mnuIdx = MNU_FIRST_FAV_IDX; mnuIdx <= MNU_LAST_FAV_IDX; ++mnuIdx) {
        if (!cfg::getStr(kFavorites, cfgIdx++, _menuItems[mnuIdx]._itemName, MNU_MAX_NAME_LEN)) {
            break;
        }
        ++_menuItemsCount;
    }
    if (cfgIdx > 0) {
        ++_menuItemsCount; // for the 2nd separator if any fav was added
    }
}

FuncItem* mnu_getItems(INT *pNum)
{
    *pNum = _menuItemsCount;
    return _menuItems;
}

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

/** @return a pointer to the main or the 0-based mnuIdx'th sub label. */
LPCWSTR mnu_getMenuLabel(INT mnuIdx)
{
    LPCWSTR lbl = NULL;

    if (mnuIdx == -1) {
        lbl = _menuMainLabel;
    }
    else if (mnuIdx >= 0 && mnuIdx < MNU_BASE_MAX_ITEMS) {
        lbl = _menuItems[mnuIdx]._itemName;
    }
    return lbl;
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
    const size_t s = 450;
    WCHAR m[s];

    ::StringCchCopyW(m, s, PLUGIN_ABOUT);
    ::StringCchCatW(m, s, L"\n\nSpecial thanks to....\n\n- Don Ho and team, for Notepad++\n- You! for using Session Manager\n- Dave Brotherstone, for PluginManager\n- Jack Handy, for wildcardMatch\n- Jens Lorenz and Thell Fowler, for example code\n- Julien Audo, for ResEdit\n- Lee Thomason, for TinyXML2\n- Nemanja Trifunovic, for UTF8-CPP\n- Users at the plugin forum, for testing and feedback");
    msg::show(m, L"About Session Manager", MB_ICONINFORMATION);
    //LOG("strlen = %u", ::wcslen(m));
}

void loadFavorite(INT mnuOfs)
{
    app_loadSession(app_getSessionIndex(_menuItems[mnuOfs + MNU_BASE_MAX_ITEMS]._itemName));
}

extern "C" void cbFav1() { loadFavorite(1); }
extern "C" void cbFav2() { loadFavorite(2); }
extern "C" void cbFav3() { loadFavorite(3); }
extern "C" void cbFav4() { loadFavorite(4); }
extern "C" void cbFav5() { loadFavorite(5); }
extern "C" void cbFav6() { loadFavorite(6); }
extern "C" void cbFav7() { loadFavorite(7); }
extern "C" void cbFav8() { loadFavorite(8); }
extern "C" void cbFav9() { loadFavorite(9); }
extern "C" void cbFav10() { loadFavorite(10); }
extern "C" void cbFav11() { loadFavorite(11); }
extern "C" void cbFav12() { loadFavorite(12); }
extern "C" void cbFav13() { loadFavorite(13); }
extern "C" void cbFav14() { loadFavorite(14); }
extern "C" void cbFav15() { loadFavorite(15); }
extern "C" void cbFav16() { loadFavorite(16); }
extern "C" void cbFav17() { loadFavorite(17); }
extern "C" void cbFav18() { loadFavorite(18); }
extern "C" void cbFav19() { loadFavorite(19); }
extern "C" void cbFav20() { loadFavorite(20); }

} // end namespace

} // end namespace NppPlugin
