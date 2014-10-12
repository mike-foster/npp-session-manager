/*
    resource.h
    Copyright 2011-2014 Michael Foster (http://mfoster.com/npp/)

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
#define IDD_SES_DLG             100
#define IDD_NEW_DLG             110
#define IDD_REN_DLG             120
#define IDD_DEL_DLG             130
#define IDD_CFG_DLG             140

// Control IDs
#define IDC_SES_LST_SES         1000
#define IDC_SES_CTX_CUR         1001
#define IDC_SES_CTX_PRV         1002
#define IDC_SES_BTN_LOAD        1003
#define IDC_SES_BTN_PRV         1004
#define IDC_SES_BTN_DEF         1005
#define IDC_SES_BTN_SAVE        1006
#define IDC_SES_BTN_NEW         1007
#define IDC_SES_BTN_REN         1008
#define IDC_SES_BTN_DEL         1009
#define IDC_SES_BTN_CANCEL      1010
#define IDC_SES_RAD_ALPHA       1011
#define IDC_SES_RAD_DATE        1012
#define IDC_SES_CHK_LIC         1013
#define IDC_SES_CHK_LWC         1014
#define IDC_NEW_ETX_NAME        1100
#define IDC_NEW_RAD_EMPTY       1101
#define IDC_NEW_RAD_COPY        1102
#define IDC_NEW_RAD_OPEN        1103
#define IDC_REN_ETX_NAME        1200
#define IDC_CFG_CHK_ASV         1400
#define IDC_CFG_CHK_ALD         1401
#define IDC_CFG_CHK_LIC         1402
#define IDC_CFG_CHK_LWC         1403
#define IDC_CFG_CHK_SITB        1404
#define IDC_CFG_CHK_SISB        1405
#define IDC_CFG_CHK_GBKM        1406
#define IDC_CFG_BTN_BRW         1407
#define IDC_CFG_ETX_DIR         1408
#define IDC_CFG_ETX_EXT         1409

// Common sizes
#define IDC_MAR_1               6
#define IDC_MAR_2               2
#define IDC_MAR_3               3
#define IDC_BTN_W               34
#define IDC_BTN_H               13
#define IDC_ETX_H               13
#define IDC_LTX_H               8
#define IDC_CHK_H               8

/* The Sessions dialog */

#define IDD_SES_W               187
#define IDD_SES_H               230

// Right and bottom offsets (for moving/resizing)
#define IDC_SES_CTX_WRO         IDC_MAR_1
#define IDC_SES_LST_WRO         (IDC_MAR_1 + IDC_BTN_W + IDC_MAR_1)
#define IDC_SES_LST_HBO         (IDC_MAR_1 + IDC_CHK_H + IDC_MAR_2 + IDC_CHK_H + IDC_MAR_1)
#define IDC_SES_BTN_XRO         (IDC_MAR_1 + IDC_BTN_W)
#define IDC_SES_OPT_R1_YBO      (IDC_MAR_1 + IDC_CHK_H + IDC_MAR_3 + IDC_CHK_H)
#define IDC_SES_OPT_R2_YBO      (IDC_MAR_1 + IDC_CHK_H)

#define IDC_SES_CTX_CUR_X       40
#define IDC_SES_CTX_CUR_Y       IDC_MAR_3
#define IDC_SES_CTX_CUR_W       (IDD_SES_W - IDC_SES_CTX_CUR_X - IDC_SES_CTX_WRO)
#define IDC_SES_CTX_CUR_H       IDC_LTX_H

#define IDC_SES_CTX_PRV_X       IDC_SES_CTX_CUR_X
#define IDC_SES_CTX_PRV_Y       (IDC_SES_CTX_CUR_Y + IDC_MAR_2 + IDC_SES_CTX_CUR_H)
#define IDC_SES_CTX_PRV_W       IDC_SES_CTX_CUR_W
#define IDC_SES_CTX_PRV_H       IDC_SES_CTX_CUR_H

