/*
    SessionMgr.h
    Copyright 2011,2013 Michael Foster (http://mfoster.com/npp/)

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

#ifndef NPP_PLUGIN_APPLICATION_H
#define NPP_PLUGIN_APPLICATION_H

#include "res\version.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

#define PLUGIN_DLL_NAME  _T("SessionMgr")
#define PLUGIN_FULL_NAME _T("Session Manager")
#define SES_MAX_LEN      100
#define SES_NONE         -1
#define SES_CURRENT      -2
#define SES_PREVIOUS     -3
#define SES_DEFAULT      -4

//------------------------------------------------------------------------------
// The api namespace contains functions called only from DllMain.

namespace api {

void app_onLoad();
void app_onUnload();
void app_init();
const TCHAR* app_getName();
void app_onNotify(SCNotification *pscn);
LRESULT app_msgProc(UINT Message, WPARAM wParam, LPARAM lParam);

} // end namespace api

//------------------------------------------------------------------------------

void app_readSesDir();
void app_loadSession(INT si);
void app_loadSession(INT si, bool lic, bool lwc);
void app_saveSession(INT si = SES_CURRENT);
bool app_validSesIndex(INT si);
INT app_getSesCount();
INT app_getSesIndex(TCHAR *name = NULL);
const TCHAR* app_getSesName(INT si = SES_CURRENT);
void app_getSesFile(INT si, TCHAR *buf);
void app_showSesInNppBars();

} // end namespace NppPlugin

#endif // NPP_PLUGIN_APPLICATION_H
