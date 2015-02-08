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
    @file      ContextMenu.cpp
    @copyright Copyright 2014,2015 Michael Foster <http://mfoster.com/npp/>

    Session Manager creates a submenu in the context (right-click) menu if the
    useContextMenu setting is enabled. The submenu is only created or updated
    when a change is made to favorite sessions. Notepad++ must be restarted for
    the changes to appear in the menus. The menu labels used in the context
    submenu are the same as those used in the Plugins menu and favorite sessions
    are listed after the About item.
*/

#include "System.h"
#include "ContextMenu.h"
#include "Menu.h"
#include "Util.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

/// XML nodes
#define XN_NOTEPADPLUS          "NotepadPlus" ///< root node
#define XN_SCINTILLACONTEXTMENU "ScintillaContextMenu"
#define XN_ITEM                 "Item"

/// XML attributes
#define XA_FOLDERNAME            "FolderName"
#define XA_PLUGINENTRYNAME       "PluginEntryName"
#define XA_PLUGINCOMMANDITEMNAME "PluginCommandItemName"
#define XA_ITEMNAMEAS            "ItemNameAs"
#define XA_ID                    "id"

tXmlDocP _pCtxXmlDoc = NULL;
tXmlEleP _pCtxLastFav = NULL;

tXmlEleP getFavSeparator();
tXmlEleP createContextMenu(tXmlEleP sciCtxMnuEle);
tXmlEleP newItemElement(LPCWSTR itemName = NULL);

} // end namespace

//------------------------------------------------------------------------------

namespace api {

void ctx_onUnload()
{
    ctx::unload();
}

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

namespace ctx {

/** Deletes all favorite Item elements. Does not save the file after deleting.
    @return a pointer to the separator element preceeding the favorite elements */
void deleteFavorites()
{
    tXmlEleP ele, sepEle, favEle, sciCtxMnuEle;

    if (cfg::getBool(kUseContextMenu)) {
        LPSTR mbMain = str::utf16ToUtf8(mnu_getMenuLabel());
        if (mbMain) {
            sepEle = getFavSeparator();
            if (sepEle) {
                sciCtxMnuEle = sepEle->Parent()->ToElement();
                favEle = sepEle->NextSiblingElement(XN_ITEM);
                while (favEle && favEle->Attribute(XA_FOLDERNAME, mbMain)) {
                    ele = favEle;
                    favEle = favEle->NextSiblingElement(XN_ITEM);
                    sciCtxMnuEle->DeleteChild(ele);
                }
            }
            sys_free(mbMain);
        }
        _pCtxLastFav = NULL;
    }
}

/** Adds a new favorite element in NPP's contextMenu.xml file. Does not save the file. */
void addFavorite(LPCWSTR favName)
{
    tXmlEleP sepEle, favEle, sciCtxMnuEle;

    if (cfg::getBool(kUseContextMenu)) {
        if (!_pCtxLastFav) {
            sepEle = getFavSeparator();
            if (sepEle) {
                favEle = newItemElement(favName);
                sciCtxMnuEle = sepEle->Parent()->ToElement();
                sciCtxMnuEle->InsertAfterChild(sepEle, favEle);
                _pCtxLastFav = favEle;
            }
        }
        else {
            favEle = newItemElement(favName);
            sciCtxMnuEle = _pCtxLastFav->Parent()->ToElement();
            sciCtxMnuEle->InsertAfterChild(_pCtxLastFav, favEle);
            _pCtxLastFav = favEle;
        }
    }
}

void saveContextMenu()
{
    DWORD lastErr;
    tXmlError xmlErr;

    if (cfg::getBool(kUseContextMenu)) {
        if (_pCtxXmlDoc) {
            xmlErr = _pCtxXmlDoc->SaveFile(sys_getContextMenuFile());
            if (xmlErr != kXmlSuccess) {
                lastErr = ::GetLastError();
                msg::error(lastErr, L"%s: Error %u saving the context menu file.", _W(__FUNCTION__), xmlErr);
            }
        }
    }
}

void unload()
{
    if (_pCtxXmlDoc) {
        delete _pCtxXmlDoc;
        _pCtxXmlDoc = NULL;
    }
}

} // end namespace NppPlugin::ctx

//------------------------------------------------------------------------------

namespace {

/** Creates the entire context menu if it is not found. Saves the file if any
    changes are made.
    @return a pointer to the separator element preceeding the favorite elements */
tXmlEleP getFavSeparator()
{
    DWORD lastErr;
    tXmlError xmlErr;
    bool changed = false;
    tXmlEleP sepEle = NULL;
    LPSTR mbMain, mbAbout;

    // Load the contextMenu file if not already loaded
    if (!_pCtxXmlDoc) {
        _pCtxXmlDoc = new tinyxml2::XMLDocument();
        xmlErr = _pCtxXmlDoc->LoadFile(sys_getContextMenuFile());
        if (xmlErr != kXmlSuccess) {
            lastErr = ::GetLastError();
            msg::error(lastErr, L"%s: Error %u loading the context menu file.", _W(__FUNCTION__), xmlErr);
            return NULL;
        }
    }
    tXmlEleP sciCtxMnuEle, itemEle;
    tXmlHnd ctxDocHnd(_pCtxXmlDoc);
    sciCtxMnuEle = ctxDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SCINTILLACONTEXTMENU).ToElement();

