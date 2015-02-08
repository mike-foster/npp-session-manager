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
    @file      Settings.cpp
    @copyright Copyright 2014,2015 Michael Foster <http://mfoster.com/npp/>

    Implements management of configuration settings.
*/

#include "System.h"
#include "SessionMgr.h"
#include "Menu.h"
#include "Util.h"
#include <strsafe.h>
#include <shlobj.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

INT gDbgLvl = 0;

//------------------------------------------------------------------------------

namespace {

/// Initial contents of a new settings.xml file
#define INITIAL_CONTENTS "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<SessionMgr/>\n"
/// XML node and attribute names
#define XN_ROOT  "SessionMgr"
#define XN_ITEM  "item"
#define XA_VALUE "value"
/// Defaults
#define DEFAULT_SES_DIR  L"sessions\\"
#define DEFAULT_SES_EXT  L".npp-session"

tXmlDocP _xmlDocument = NULL;
WCHAR _tmpBuffer[MAX_PATH];
bool _isDirty = false;

/** These must be in the same order as the ContainerId enums. */
LPCSTR _containerNames[] = {
    "Settings",
    "Favorites",
    "Filters"
};

tXmlEleP _containerElements[kContainersCount];

typedef struct Setting_tag {
    LPSTR    cName;
    LPSTR    cDefault;
    bool     isInt;
    INT      iCache;
    LPWSTR   wCache;
    tXmlEleP element;
    INT      wCacheSize; // character size of the wCache buffer, 0 if isInt
} Setting;

/** These must be in the same order as the SettingId enums. */
Setting _settings[] = {
    { "automaticSave",        "1",                true,  0, 0, 0, 0 },
    { "automaticLoad",        "0",                true,  0, 0, 0, 0 },
    { "loadIntoCurrent",      "0",                true,  0, 0, 0, 0 },
    { "loadWithoutClosing",   "0",                true,  0, 0, 0, 0 },
    { "showInTitlebar",       "0",                true,  0, 0, 0, 0 },
    { "showInStatusbar",      "0",                true,  0, 0, 0, 0 },
    { "useGlobalProperties",  "1",                true,  0, 0, 0, 0 },
    { "cleanGlobalProperties","0",                true,  0, 0, 0, 0 },
    { "useContextMenu",       "1",                true,  0, 0, 0, 0 },
    { "backupOnStartup",      "1",                true,  0, 0, 0, 0 },
    { "sessionSaveDelay",     "3",                true,  0, 0, 0, 0 },
    { "settingsSavePoll",     "2",                true,  0, 0, 0, 0 },
    { "sessionDirectory",     "",                 false, 0, 0, 0, MAX_PATH },
    { "sessionExtension",     ".npp-session",     false, 0, 0, 0, MAX_PATH }, // 25 },
    { "currentMark",          "9674",             true,  0, 0, 0, 0 },
    { "currentFavMark",       "9830",             true,  0, 0, 0, 0 },
    { "previousMark",         "9702",             true,  0, 0, 0, 0 },
    { "previousFavMark",      "8226",             true,  0, 0, 0, 0 },
    { "defaultMark",          "9653",             true,  0, 0, 0, 0 },
    { "defaultFavMark",       "9652",             true,  0, 0, 0, 0 },
    { "favoriteMark",         "183",              true,  0, 0, 0, 0 },
    { "useFilterWildcards",   "0",                true,  0, 0, 0, 0 },
    { "sessionSortOrder",     "1",                true,  0, 0, 0, 0 },
    { "currentSession",       "Default",          false, 0, 0, 0, MAX_PATH }, // SES_NAME_BUF_LEN },
    { "previousSession",      "Default",          false, 0, 0, 0, MAX_PATH }, // SES_NAME_BUF_LEN },
    { "defaultSession",       "Default",          false, 0, 0, 0, MAX_PATH }, // SES_NAME_BUF_LEN },
    { "menuLabelMain",        "&Session Manager", false, 0, 0, 0, MAX_PATH }, // MNU_MAX_NAME_LEN + 1 },
    { "menuLabelSub1",        "&Sessions...",     false, 0, 0, 0, MAX_PATH }, // MNU_MAX_NAME_LEN + 1 },
    { "menuLabelSub2",        "Se&ttings...",     false, 0, 0, 0, MAX_PATH }, // MNU_MAX_NAME_LEN + 1 },
    { "menuLabelSub3",        "Sa&ve current",    false, 0, 0, 0, MAX_PATH }, // MNU_MAX_NAME_LEN + 1 },
    { "menuLabelSub4",        "Load &previous",   false, 0, 0, 0, MAX_PATH }, // MNU_MAX_NAME_LEN + 1 },
    { "menuLabelSub5",        "&Help",            false, 0, 0, 0, MAX_PATH }, // MNU_MAX_NAME_LEN + 1 },
    { "menuLabelSub6",        "&About...",        false, 0, 0, 0, MAX_PATH }, // MNU_MAX_NAME_LEN + 1 },
    { "sessionsDialogWidth",  "0",                true,  0, 0, 0, 0 },
    { "sessionsDialogHeight", "0",                true,  0, 0, 0, 0 },
    { "settingsDialogWidth",  "0",                true,  0, 0, 0, 0 },
    { "settingsDialogHeight", "0",                true,  0, 0, 0, 0 },
    { "debugLogLevel",        "0",                true,  0, 0, 0, 0 },
    { "debugLogFile",         "",                 false, 0, 0, 0, MAX_PATH }
};

bool readSettingsFile();
void initContainers();
void initSettings();
void updateCache(Setting *setting);
void afterLoad();
void upgradeIniToXml();

} // end namespace

