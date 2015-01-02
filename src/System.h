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
    @file      System.h
    @copyright Copyright 2011,2014,2015 Michael Foster <http://mfoster.com/npp/>
*/

#ifndef NPP_PLUGIN_SYSTEM_H
#define NPP_PLUGIN_SYSTEM_H

#define SES_DEFAULT_CONTENTS "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<NotepadPlus><Session activeView=\"0\"><mainView activeIndex=\"0\"></mainView></Session></NotepadPlus>\n"

#include <windows.h>
#include "npp\PluginInterface.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------
/// @namespace NppPlugin::api Contains functions called only from DllMain.

namespace api {

void sys_onLoad(HINSTANCE hDLLInstance);
void sys_onUnload();
void sys_init(NppData nppd);

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

LPWSTR sys_getCfgDir();
LPWSTR sys_getIniFile();
LPWSTR sys_getSettingsFile();
LPWSTR sys_getPropsFile();
LPCWSTR sys_getContextMenuFile();
HINSTANCE sys_getDllHandle();
HWND sys_getNppHandle();
HWND sys_getSciHandle(INT v);
LPVOID sys_alloc(INT bytes);
void sys_free(LPVOID p);

} // end namespace NppPlugin

#endif // NPP_PLUGIN_SYSTEM_H
