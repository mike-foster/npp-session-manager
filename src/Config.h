/*
    Config.h
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

#ifndef NPP_PLUGIN_CONFIG_H
#define NPP_PLUGIN_CONFIG_H

//------------------------------------------------------------------------------

namespace NppPlugin {

class Config
{
  private:

    // session properties
    TCHAR _directory[MAX_PATH_1];
    TCHAR _extension[MAX_PATH_1];
    bool _autoSave;
    bool _autoLoad;
    bool _enableLIC;
    bool _disableLWC;
    UINT _saveDelay;

  public:

    // debug property
    bool debug;

    // Methods
    void load();
    bool save();
    void readCurrent(TCHAR *buf);
    BOOL saveCurrent(TCHAR *s);
    bool setSesDir(TCHAR *p);
    TCHAR *getSesDir() { return _directory; }
    void setSesExt(TCHAR *p);
    TCHAR *getSesExt() { return _extension; }
    void setAutoSave(bool v) { _autoSave = v; }
    bool getAutoSave() { return _autoSave; }
    void setAutoLoad(bool v) { _autoLoad = v; }
    bool getAutoLoad() { return _autoLoad; }
    void setEnableLIC(bool v) { _enableLIC = v; }
    bool getEnableLIC() { return _enableLIC; }
    void setDisableLWC(bool v) { _disableLWC = v; }
    bool getDisableLWC() { return _disableLWC; }
    void setSaveDelay(TCHAR *p);
    int getSaveDelay() { return _saveDelay; }
    void getSaveDelay(TCHAR *buf, INT len);
};

// The global configuration/settings object.
extern Config gCfg;

} // end namespace NppPlugin

#endif // NPP_PLUGIN_CONFIG_H
