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
    @copyright Copyright 2014 Michael Foster <http://mfoster.com/npp/>

    Manages a Session Manager submenu in the context (right-click) menu.
*/

#include "System.h"
#include "ContextMenu.h"
#include "Config.h"
#include "Menu.h"
#include "Util.h"
#include "xml\tinyxml2.h"

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

tinyxml2::XMLDocument *_pCtxXmlDoc = NULL;
tinyxml2::XMLElement *_pCtxLastFav = NULL;

tinyxml2::XMLElement* getFavSeparator();
tinyxml2::XMLElement* createContextMenu(tinyxml2::XMLElement *sciCtxMnuEle);
tinyxml2::XMLElement* newItemElement(LPCWSTR itemName = NULL);

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
    tinyxml2::XMLElement *ele, *sepEle, *favEle, *sciCtxMnuEle;

    if (gCfg.useContextMenuEnabled()) {
        CHAR mbMain[MNU_MAX_NAME_LEN];
        ::WideCharToMultiByte(CP_UTF8, 0, mnu_getMenuLabel(), -1, mbMain, MNU_MAX_NAME_LEN, NULL, NULL);
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
        _pCtxLastFav = NULL;
    }
}

/** Writes favName to the 1-based idx'th fav element in NPP's contextMenu.xml
    file. Does not save the file. */
void addFavorite(LPCWSTR favName)
{
    tinyxml2::XMLElement *sepEle, *favEle, *sciCtxMnuEle;

    if (gCfg.useContextMenuEnabled()) {
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
    tinyxml2::XMLError xmlErr;

    if (gCfg.useContextMenuEnabled()) {
        if (_pCtxXmlDoc) {
            xmlErr = _pCtxXmlDoc->SaveFile(sys_getContextMenuFile());
            if (xmlErr != tinyxml2::XML_SUCCESS) {
                lastErr = ::GetLastError();
                msg::error(lastErr, L"%s: Error %i saving the context menu file.", _W(__FUNCTION__), xmlErr);
            }
        }
    }
}

void unload()
{
    if (_pCtxXmlDoc) {
        delete _pCtxXmlDoc;
    }
    _pCtxXmlDoc = NULL;
}

} // end namespace NppPlugin::ctx

//------------------------------------------------------------------------------

namespace {

/** Creates the entire context menu if it is not found. Saves the file if any
    changes are made.
    @return a pointer to the separator element preceeding the favorite elements */
tinyxml2::XMLElement* getFavSeparator()
{
    DWORD lastErr;
    bool changed = false;
    tinyxml2::XMLError xmlErr;
    tinyxml2::XMLElement *sepEle;
    CHAR mbMain[MNU_MAX_NAME_LEN], mbAbout[MNU_MAX_NAME_LEN];

    // Load the contextMenu file if not already loaded
    if (!_pCtxXmlDoc) {
        _pCtxXmlDoc = new tinyxml2::XMLDocument();
        xmlErr = _pCtxXmlDoc->LoadFile(sys_getContextMenuFile());
        if (xmlErr != tinyxml2::XML_SUCCESS) {
            lastErr = ::GetLastError();
            msg::error(lastErr, L"%s: Error %i loading the context menu file.", _W(__FUNCTION__), xmlErr);
            return NULL;
        }
    }
    tinyxml2::XMLElement *sciCtxMnuEle, *itemEle;
    tinyxml2::XMLHandle ctxDocHnd(_pCtxXmlDoc);
    sciCtxMnuEle = ctxDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SCINTILLACONTEXTMENU).ToElement();

    // the main menu item label
    ::WideCharToMultiByte(CP_UTF8, 0, mnu_getMenuLabel(), -1, mbMain, MNU_MAX_NAME_LEN, NULL, NULL);
    // the About item label
    ::WideCharToMultiByte(CP_UTF8, 0, mnu_getMenuLabel(MNU_BASE_MAX_ITEMS - 1), -1, mbAbout, MNU_MAX_NAME_LEN, NULL, NULL);

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

    return sepEle;
}

/** Creates our context menu. Does not save the file.
    @return a pointer to the separator element preceeding the favorite elements */
tinyxml2::XMLElement* createContextMenu(tinyxml2::XMLElement *sciCtxMnuEle)
{
    tinyxml2::XMLElement *ele;

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
tinyxml2::XMLElement* newItemElement(LPCWSTR itemName)
{
    tinyxml2::XMLElement *ele;
    CHAR mbMain[MNU_MAX_NAME_LEN];

    ::WideCharToMultiByte(CP_UTF8, 0, mnu_getMenuLabel(), -1, mbMain, MNU_MAX_NAME_LEN, NULL, NULL);
    ele = _pCtxXmlDoc->NewElement(XN_ITEM);
    ele->SetAttribute(XA_FOLDERNAME, mbMain);
    if (!itemName) { // separator
        ele->SetAttribute(XA_ID, "0");
    }
    else {
        CHAR mbMainNoAmp[MNU_MAX_NAME_LEN], mbBuf[MNU_MAX_NAME_LEN], mbBufNoAmp[MNU_MAX_NAME_LEN];
        pth::removeAmp(mbMain, mbMainNoAmp);
        ::WideCharToMultiByte(CP_UTF8, 0, itemName, -1, mbBuf, MNU_MAX_NAME_LEN, NULL, NULL);
        pth::removeAmp(mbBuf, mbBufNoAmp);
        ele->SetAttribute(XA_PLUGINENTRYNAME, mbMainNoAmp);
        ele->SetAttribute(XA_ITEMNAMEAS, mbBuf);
        ele->SetAttribute(XA_PLUGINCOMMANDITEMNAME, mbBufNoAmp);
    }

    return ele;
}

} // end namespace

} // end namespace NppPlugin
