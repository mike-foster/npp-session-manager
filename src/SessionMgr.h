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
    @file      SessionMgr.h
    @copyright Copyright 2011,2013,2014 Michael Foster <http://mfoster.com/npp/>
*/

#ifndef NPP_PLUGIN_APPLICATION_H
#define NPP_PLUGIN_APPLICATION_H

#include "res\version.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

#define PLUGIN_DLL_NAME     L"SessionMgr"
#define PLUGIN_FULL_NAME    L"Session Manager"
#define SES_NAME_NONE       L"none"
#define SES_NAME_DEFAULT    L"default"
#define SES_NAME_BUF_LEN    100
/// Used as virtual session indexes
#define SI_NONE     -1
#define SI_CURRENT  -2
#define SI_PREVIOUS -3
#define SI_DEFAULT  -4

//------------------------------------------------------------------------------
/// @namespace NppPlugin.api Contains functions called only from DllMain.

namespace api {

void app_onLoad();
void app_onUnload();
void app_init();
LPCWSTR app_getName();
void app_onNotify(SCNotification *pscn);
LRESULT app_msgProc(UINT Message, WPARAM wParam, LPARAM lParam);

} // end namespace api

//------------------------------------------------------------------------------

void app_readSessionDirectory();
void app_loadSession(INT si);
void app_loadSession(INT si, bool lic, bool lwc);
void app_saveSession(INT si = SI_CURRENT);
bool app_isValidSessionIndex(INT si);
INT app_getSessionCount();
INT app_getSessionIndex(LPWSTR name = NULL);
INT app_getCurrentIndex();
INT app_getPreviousIndex();
void app_resetPreviousIndex();
bool app_renameSession(INT si, LPWSTR newName);
LPCWSTR app_getSessionName(INT si = SI_CURRENT);
void app_getSessionFile(INT si, LPWSTR buf);
void app_showSessionInNppBars();

} // end namespace NppPlugin

#endif // NPP_PLUGIN_APPLICATION_H
