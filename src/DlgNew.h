/*
    DlgNew.h
    Copyright 2011 Michael Foster (http://mfoster.com/npp/)

    This file is part of SessionMgr, A Plugin for Notepad++.

    SessionMgr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NPP_PLUGIN_DLGNEW_H
#define NPP_PLUGIN_DLGNEW_H

//------------------------------------------------------------------------------

namespace NppPlugin {

INT_PTR CALLBACK dlgNew_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
TCHAR* dlgNew_getLbNewName();

} // end namespace NppPlugin

#endif // NPP_PLUGIN_DLGNEW_H
