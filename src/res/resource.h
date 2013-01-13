/*
    resource.h
    Copyright 2011,2012,2013 Michael Foster (http://mfoster.com/npp/)

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

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Dialog IDs
#define IDD_SES_DLG              100
#define IDD_NEW_DLG              110
#define IDD_REN_DLG              120
#define IDD_DEL_DLG              130
#define IDD_CFG_DLG              140

// Control IDs
#define IDC_SES_LST_SES         1000
#define IDC_SES_TXT_CUR         1001
#define IDC_SES_TXT_PRV         1002
#define IDC_SES_BTN_LOAD        1003
#define IDC_SES_BTN_PRV         1004
#define IDC_SES_BTN_DEF         1005
#define IDC_SES_BTN_SAVE        1006
#define IDC_SES_BTN_NEW         1007
#define IDC_SES_BTN_REN         1008
#define IDC_SES_BTN_DEL         1009
#define IDC_SES_BTN_CANCEL      1010
#define IDC_SES_CHK_LIC         1011
#define IDC_SES_CHK_LWC         1012
#define IDC_NEW_EDT_NAME        1100
#define IDC_NEW_RAD_EMPTY       1101
#define IDC_NEW_RAD_COPY        1102
#define IDC_NEW_RAD_OPEN        1103
#define IDC_REN_EDT_NAME        1200
#define IDC_CFG_CHK_ASV         1400
#define IDC_CFG_CHK_ALD         1401
#define IDC_CFG_CHK_LIC         1402
#define IDC_CFG_CHK_LWC         1403
#define IDC_CFG_CHK_SITB        1404
#define IDC_CFG_CHK_SISB        1405
#define IDC_CFG_BTN_BRW         1406
#define IDC_CFG_EDT_DIR         1407
#define IDC_CFG_EDT_EXT         1408

// Positions and Sizes

#define IDC_ALL_MARGIN             7
#define IDC_ALL_BTN_W             34
#define IDC_ALL_BTN_H             14
#define IDC_ALL_TXT_W            150
#define IDC_ALL_TXT_H1            10
#define IDC_ALL_TXT_H2             8
#define IDC_ALL_EDT_H             13

#define IDD_SES_W                187
#define IDD_SES_H                220
#define IDC_SES_CHK_W            115
#define IDC_SES_CHK_H              9
#define IDC_SES_BTN_X            146

#define IDC_SES_TXT_CUR_Y          4
#define IDC_SES_TXT_PRV_Y        (IDC_SES_TXT_CUR_Y + IDC_ALL_TXT_H1)

#define IDC_SES_LST_SES_Y        (IDC_SES_TXT_PRV_Y + IDC_ALL_TXT_H1 + 3)
#define IDC_SES_LST_SES_W        134
#define IDC_SES_LST_SES_H        162
#define IDC_SES_CHK_LIC_Y        (IDC_SES_LST_SES_Y + IDC_SES_LST_SES_H + 5)
#define IDC_SES_CHK_LWC_Y        (IDC_SES_CHK_LIC_Y + IDC_SES_CHK_H + 3)

#define IDC_SES_BTN_LOAD_Y       IDC_SES_LST_SES_Y
#define IDC_SES_BTN_PRV_Y        (IDC_SES_BTN_LOAD_Y + IDC_ALL_MARGIN + IDC_ALL_BTN_H)
#define IDC_SES_BTN_DEF_Y        (IDC_SES_BTN_PRV_Y + IDC_ALL_MARGIN + IDC_ALL_BTN_H)
#define IDC_SES_BTN_SAVE_Y       (IDC_SES_BTN_DEF_Y + IDC_ALL_MARGIN + IDC_ALL_BTN_H)
#define IDC_SES_BTN_NEW_Y        (IDC_SES_BTN_SAVE_Y + IDC_ALL_MARGIN + IDC_ALL_BTN_H)
#define IDC_SES_BTN_REN_Y        (IDC_SES_BTN_NEW_Y + IDC_ALL_MARGIN + IDC_ALL_BTN_H)
#define IDC_SES_BTN_DEL_Y        (IDC_SES_BTN_REN_Y + IDC_ALL_MARGIN + IDC_ALL_BTN_H)
#define IDC_SES_BTN_CANCEL_Y     (IDC_SES_BTN_DEL_Y + IDC_ALL_MARGIN + IDC_ALL_BTN_H)

#define IDD_SUBDLG_W             164

#define IDC_NEW_RAD_W            115
#define IDC_NEW_RAD_H             12

#define IDD_CFG_W                311
#define IDD_CFG_H                120
#define IDC_CFG_CHK_W             47
#define IDC_CFG_CHK_W2            75
#define IDC_CFG_CHK_H             10
#define IDC_CFG_EDT_W            (IDD_CFG_W - IDC_ALL_MARGIN - IDC_ALL_MARGIN - 1)
#define IDC_CFG_BTN_X1           (IDD_CFG_W - IDC_ALL_MARGIN - IDC_ALL_BTN_W - IDC_ALL_MARGIN - IDC_ALL_BTN_W)
#define IDC_CFG_BTN_X2           (IDD_CFG_W - IDC_ALL_MARGIN - IDC_ALL_BTN_W)

