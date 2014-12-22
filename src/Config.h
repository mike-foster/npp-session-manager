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
    @file      Config.h
    @copyright Copyright 2011-2014 Michael Foster <http://mfoster.com/npp/>

    The configuration (settings) object.
*/

#ifndef NPP_PLUGIN_CONFIG_H
#define NPP_PLUGIN_CONFIG_H

#include "Menu.h"
#include "SessionMgr.h"
#include <list>

using std::list;

//------------------------------------------------------------------------------

namespace NppPlugin {

#define SORT_ORDER_ALPHA 1
#define SORT_ORDER_DATE  2

#define FIL_EXP_BUF_LEN 50
#define FIL_EXP_MAX     100

#define CURRENT_MARK      0
#define CURRENT_FAV_MARK  1
#define PREVIOUS_MARK     2
#define PREVIOUS_FAV_MARK 3
#define DEFAULT_MARK      4
#define DEFAULT_FAV_MARK  5
#define FAVORITE_MARK     6

class SessionFilter
{
  public:
    WCHAR exp[FIL_EXP_BUF_LEN];
    SessionFilter(LPCWSTR filter);
};

class Config
{
  private:

    // private properties
    bool _autoSave;
    bool _autoLoad;
    bool _loadIntoCurrent;
    bool _loadWithoutClosing;
    bool _showInTitlebar;
    bool _showInStatusbar;
    bool _globalBookmarks;
    bool _useContextMenu;
    list<SessionFilter> _filters;
    UINT _sortOrder;
    UINT _saveDelay;
    WCHAR _directory[MAX_PATH];
    WCHAR _extension[MAX_PATH];
    WCHAR _defaultName[SES_NAME_BUF_LEN];

    // private methods
    BOOL saveDlgSize(bool ses, INT w, INT h);
    void loadFilters();
    BOOL saveFilters();
    void loadMarks();

  public:

    // public properties
    INT debug;
    WCHAR logFile[MAX_PATH];
    WCHAR markChars[7][3];

    // public methods
    void load();
    bool save();
    void getMenuLabel(INT prpIdx, LPWSTR buf);
    bool getFavMenuLabel(INT prpIdx, LPWSTR buf);
    void deleteFavorites();
    void addFavorite(INT prpIdx, LPCWSTR favName);

    void setAutoSave(bool v) { _autoSave = v; }
    bool autoSaveEnabled() { return _autoSave; }
    void setAutoLoad(bool v) { _autoLoad = v; }
    bool autoLoadEnabled() { return _autoLoad; }
    void setGlobalBookmarks(bool v) { _globalBookmarks = v; }
    bool globalBookmarksEnabled() { return _globalBookmarks; }
    void setUseContextMenu(bool v) { _useContextMenu = v; }
    bool useContextMenuEnabled() { return _useContextMenu; }

    void setLoadIntoCurrent(bool v) { _loadIntoCurrent = v; }
    bool loadIntoCurrentEnabled() { return _loadIntoCurrent; }
    void setLoadWithoutClosing(bool v) { _loadWithoutClosing = v; }
    bool loadWithoutClosingEnabled() { return _loadWithoutClosing; }
    bool sortAlphaEnabled() { return _sortOrder == SORT_ORDER_ALPHA; }
    void setShowInStatusbar(bool v);
    bool showInStatusbarEnabled() { return _showInStatusbar; }
    void setShowInTitlebar(bool v);
    bool showInTitlebarEnabled() { return _showInTitlebar; }
    UINT getSaveDelay() { return _saveDelay; }

    bool setSesDir(LPWSTR p);
    LPWSTR getSesDir() { return _directory; }
    void setSesExt(LPWSTR p);
    LPWSTR getSesExt() { return _extension; }
    LPWSTR getDefaultName() { return _defaultName; }

    BOOL saveSortOrder(INT order);
    void readCurrentName(LPWSTR buf);
    BOOL saveCurrentName(LPWSTR s);
    void readPreviousName(LPWSTR s);
    BOOL savePreviousName(LPWSTR s);
    BOOL saveDefaultName(LPWSTR s);
    void readSesDlgSize(INT *w, INT *h);
    void saveSesDlgSize(INT w, INT h);
    void readCfgDlgSize(INT *w, INT *h);
    void saveCfgDlgSize(INT w, INT h);

    LPCWSTR getFilter(INT index);
    void addFilter(LPCWSTR filter);
};

// The global configuration/settings object.
extern Config gCfg;

} // end namespace NppPlugin

#endif // NPP_PLUGIN_CONFIG_H
