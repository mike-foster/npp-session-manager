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
    @copyright Copyright 2014,2015 Michael Foster <http://mfoster.com/npp/>

    Session Manager P2P API

    Clients should send NPPM_MSGTOPLUGIN to NPP with wParam pointing to
    L"SessionMgr.dll" and lParam pointing to a SessionMgrApiData object.
*/

#ifndef NPP_PLUGIN_SESSIONMGRAPI_H
#define NPP_PLUGIN_SESSIONMGRAPI_H

#define SM_NULL    0
#define SM_OK     -1
#define SM_BUSY   -2
#define SM_ERROR  -3
#define SM_INVMSG -4
#define SM_INVARG -5

/** This is compatible with casting to NPP's CommunicationInfo struct.
    @see npp\Notepad_plus_msgs.h */
struct SessionMgrApiData {
    long    message;         ///< one of the SMM_ message codes
    LPCWSTR caller;          ///< for NPP but not used as of v6.6.9
    INT     iData;           ///< input and output API usage
    WCHAR   wData[MAX_PATH]; ///< input or output API usage
};

enum SettingId {
    kAutomaticSave = 0,
    kAutomaticLoad,
    kLoadIntoCurrent,
    kLoadWithoutClosing,
    kShowInTitlebar,
    kShowInStatusbar,
    kUseGlobalProperties,
    kCleanGlobalProperties,
    kUseContextMenu,
    kBackupOnStartup,
    kSessionSaveDelay,
    kSettingsSavePoll,
    kSessionDirectory,
    kSessionExtension,
    kCurrentMark,
    kCurrentFavMark,
    kPreviousMark,
    kPreviousFavMark,
    kDefaultMark,
    kDefaultFavMark,
    kFavoriteMark,
    kUseFilterWildcards,
    kSessionSortOrder,
    kCurrentSession,
    kPreviousSession,
    kDefaultSession,
    kMenuLabelMain,
    kMenuLabelSub1,
    kMenuLabelSub2,
    kMenuLabelSub3,
    kMenuLabelSub4,
    kMenuLabelSub5,
    kMenuLabelSub6,
    kSessionsDialogWidth,
    kSessionsDialogHeight,
    kSettingsDialogWidth,
    kSettingsDialogHeight,
    kDebugLogLevel,
    kDebugLogFile,
    kSettingsCount
};

//------------------------------------------------------------------------------

/** Loads a session from the current sessions list.
    @pre  wData = session name, no path or extension
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY or SM_INVARG */
#define SMM_SES_LOAD     (WM_APP + 1)

/** Loads the previous session.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY */
#define SMM_SES_LOAD_PRV (WM_APP + 2)

/** Loads the default session.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY */
#define SMM_SES_LOAD_DEF (WM_APP + 3)

/** Saves the current session.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY */
#define SMM_SES_SAVE     (WM_APP + 4)

/** Gets the current session name, no path or extension.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY
    @post wData = name */
#define SMM_SES_GET_NAME (WM_APP + 5)

/** Gets the fully qualified name of the current session file.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY
    @post wData = fqn */
#define SMM_SES_GET_FQN  (WM_APP + 6)

/** Gets the integer value of a setting.
    @pre  iData    = SettingId
    @post iData    = SM_OK else SM_BUSY or SM_INVARG
    @post wData[0] = value of setting */
#define SMM_CFG_GET_INT  (WM_APP + 7)

/** Sets the integer value of a setting.
    @pre  iData    = SettingId
    @pre  wData[0] = value to set
    @post iData    = SM_OK else SM_BUSY or SM_INVARG */
#define SMM_CFG_PUT_INT  (WM_APP + 8)

/** Gets the string value of a setting.
    @pre  iData = SettingId
    @post iData = SM_OK else SM_BUSY or SM_INVARG
    @post wData = value of setting */
#define SMM_CFG_GET_STR  (WM_APP + 9)

/** Sets the string value of a setting.
    @pre  iData = SettingId
    @pre  wData = value to set
    @post iData = SM_OK else SM_BUSY, SM_INVARG or SM_ERROR */
#define SMM_CFG_PUT_STR  (WM_APP + 10)

/** Removes all favorites.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY */
#define SMM_FAV_CLR      (WM_APP + 11)

/** Adds or removes a session as a favorite.
    @pre  wData = session name, no path or extension
    @pre  iData = 0: remove, 1: add
    @post iData = SM_OK else SM_BUSY, SM_INVARG or SM_ERROR */
#define SMM_FAV_SET      (WM_APP + 12)

/** Removes all filters.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY */
#define SMM_FIL_CLR      (WM_APP + 13)

/** Adds a filter. It is moved to the top if it is already in the list.
    @pre  wData = filter
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY */
#define SMM_FIL_ADD      (WM_APP + 14)

/** Gets NPP's configuration directory.
    @pre  iData = SM_NULL
    @post iData = SM_OK else SM_BUSY or SM_ERROR
    @post wData = path */
#define SMM_NPP_CFG_DIR  (WM_APP + 15)


#endif // NPP_PLUGIN_SESSIONMGRAPI_H
