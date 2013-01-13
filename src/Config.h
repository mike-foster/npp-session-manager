/*
    Config.h
    Copyright 2011,2012,2013 Michael Foster (http://mfoster.com/npp/)

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

#ifndef NPP_PLUGIN_CONFIG_H
#define NPP_PLUGIN_CONFIG_H

#include "Menu.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

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
    UINT _saveDelay;
    TCHAR _directory[MAX_PATH_1];
    TCHAR _extension[MAX_PATH_1];
    TCHAR _menuMainLabel[MNU_MAX_NAME_LEN + 1];
    TCHAR _menuSubLabels[MNU_MAX_ITEMS + 1][MNU_MAX_NAME_LEN + 1];

    // private methods
    BOOL saveDlgSize(bool ses, INT w, INT h);

  public:

    // public properties
    bool debug;

    // public methods
    void load();
    bool save();
    void getMenuLabel(int idx, TCHAR *buf);

    void setAutoSave(bool v) { _autoSave = v; }
    bool getAutoSave() { return _autoSave; }
    void setAutoLoad(bool v) { _autoLoad = v; }
    bool getAutoLoad() { return _autoLoad; }
    void setLoadIntoCurrent(bool v) { _loadIntoCurrent = v; }
    bool getLoadIntoCurrent() { return _loadIntoCurrent; }
    void setLoadWithoutClosing(bool v) { _loadWithoutClosing = v; }
    bool getLoadWithoutClosing() { return _loadWithoutClosing; }
    void setShowInStatusbar(bool v);
    bool getShowInStatusbar() { return _showInStatusbar; }
    void setShowInTitlebar(bool v);
    bool getShowInTitlebar() { return _showInTitlebar; }
    void setSaveDelay(TCHAR *p);
    int getSaveDelay() { return _saveDelay; }
    void getSaveDelay(TCHAR *buf, INT len);

    bool setSesDir(TCHAR *p);
    TCHAR *getSesDir() { return _directory; }
    void setSesExt(TCHAR *p);
    TCHAR *getSesExt() { return _extension; }

    void readCurrent(TCHAR *buf);
    BOOL saveCurrent(TCHAR *s);
    void readPrevious(TCHAR *s);
    BOOL savePrevious(TCHAR *s);
    void readSesDlgSize(INT *w, INT *h);
    void saveSesDlgSize(INT w, INT h);
    void readCfgDlgSize(INT *w, INT *h);
    void saveCfgDlgSize(INT w, INT h);
};

// The global configuration/settings object.
extern Config gCfg;

} // end namespace NppPlugin

#endif // NPP_PLUGIN_CONFIG_H