//------------------------------------------------------------------------------

namespace api {

void cfg_onUnload()
{
    INT i;

    if (_isDirty) {
        cfg::saveSettings();
    }
    for (i = 0; i < kSettingsCount; ++i) {
        if (!_settings[i].isInt && _settings[i].wCache) {
            sys_free(_settings[i].wCache);
        }
    }
    for (i = 0; i < kContainersCount; ++i) {
        _containerElements[i] = NULL;
    }
    if (_xmlDocument) {
        delete _xmlDocument;
        _xmlDocument = NULL;
    }
}

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

namespace cfg {

void loadSettings()
{
    if (readSettingsFile()) {
        initContainers();
        initSettings();
        afterLoad();
        if (_isDirty) {
            saveSettings();
        }
    }
}

void saveSettings()
{
    DWORD lastErr;
    tXmlError xmlErr;

    if (_xmlDocument) {
        xmlErr = _xmlDocument->SaveFile(sys_getSettingsFile());
        if (xmlErr != kXmlSuccess) {
            lastErr = ::GetLastError();
            msg::error(lastErr, L"%s: Error %u saving the settings file.", _W(__FUNCTION__), xmlErr);
        }
        else {
            _isDirty = false;
            LOG("Settings saved.");
        }
    }
}

bool isDirty()
{
    return _isDirty;
}

//------------------------------------------------------------------------------
// Functions that read or write child elements of the Settings container.

/** @return a pointer to the value of the cfgId element of the Settings container. */
LPCWSTR getStr(SettingId cfgId)
{
    return _settings[cfgId].wCache;
}

/** Copies to buf the value of the cfgId element of the Settings container. */
void getStr(SettingId cfgId, LPWSTR buf, INT bufLen)
{
    ::StringCchCopyW(buf, bufLen, _settings[cfgId].wCache);
}

/** @return the boolean value of the cfgId element of the Settings container. */
bool getBool(SettingId cfgId)
{
    return _settings[cfgId].iCache != 0;
}

/** @return the integer value of the cfgId element of the Settings container. */
INT getInt(SettingId cfgId)
{
    return _settings[cfgId].iCache;
}

/** Copies value to the cfgId element of the Settings container. */
void putStr(SettingId cfgId, LPCSTR value)
{
    if (value && *value) {
        _settings[cfgId].element->SetAttribute(XA_VALUE, value);
        updateCache(&_settings[cfgId]);
        _isDirty = true;
    }
}

/** Copies value to the cfgId element of the Settings container. */
void putStr(SettingId cfgId, LPCWSTR value)
{
    LPSTR mbValue = str::utf16ToUtf8(value);
    if (mbValue) {
        putStr(cfgId, mbValue);
        sys_free(mbValue);
    }
}

/** Writes the boolean value to the cfgId element of the Settings container. */
void putBool(SettingId cfgId, bool value)
{
    putStr(cfgId, value ? "1" : "0");
}

/** Writes the integer value to the cfgId element of the Settings container. */
void putInt(SettingId cfgId, INT value)
{
    CHAR buf[MAX_PATH];
    ::_itoa_s(value, buf, MAX_PATH, 10);
    putStr(cfgId, buf);
}

//------------------------------------------------------------------------------
// Functions that read or write child elements of any container.

/** @return a pointer to the value of the 0-based childIndex'th element of the
    conId container, or NULL if the element doesn't exist. */
LPCSTR getCStr(ContainerId conId, INT childIndex)
{
    tXmlEleP childEle;
    childEle = _containerElements[conId]->FirstChildElement();
    while (childEle && childIndex-- > 0) {
        childEle = childEle->NextSiblingElement();
    }
    if (childEle) {
        return childEle->Attribute(XA_VALUE);
    }
    return NULL;
}

/** @return a pointer to the value of the 0-based childIndex'th element of the
    conId container, or NULL if the element doesn't exist. It points to a
    temporary buffer which will be overwritten the next time this function
    is called. */
LPCWSTR getStr(ContainerId conId, INT childIndex)
{
    LPCSTR mbStr = getCStr(conId, childIndex);
    if (!mbStr) {
        return NULL;
    }
    return str::utf8ToUtf16(mbStr, _tmpBuffer, MAX_PATH);
}

/** Copies to buf the value of the 0-based childIndex'th element of the conId container.
    @return true if the element exists else false */
bool getStr(ContainerId conId, INT childIndex, LPWSTR buf, INT bufLen)
{
    LPCSTR mbStr = getCStr(conId, childIndex);
    if (!mbStr) {
        return false;
    }
    return str::utf8ToUtf16(mbStr, buf, bufLen) != NULL;
}

/** @return a pointer to the first child of conId with value, else NULL */
tXmlEleP getChild(ContainerId conId, LPCSTR value)
{
    if (value && *value) {
        tXmlEleP childEle = _containerElements[conId]->FirstChildElement();
        while (childEle) {
            if (childEle->Attribute(XA_VALUE, value)) {
                return childEle;
            }
            childEle = childEle->NextSiblingElement();
        }
    }
    return NULL;
}

/** Adds a new element to the conId container and copies value to it. Appends
    if append is true else prepends. Does nothing if conId is kSettings, or
    value is null or empty, or a child with value already exists. */
void addChild(ContainerId conId, LPCWSTR value, bool append)
{
    if (conId != kSettings && value && *value) {
        LPSTR mbValue = str::utf16ToUtf8(value);
        if (mbValue && !getChild(conId, mbValue)) {
            tXmlEleP cfgEle = _xmlDocument->NewElement(XN_ITEM);
            cfgEle->SetAttribute(XA_VALUE, mbValue);
            if (append) {
                _containerElements[conId]->InsertEndChild(cfgEle);
            }
            else {
                _containerElements[conId]->InsertFirstChild(cfgEle);
            }
            sys_free(mbValue);
            _isDirty = true;
        }
    }
}

/** If a child of conId with value exists, moves it to the top, else adds a new
    element at the top and copies value to it. If it already exists at the top,
    does nothing. Does nothing if conId is kSettings or value is null or empty.
    @return true if any change was made, else false */
bool moveToTop(ContainerId conId, LPCWSTR value)
{
    if (conId != kSettings && value && *value) {
        LPSTR mbValue = str::utf16ToUtf8(value);
        if (mbValue) {
            tXmlEleP childEle = getChild(conId, mbValue);
            sys_free(mbValue);
            if (!childEle) { // not found so add it at the top
                addChild(conId, value, false);
                return true;
            }
            if (childEle->PreviousSiblingElement()) { // found and not at top so move it to the top
                _containerElements[conId]->InsertFirstChild(childEle);
                _isDirty = true;
                return true;
            }
        }
    }
    return false;
}

/** Deletes all child elements of the conId container. Does nothing if conId
    is kSettings. */
void deleteChildren(ContainerId conId)
{
    if (conId != kSettings) {
        _containerElements[conId]->DeleteChildren();
        _isDirty = true;
    }
}

//------------------------------------------------------------------------------
// Application-specific functions built on top of the generic cfg functions.

bool setSessionDirectory(LPCWSTR sesDir, bool confirmDefSes)
{
    WCHAR buf[MAX_PATH];

    if (!sesDir || !*sesDir) {
        ::StringCchCopyW(buf, MAX_PATH, sys_getCfgDir());
        ::StringCchCatW(buf, MAX_PATH, DEFAULT_SES_DIR);
    }
    else {
        ::StringCchCopyW(buf, MAX_PATH, sesDir);
        pth::appendSlash(buf, MAX_PATH);
        if (!pth::dirExists(buf)) {
            if (::SHCreateDirectoryExW(NULL, buf, NULL) != ERROR_SUCCESS ) {
                DWORD le = ::GetLastError();
                msg::error(le, L"%s: Error creating session directory \"%s\".", _W(__FUNCTION__), buf);
                return false; // ses dir not changed
            }
        }
    }
    putStr(kSessionDirectory, buf);
    /* TODO: What if both kSessionDirectory and kSessionExtension are changing?
    Then app_confirmDefaultSession doesn't need to be called until after both have changed. */
    if (confirmDefSes) {
        app_confirmDefaultSession();
    }
    return true;
}

void setSessionExtension(LPCWSTR sesExt, bool confirmDefSes)
{
    WCHAR buf[MAX_PATH];

    if (!sesExt || !*sesExt) {
        ::StringCchCopyW(buf, MAX_PATH, DEFAULT_SES_EXT);
    }
    else {
        buf[0] = L'\0';
        if (*sesExt != L'.') {
            buf[0] = L'.';
            buf[1] = L'\0';
        }
        ::StringCchCatW(buf, MAX_PATH, sesExt);
    }
    putStr(kSessionExtension, buf);
    if (confirmDefSes) {
        app_confirmDefaultSession();
    }
}

void setShowInTitlebar(bool enable)
{
    putBool(kShowInTitlebar, enable);
    if (enable) {
        app_updateNppBars();
    }
}

void setShowInStatusbar(bool enable)
{
    putBool(kShowInStatusbar, enable);
    if (enable) {
        app_updateNppBars();
    }
}

void getMarkStr(SettingId cfgId, LPWSTR buf)
{
    buf[0] = getInt(cfgId);
    buf[1] = L'\t';
    buf[2] = 0;
}

bool isSortAlpha()
{
    return getInt(kSessionSortOrder) == SORT_ORDER_ALPHA;
}

bool isFavorite(LPCWSTR fav)
{
    bool isFav = false;
    LPSTR mbFav = str::utf16ToUtf8(fav);
    if (mbFav) {
        isFav = getChild(kFavorites, mbFav) != NULL;
        sys_free(mbFav);
    }
    return isFav;
}

} // end namespace NppPlugin::cfg

//------------------------------------------------------------------------------

namespace {

/** Loads the settings.xml file if it has not already been loaded. Creates it if
    it doesn't exist. */
bool readSettingsFile()
{
    DWORD lastErr;
    tXmlError xmlErr;
    LPCWSTR settingsFile;

    if (!_xmlDocument) {
        settingsFile = sys_getSettingsFile();
        if (!pth::fileExists(settingsFile)) {
            pth::createFileIfMissing(settingsFile, INITIAL_CONTENTS);
            _isDirty = true;
        }
        _xmlDocument = new tinyxml2::XMLDocument();
        xmlErr = _xmlDocument->LoadFile(settingsFile);
        if (xmlErr != kXmlSuccess) {
            lastErr = ::GetLastError();
            msg::error(lastErr, L"%s: Error %u loading the settings file.", _W(__FUNCTION__), xmlErr);
            return false;
        }
    }

    return true;
}

/** Initializes the _containerElements array with pointers to the ContainerId
    elements. Creates missing elements. */
void initContainers()
{
    INT conId;
    tXmlEleP conEle, rootEle;

    rootEle = _xmlDocument->FirstChildElement(XN_ROOT);
    for (conId = 0; conId < kContainersCount; ++conId) {
        conEle = rootEle->FirstChildElement(_containerNames[conId]);
        if (!conEle) {
            conEle = _xmlDocument->NewElement(_containerNames[conId]);
            rootEle->InsertEndChild(conEle);
            _isDirty = true;
        }
        _containerElements[conId] = conEle;
    }
}

/** Initializes the _settings array. Creates missing elements. */
void initSettings()
{
    INT cfgId;
    tXmlEleP settingsEle, cfgEle, prvCfgEle = NULL;

    settingsEle = _containerElements[kSettings];
    for (cfgId = 0; cfgId < kSettingsCount; ++cfgId) {
        cfgEle = settingsEle->FirstChildElement(_settings[cfgId].cName);
        if (!cfgEle) {
            cfgEle = _xmlDocument->NewElement(_settings[cfgId].cName);
            cfgEle->SetAttribute(XA_VALUE, _settings[cfgId].cDefault);
            if (prvCfgEle) {
                settingsEle->InsertAfterChild(prvCfgEle, cfgEle);
            }
            else {
                settingsEle->InsertEndChild(cfgEle);
            }
            _isDirty = true;
        }
        if (!_settings[cfgId].isInt) {
            _settings[cfgId].wCache = (LPWSTR)sys_alloc(_settings[cfgId].wCacheSize * sizeof WCHAR);
            _settings[cfgId].wCache[0] = 0;
        }
        prvCfgEle = cfgEle;
        _settings[cfgId].element = cfgEle;
        updateCache(&_settings[cfgId]);
    }
}

/** Refreshes the cached value of the given setting. */
void updateCache(Setting *setting)
{
    LPCSTR value = setting->element->Attribute(XA_VALUE);
    if (!value || !*value) {
        value = setting->cDefault;
    }
    if (setting->isInt) {
        setting->iCache = ::atoi(value);
    }
    else {
         str::utf8ToUtf16(value, setting->wCache, setting->wCacheSize);
    }
}

/** Things that may need to be done after the configuration is loaded and initialized. */
void afterLoad()
{
    if (!*cfg::getStr(kSessionDirectory)) { // If needed, set default session directory.
        cfg::setSessionDirectory(NULL, false);
    }
    cfg::addChild(kFilters, L"*"); // Add "*" filter if it doesn't already exist.
    gDbgLvl = cfg::getInt(kDebugLogLevel); // Use a global for fastest access.
}

} // end namespace

} // end namespace NppPlugin
