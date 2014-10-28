/// @file
/*
    Properties.cpp
    Copyright 2014 Michael Foster (http://mfoster.com/npp/)

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

#include "System.h"
#include "Properties.h"
#include "Config.h"
#include "Util.h"
#include "xml\tinyxml2.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

/// XML nodes
#define XN_NOTEPADPLUS "NotepadPlus" ///< root node
#define XN_SESSION "Session"
#define XN_MAINVIEW "mainView"
#define XN_SUBVIEW "subView"
#define XN_FILE "File"
#define XN_MARK "Mark"
#define XN_FILEPROPERTIES "FileProperties"

/// XML attributes
#define XA_FILENAME "filename"
#define XA_LANG "lang"
#define XA_ENCODING "encoding"
#define XA_FIRSTVISIBLELINE "firstVisibleLine"
#define XA_LINE "line"

} // end namespace

//------------------------------------------------------------------------------
/// @namespace NppPlugin.prp Implements global file properties.

namespace prp {

/*
Example session file:

<NotepadPlus>
    <Session activeView="0">
        <mainView activeIndex="0">
            <File firstVisibleLine="444" xOffset="0" scrollWidth="1696" startPos="14583" endPos="14583" selMode="0" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\SessionMgr.cpp">
                <Mark line="312" />
                <Mark line="466" />
            </File>
        </mainView>
        <subView activeIndex="0">
            <File firstVisibleLine="451" xOffset="0" scrollWidth="1168" startPos="12528" endPos="12528" selMode="0" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\xml\tinyxml2.h">
                <Mark line="483" />
            </File>
        </subView>
    </Session>
</NotepadPlus>

Example file-properties.xml file:

<NotepadPlus>
    <FileProperties>
        <File firstVisibleLine="444" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\SessionMgr.cpp">
            <Mark line="312" />
            <Mark line="466" />
        </File>
        <File firstVisibleLine="451" lang="C++" encoding="-1" filename="C:\prj\npp-session-manager_global-marks\src\xml\tinyxml2.h">
            <Mark line="483" />
        </File>
    </FileProperties>
</NotepadPlus>
*/

/** Updates global file properties from local (session) file properties.
    After a session is saved, the global bookmarks, firstVisibleLine, language
    and encoding are updated from the session properties. */
void updateGlobalFromSession(TCHAR *sesFile)
{
    const char *p;
    tinyxml2::XMLError err;

    LOGF("%S", sesFile);

    // Load the properties file (global file properties)
    tinyxml2::XMLDocument globalDoc;
    err = globalDoc.LoadFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *globalPropsEle, *globalFileEle, *globalMarkEle;
    tinyxml2::XMLHandle globalDocHnd(&globalDoc);
    globalPropsEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).ToElement();

    // Load the session file (file properties local to a session)
    size_t num;
    char mbSesFile[MAX_PATH_T2];
    ::wcstombs_s(&num, mbSesFile, MAX_PATH_T2, sesFile, _TRUNCATE);
    tinyxml2::XMLDocument localDoc;
    err = localDoc.LoadFile(mbSesFile);
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the session file: '%s'.", err, mbSesFile);
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *localViewEle, *localFileEle, *localMarkEle;
    tinyxml2::XMLHandle localDocHnd(&localDoc);

    // Iterate over the local View elements
    localViewEle = localDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SESSION).FirstChildElement(XN_MAINVIEW).ToElement();
    while (localViewEle) {
        // Iterate over the local File elements
        localFileEle = localViewEle->FirstChildElement(XN_FILE);
        while (localFileEle) {
            // Find the global File element corresponding to the current local File element
            p = localFileEle->Attribute(XA_FILENAME);
            LOGG(30, "File = %s", p);
            globalFileEle = globalPropsEle->FirstChildElement(XN_FILE);
            while (globalFileEle) {
                if (globalFileEle->Attribute(XA_FILENAME, p)) {
                    break; // found it
                }
                globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
            }
            if (!globalFileEle) { // not found so create one
                globalFileEle = globalDoc.NewElement(XN_FILE);
                globalFileEle->SetAttribute(XA_FILENAME, p);
            }
            globalPropsEle->InsertFirstChild(globalFileEle); // an existing element will get moved to the top
            // Update global File attributes with values from the current local File attributes
            globalFileEle->SetAttribute(XA_LANG, localFileEle->Attribute(XA_LANG));
            globalFileEle->SetAttribute(XA_ENCODING, localFileEle->Attribute(XA_ENCODING));
            globalFileEle->SetAttribute(XA_FIRSTVISIBLELINE, localFileEle->Attribute(XA_FIRSTVISIBLELINE));
            globalFileEle->DeleteChildren();
            LOGG(30, "lang = '%s', encoding = '%s', firstVisibleLine = %s", localFileEle->Attribute(XA_LANG), localFileEle->Attribute(XA_ENCODING), localFileEle->Attribute(XA_FIRSTVISIBLELINE));
            // Iterate over the local Mark elements for the current local File element
            localMarkEle = localFileEle->FirstChildElement(XN_MARK);
            while (localMarkEle) {
                globalMarkEle = globalDoc.NewElement(XN_MARK);
                globalFileEle->InsertEndChild(globalMarkEle);
                // Update global Mark attributes with values from the current local Mark attributes
                globalMarkEle->SetAttribute(XA_LINE, localMarkEle->Attribute(XA_LINE));
                LOGG(30, "Mark = %s", localMarkEle->Attribute(XA_LINE));
                localMarkEle = localMarkEle->NextSiblingElement(XN_MARK);
            }
            localFileEle = localFileEle->NextSiblingElement(XN_FILE);
        }
        localViewEle = localViewEle->NextSiblingElement(XN_SUBVIEW);
    }

    // Save changes to the properties file
    err = globalDoc.SaveFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i saving the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
    }
}

