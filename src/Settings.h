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
    @file      Settings.h
    @copyright Copyright 2014,2015 Michael Foster <http://mfoster.com/npp/>
*/

#ifndef NPP_PLUGIN_SETTINGS_H
#define NPP_PLUGIN_SETTINGS_H

#include "xml\tinyxml.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

extern INT gDbgLvl;

enum ContainerId {
    kSettings = 0,
    kFavorites,
    kFilters,
    kContainersCount
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

#define SORT_ORDER_ALPHA 1
#define SORT_ORDER_DATE  2
#define FILTER_BUF_LEN  50
#define FILTERS_MAX    100

//------------------------------------------------------------------------------
/// @namespace NppPlugin::api Contains functions called only from DllMain.

namespace api {

void cfg_onUnload();

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------
/// @namespace NppPlugin::cfg Implements management of configuration settings.

namespace cfg {

void loadSettings();
void saveSettings();
bool isDirty();

// Functions that read or write child elements of the Settings container.
LPCWSTR getStr(SettingId cfgId);
void getStr(SettingId cfgId, LPWSTR buf, INT bufLen);
bool getBool(SettingId cfgId);
INT getInt(SettingId cfgId);
void putStr(SettingId cfgId, LPCSTR value);
void putStr(SettingId cfgId, LPCWSTR value);
void putBool(SettingId cfgId, bool value);
void putInt(SettingId cfgId, INT value);

// Functions that read or write child elements of any container.
LPCSTR getCStr(ContainerId conId, INT childIndex);
LPCWSTR getStr(ContainerId conId, INT childIndex);
bool getStr(ContainerId conId, INT childIndex, LPWSTR buf, INT bufLen);
tXmlEleP getChild(ContainerId conId, LPCSTR value);
void addChild(ContainerId conId, LPCWSTR value, bool append = true);
bool moveToTop(ContainerId conId, LPCWSTR value);
void deleteChildren(ContainerId conId);

// Application-specific functions built on top of the generic cfg functions.
bool setSessionDirectory(LPCWSTR sesDir, bool confirmDefSes = true);
void setSessionExtension(LPCWSTR sesExt, bool confirmDefSes = true);
void setShowInTitlebar(bool enable);
void setShowInStatusbar(bool enable);
void getMarkStr(SettingId cfgId, LPWSTR buf);
bool isSortAlpha();
bool isFavorite(LPCWSTR fav);

} // end namespace NppPlugin::cfg

} // end namespace NppPlugin

#endif // NPP_PLUGIN_SETTINGS_H
