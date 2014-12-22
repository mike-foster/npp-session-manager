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
    @file      Menu.h
    @copyright Copyright 2011,2013,2014 Michael Foster <http://mfoster.com/npp/>

    The NPP "Plugins" menu entries for Session Manager.
*/

#ifndef NPP_PLUGIN_MENU_H
#define NPP_PLUGIN_MENU_H

//------------------------------------------------------------------------------

namespace NppPlugin {

const int MNU_BASE_MAX_ITEMS = 7;  ///< see _menuItemsCount
const int MNU_MAX_FAVS       = 20;
const int MNU_FIRST_FAV_IDX  = MNU_BASE_MAX_ITEMS + 1;
const int MNU_LAST_FAV_IDX   = MNU_BASE_MAX_ITEMS + MNU_MAX_FAVS;
const int MNU_MAX_NAME_LEN   = 63; ///< see nbChar in npp\PluginInterface.h

//------------------------------------------------------------------------------
/// @namespace NppPlugin::api Contains functions called only from DllMain.

namespace api {

void mnu_onLoad();
void mnu_onUnload();
void mnu_init();
FuncItem* mnu_getItems(INT *pNum);

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

LPCWSTR mnu_getMenuLabel(INT mnuIdx = -1);
bool mnu_isFavorite(LPCWSTR ses);
void mnu_clearFavorites();
void mnu_addFavorite(INT prpIdx, LPCWSTR favName);

} // end namespace NppPlugin

#endif // NPP_PLUGIN_MENU_H
