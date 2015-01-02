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
    @file      Properties.h
    @copyright Copyright 2014,2015 Michael Foster <http://mfoster.com/npp/>
*/

#ifndef NPP_PLUGIN_PROPERTIES_H
#define NPP_PLUGIN_PROPERTIES_H

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------
/// @namespace NppPlugin::api Contains functions called only from DllMain.

namespace api {

void prp_init();

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------
/// @namespace NppPlugin::prp Implements global file properties.

namespace prp {

void updateGlobalFromSession(LPWSTR sesFile);
void updateSessionFromGlobal(LPWSTR sesFile);
void updateDocumentFromGlobal(INT bufferId);

} // end namespace NppPlugin::prp

} // end namespace NppPlugin

#endif // NPP_PLUGIN_PROPERTIES_H
