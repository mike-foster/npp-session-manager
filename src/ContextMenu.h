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
    @file      ContextMenu.h
    @copyright Copyright 2014,2015 Michael Foster <http://mfoster.com/npp/>
*/

#ifndef NPP_PLUGIN_CONTEXTMENU_H
#define NPP_PLUGIN_CONTEXTMENU_H

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------
/// @namespace NppPlugin::api Contains functions called only from DllMain.

namespace api {

void ctx_onUnload();

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------
/** @namespace NppPlugin::ctx Contains functions for creating the context menu
    items and managing the favorites there. */

namespace ctx {

void deleteFavorites();
void addFavorite(LPCWSTR favName);
void saveContextMenu();
void unload();

} // end namespace NppPlugin::ctx

} // end namespace NppPlugin

#endif // NPP_PLUGIN_CONTEXTMENU_H