/** Updates local (session) file properties from global file properties.
    When a session is about to be loaded, the session bookmarks, language and
    encoding are updated from the global properties, then the session is loaded. */
void updateSessionFromGlobal(TCHAR *sesFile)
{
    const char *p;
    bool save = false;
    tinyxml2::XMLError err;

    LOGF("%S", sesFile);

    // Load the properties file (global file properties)
    tinyxml2::XMLDocument globalDoc;
    err = globalDoc.LoadFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *globalPropsEle, *globalFileEle, *globalMarkEle;
    tinyxml2::XMLHandle globalDocHnd(&globalDoc);
    globalPropsEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).ToElement();

    // Load the session file (file properties local to a session)
    size_t num;
    char mbSesFile[MAX_PATH_T2];
    ::wcstombs_s(&num, mbSesFile, MAX_PATH_T2, sesFile, _TRUNCATE);
    tinyxml2::XMLDocument localDoc;
    err = localDoc.LoadFile(mbSesFile);
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the session file: '%s'.", err, mbSesFile);
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *localViewEle, *localFileEle, *localMarkEle;
    tinyxml2::XMLHandle localDocHnd(&localDoc);

    // Iterate over the local View elements
    localViewEle = localDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SESSION).FirstChildElement(XN_MAINVIEW).ToElement();
    while (localViewEle) {
        // Iterate over the local File elements
        localFileEle = localViewEle->FirstChildElement(XN_FILE);
        while (localFileEle) {
            // Find the global File element corresponding to the current local File element
            p = localFileEle->Attribute(XA_FILENAME);
            LOGG(30, "File = %s", p);
            globalFileEle = globalPropsEle->FirstChildElement(XN_FILE);
            while (globalFileEle) {
                if (globalFileEle->Attribute(XA_FILENAME, p)) {
                    break; // found it
                }
                globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
            }
            if (globalFileEle) {
                save = true;
                // Update current local File attributes with values from the global File attributes
                localFileEle->SetAttribute(XA_LANG, globalFileEle->Attribute(XA_LANG));
                localFileEle->SetAttribute(XA_ENCODING, globalFileEle->Attribute(XA_ENCODING));
                localFileEle->DeleteChildren();
                LOGG(30, "lang = '%s', encoding = '%s'", globalFileEle->Attribute(XA_LANG), globalFileEle->Attribute(XA_ENCODING));
                // Iterate over the global Mark elements for the current global File element
                globalMarkEle = globalFileEle->FirstChildElement(XN_MARK);
                while (globalMarkEle) {
                    localMarkEle = localDoc.NewElement(XN_MARK);
                    localFileEle->InsertEndChild(localMarkEle);
                    // Update local Mark attributes with values from the current global Mark attributes
                    localMarkEle->SetAttribute(XA_LINE, globalMarkEle->Attribute(XA_LINE));
                    LOGG(30, "Mark = %s", globalMarkEle->Attribute(XA_LINE));
                    globalMarkEle = globalMarkEle->NextSiblingElement(XN_MARK);
                }
            }
            //else {
            //    TODO: not found
            //    This indicates global needs to be updated from this session,
            //    but we can't call updateGlobalFromSession here.
            //}
            localFileEle = localFileEle->NextSiblingElement(XN_FILE);
        }
        localViewEle = localViewEle->NextSiblingElement(XN_SUBVIEW);
    }

    // Save changes to the session file
    if (save) {
        err = localDoc.SaveFile(mbSesFile);
        if (err != tinyxml2::XML_SUCCESS) {
            LOG("Error %i saving the session file: '%s'.", err, mbSesFile);
            SHOW_ERROR;
        }
    }
}