#define IDC_SES_LST_SES_X       IDC_MAR_1
#define IDC_SES_LST_SES_Y       (IDC_SES_CTX_PRV_Y + IDC_SES_CTX_PRV_H + IDC_MAR_3)
#define IDC_SES_LST_SES_W       (IDD_SES_W - IDC_SES_LST_SES_X - IDC_SES_LST_WRO)
#define IDC_SES_LST_SES_H       (IDD_SES_H - IDC_SES_LST_SES_Y - IDC_SES_LST_HBO)

#define IDC_SES_OPT_R1_Y        (IDD_SES_H - IDC_SES_OPT_R1_YBO)
#define IDC_SES_OPT_R2_Y        (IDD_SES_H - IDC_SES_OPT_R2_YBO)

#define IDC_SES_BTN_X           (IDD_SES_W - IDC_SES_BTN_XRO)
#define IDC_SES_BTN_LOAD_Y      IDC_SES_LST_SES_Y
#define IDC_SES_BTN_PRV_Y       (IDC_SES_BTN_LOAD_Y + IDC_MAR_1 + IDC_BTN_H)
#define IDC_SES_BTN_DEF_Y       (IDC_SES_BTN_PRV_Y + IDC_MAR_1 + IDC_BTN_H)
#define IDC_SES_BTN_SAVE_Y      (IDC_SES_BTN_DEF_Y + IDC_MAR_1 + IDC_BTN_H)
#define IDC_SES_BTN_NEW_Y       (IDC_SES_BTN_SAVE_Y + IDC_MAR_1 + IDC_BTN_H)
#define IDC_SES_BTN_REN_Y       (IDC_SES_BTN_NEW_Y + IDC_MAR_1 + IDC_BTN_H)
#define IDC_SES_BTN_DEL_Y       (IDC_SES_BTN_REN_Y + IDC_MAR_1 + IDC_BTN_H)
#define IDC_SES_BTN_CANCEL_Y    (IDC_SES_BTN_DEL_Y + IDC_MAR_1 + IDC_BTN_H)

// Child dialogs of the Sessions dialog
#define IDD_CDLG_W              164
#define IDC_CDLG_TXT_W          150
#define IDC_CDLG_RAD_W          115

/* The Settings dialog */

#define IDD_CFG_W               311
#define IDD_CFG_H               120

// Right and bottom offsets (for moving/resizing)
#define IDC_CFG_ETX_WRO         IDC_MAR_1
#define IDC_CFG_BTN_YBO         (IDC_MAR_1 + IDC_BTN_H)
#define IDC_CFG_BTN_OK_XRO      (IDC_MAR_1 + IDC_BTN_W + IDC_MAR_1 + IDC_BTN_W)
#define IDC_CFG_BTN_CAN_XRO     (IDC_MAR_1 + IDC_BTN_W)

#define IDC_CFG_ETX_DIR_X       25
#define IDC_CFG_ETX_DIR_Y       50
#define IDC_CFG_ETX_DIR_W       (IDD_CFG_W - IDC_CFG_ETX_DIR_X - IDC_CFG_ETX_WRO)
#define IDC_CFG_ETX_DIR_H       IDC_ETX_H

#define IDC_CFG_ETX_EXT_X       IDC_MAR_1
#define IDC_CFG_ETX_EXT_Y       78
#define IDC_CFG_ETX_EXT_W       (IDD_CFG_W - IDC_CFG_ETX_EXT_X - IDC_CFG_ETX_WRO)
#define IDC_CFG_ETX_EXT_H       IDC_ETX_H

#define IDC_CFG_BTN_OK_X        (IDD_CFG_W - IDC_CFG_BTN_OK_XRO)
#define IDC_CFG_BTN_OK_Y        (IDD_CFG_H - IDC_CFG_BTN_YBO)
#define IDC_CFG_BTN_CAN_X       (IDD_CFG_W - IDC_CFG_BTN_CAN_XRO)
#define IDC_CFG_BTN_CAN_Y       IDC_CFG_BTN_OK_Y