    // the main menu item label
    mbMain = str::utf16ToUtf8(mnu_getMenuLabel());
    // the About item label
    mbAbout = str::utf16ToUtf8(mnu_getMenuLabel(MNU_BASE_MAX_ITEMS - 1));
    if (mbMain && mbAbout) {
        // Iterate over the Item elements looking for our About item
        itemEle = sciCtxMnuEle->FirstChildElement(XN_ITEM);
        while (itemEle) {
            if (itemEle->Attribute(XA_FOLDERNAME, mbMain) && itemEle->Attribute(XA_ITEMNAMEAS, mbAbout)) {
                break; // found it
            }
            itemEle = itemEle->NextSiblingElement(XN_ITEM);
        }
        if (!itemEle) { // not found so need to create our entire context menu
            sepEle = createContextMenu(sciCtxMnuEle);
            changed = true;
        }
        else { // found it so check if a separator follows it, if not then add it
            sepEle = itemEle->NextSiblingElement(XN_ITEM);
            if (!sepEle->Attribute(XA_FOLDERNAME, mbMain) || !sepEle->Attribute(XA_ID, "0")) {
                sepEle = newItemElement();
                sciCtxMnuEle->InsertAfterChild(itemEle, sepEle);
                changed = true;
            }
        }
        if (changed) {
            ctx::saveContextMenu();
        }
        sys_free(mbMain);
        sys_free(mbAbout);
    }

    return sepEle;
}

/** Creates our context menu. Does not save the file.
    @return a pointer to the separator element preceeding the favorite elements */
tXmlEleP createContextMenu(tXmlEleP sciCtxMnuEle)
{
    tXmlEleP ele;

    for (INT mnuIdx = 0; mnuIdx < MNU_BASE_MAX_ITEMS; ++mnuIdx) {
        ele = newItemElement(mnuIdx == 4 ? NULL : mnu_getMenuLabel(mnuIdx));
        sciCtxMnuEle->InsertEndChild(ele);
    }
    // the separator preceeding the favorites
    ele = newItemElement();
    sciCtxMnuEle->InsertEndChild(ele);

    return ele;
}

/** @return a new Item element */
tXmlEleP newItemElement(LPCWSTR itemName)
{
    tXmlEleP ele = NULL;

    LPSTR mbMain = str::utf16ToUtf8(mnu_getMenuLabel());
    if (mbMain) {
        ele = _pCtxXmlDoc->NewElement(XN_ITEM);
        ele->SetAttribute(XA_FOLDERNAME, mbMain);
        if (!itemName) { // separator
            ele->SetAttribute(XA_ID, "0");
        }
        else {
            CHAR mbMainNoAmp[MNU_MAX_NAME_LEN], mbBufNoAmp[MNU_MAX_NAME_LEN];
            LPSTR mbBuf = str::utf16ToUtf8(itemName);
            if (mbBuf) {
                str::removeAmp(mbMain, mbMainNoAmp);
                str::removeAmp(mbBuf, mbBufNoAmp);
                ele->SetAttribute(XA_PLUGINENTRYNAME, mbMainNoAmp);
                ele->SetAttribute(XA_ITEMNAMEAS, mbBuf);
                ele->SetAttribute(XA_PLUGINCOMMANDITEMNAME, mbBufNoAmp);
                sys_free(mbBuf);
            }
        }
        sys_free(mbMain);
    }

    return ele;
}

} // end namespace

} // end namespace NppPlugin