/** Updates document properties from global file properties.
    When an existing document is added to a session, its bookmarks and
    firstVisibleLine are updated from the global properties. */
void updateDocumentFromGlobal(INT bufferId)
{
    size_t num;
    INT line, pos, view;
    TCHAR pathname[MAX_PATH_P1];
    char mbPathname[MAX_PATH_T2_P1];
    HWND hNpp = sys_getNppHandle();

    LOGF("%i", bufferId);

    // Get pathname for bufferId
    ::SendMessage(hNpp, NPPM_GETFULLPATHFROMBUFFERID, bufferId, (LPARAM)pathname);
    ::wcstombs_s(&num, mbPathname, MAX_PATH_T2, pathname, _TRUNCATE);
    LOGG(20, "File = %s", mbPathname);

    // Load the properties file (global file properties)
    tinyxml2::XMLDocument globalDoc;
    tinyxml2::XMLError err = globalDoc.LoadFile(sys_getPropsFile());
    if (err != tinyxml2::XML_SUCCESS) {
        LOG("Error %i loading the properties file: '%s'.", err, sys_getPropsFile());
        SHOW_ERROR;
        return;
    }
    tinyxml2::XMLElement *globalFileEle, *globalMarkEle;
    tinyxml2::XMLHandle globalDocHnd(&globalDoc);
    globalFileEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).FirstChildElement(XN_FILE).ToElement();

    // Find the global File element corresponding to mbPathname
    while (globalFileEle) {
        if (globalFileEle->Attribute(XA_FILENAME, mbPathname)) {
            break; // found it
        }
        globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
    }
    if (!globalFileEle) { // not found
        return;
    }

    // TODO: If I knew how I would set lang and encoding here

    // Determine containing view and tab for bufferId
    pos = ::SendMessage(hNpp, NPPM_GETPOSFROMBUFFERID, bufferId, 0);
    LOGG(20, "Pos = 0x%X", pos);
    view = (pos & (1 << 30)) == 0 ? 1 : 2;

    // Iterate over the global Mark elements and set them in the active document
    globalMarkEle = globalFileEle->FirstChildElement(XN_MARK);
    while (globalMarkEle) {
        line = globalMarkEle->IntAttribute(XA_LINE);
        // go to line and set mark
        ::SendMessage(sys_getSciHandle(view), SCI_GOTOLINE, line, 0);
        ::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_SEARCH_TOGGLE_BOOKMARK);
        LOGG(20, "Mark = %i", line);
        globalMarkEle = globalMarkEle->NextSiblingElement(XN_MARK);
    }

    // Move cursor to the last known firstVisibleLine
    line = globalFileEle->IntAttribute(XA_FIRSTVISIBLELINE);
    ::SendMessage(sys_getSciHandle(view), SCI_GOTOLINE, line, 0);
    LOGG(20, "firstVisibleLine = %i", line);
}

} // end namespace prp

} // end namespace NppPlugin
