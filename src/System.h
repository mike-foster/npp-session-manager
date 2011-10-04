/*
    System.h
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

#ifndef NPP_PLUGIN_SYSTEM_H
#define NPP_PLUGIN_SYSTEM_H

#define UNICODE
#define _UNICODE
#define MAX_PATH_1 (MAX_PATH + 1)
#define SES_DEFAULT_NAME _T("default")
#define SES_DEFAULT_CONTENTS "<NotepadPlus>\r\n<Session activeView=\"0\">\r\n<mainView activeIndex=\"0\">\r\n</mainView>\r\n</Session>\r\n</NotepadPlus>\r\n"

#include <windows.h>
#include <tchar.h>
#include "npp\PluginInterface.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------
// The api namespace contains functions called only from DllMain.

namespace api {

void sys_onLoad(HINSTANCE hDLLInstance);
void sys_onUnload();
void sys_init(NppData nppd);

} // end namespace api

//------------------------------------------------------------------------------

TCHAR* sys_getCfgDir();
TCHAR* sys_getIniFile();
TCHAR* sys_getHelpFile();
TCHAR* sys_getDefSesFile();
HINSTANCE sys_getDllHwnd();
HWND sys_getNppHwnd();

} // end namespace NppPlugin

#endif // NPP_PLUGIN_SYSTEM_H
