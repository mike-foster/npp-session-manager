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
    @file      SessionMgrApi.h
    @copyright Copyright 2014 Michael Foster <http://mfoster.com/npp/>

    Session Manager API

    Clients should send NPPM_MSGTOPLUGIN to NPP with wParam pointing
    to L"SessionMgr.dll" and lParam pointing to a SessionMgrApiData object.
*/

#ifndef NPP_PLUGIN_SESSIONMGRAPI_H
#define NPP_PLUGIN_SESSIONMGRAPI_H

#define SESMGR_NULL   0
#define SESMGR_OK    -1
#define SESMGR_ERROR -2
#define SESMGR_BUSY  -3

typedef struct SessionMgrApiData_tag {
    UINT  message;
    INT   iData;
    WCHAR wData[MAX_PATH];
} SessionMgrApiData;

/** Loads a session from the current sessions list.
    @pre  wData = session name, no path or extension
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_ERROR if not in the list, else SESMGR_OK */
#define SESMGRM_SES_LOAD     (WM_APP + 1)

/** Loads the previous session.
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK */
#define SESMGRM_SES_LOAD_PRV (WM_APP + 2)

/** Loads the default session.
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK */
#define SESMGRM_SES_LOAD_DEF (WM_APP + 3)

/** Saves the current session.
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK */
#define SESMGRM_SES_SAVE     (WM_APP + 4)

/** Gets the current session name, no path or extension.
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK
    @post wData = name */
#define SESMGRM_SES_GET_NAME (WM_APP + 5)

/** Gets the fully qualified name of the current session.
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK
    @post wData = fqn */
#define SESMGRM_SES_GET_FQN  (WM_APP + 6)

/** Gets the current session directory.
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK
    @post wData = directory */
#define SESMGRM_CFG_GET_DIR  (WM_APP + 7)

/** Gets the current session file extension.
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK
    @post wData = extension */
#define SESMGRM_CFG_GET_EXT  (WM_APP + 8)

/** Sets the current session directory. Creates the directory if needed, saves
    the settings file, then reloads the current sessions list.
    @pre  wData = directory
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_ERROR if directory creation failed, else SESMGR_OK */
#define SESMGRM_CFG_SET_DIR  (WM_APP + 9)

/** Sets the current session file extension. Saves the settings file then
    reloads the current sessions list.
    @pre  wData = extension
    @pre  iData = SESMGR_NULL
    @post iData = SESMGR_OK */
#define SESMGRM_CFG_SET_EXT  (WM_APP + 10)

#endif // NPP_PLUGIN_SESSIONMGRAPI_H
