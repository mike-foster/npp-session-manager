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
    @file      Properties.cpp
    @copyright Copyright 2014,2015 Michael Foster <http://mfoster.com/npp/>

    Notepad++ saves a file's bookmarks (and other properties) in the session
    file, so the same file in different sessions will not have the same
    bookmarks. Session Manager can keep files' bookmarks (and a little more)
    synchronized across different sessions. These are referred to as "global
    properties". The file "global.xml", in the Session Manager configuration
    directory, stores bookmarks, firstVisibleLine and language for each unique
    pathname in all sessions.
*/

#include "System.h"
#include "Properties.h"
#include "Util.h"
#include "utf8\unchecked.h"

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

/// XML nodes
#define XN_NOTEPADPLUS    "NotepadPlus" ///< root node
#define XN_SESSION        "Session"
#define XN_MAINVIEW       "mainView"
#define XN_SUBVIEW        "subView"
#define XN_FILE           "File"
#define XN_MARK           "Mark"
#define XN_FOLD           "Fold"
#define XN_FILEPROPERTIES "FileProperties"

/// XML attributes
#define XA_FILENAME         "filename"
#define XA_LANG             "lang"
#define XA_FIRSTVISIBLELINE "firstVisibleLine"
#define XA_LINE             "line"

INT utf8ToAscii(LPCSTR str, LPSTR buf = NULL);
void removeMissingFilesFromGlobal();
void deleteChildren(tXmlEleP parent, LPCSTR eleName);

} // end namespace

//------------------------------------------------------------------------------

namespace api {

void prp_init()
{
    if (cfg::getBool(kCleanGlobalProperties)) {
        removeMissingFilesFromGlobal();
    }
}

} // end namespace NppPlugin::api

//------------------------------------------------------------------------------

namespace prp {

/** Updates global file properties from local (session) file properties.
    After a session is saved, the global bookmarks, firstVisibleLine and
    language are updated from the session properties. */
void updateGlobalFromSession(LPWSTR sesFile)
{
    DWORD lastErr;
    tXmlError xmlErr;
    LPCSTR target;

    LOGF("%S", sesFile);

    // Load the properties file (global file properties)
    tXmlDoc globalDoc;
    xmlErr = globalDoc.LoadFile(sys_getPropsFile());
    if (xmlErr != kXmlSuccess) {
        lastErr = ::GetLastError();
        msg::error(lastErr, L"%s: Error %i loading the global properties file.", _W(__FUNCTION__), xmlErr);
        return;
    }
    tXmlEleP globalPropsEle, globalFileEle, globalMarkEle, globalFoldEle;
    tXmlHnd globalDocHnd(&globalDoc);
    globalPropsEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).ToElement();

    // Load the session file (file properties local to a session)
    tXmlDoc localDoc;
    xmlErr = localDoc.LoadFile(sesFile);
    if (xmlErr != kXmlSuccess) {
        lastErr = ::GetLastError();
        msg::error(lastErr, L"%s: Error %i loading session file \"%s\".", _W(__FUNCTION__), xmlErr, sesFile);
        return;
    }
    tXmlEleP localViewEle, localFileEle, localMarkEle, localFoldEle;
    tXmlHnd localDocHnd(&localDoc);

    // Iterate over the local View elements
    localViewEle = localDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SESSION).FirstChildElement(XN_MAINVIEW).ToElement();
    while (localViewEle) {
        // Iterate over the local File elements
        localFileEle = localViewEle->FirstChildElement(XN_FILE);
        while (localFileEle) {
            // Find the global File element corresponding to the current local File element
            target = localFileEle->Attribute(XA_FILENAME);
            LOGG(21, "File = %s", target);
            globalFileEle = globalPropsEle->FirstChildElement(XN_FILE);
            while (globalFileEle) {
                if (globalFileEle->Attribute(XA_FILENAME, target)) {
                    break; // found it
                }
                globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
            }
            if (!globalFileEle) { // not found so create one
                globalFileEle = globalDoc.NewElement(XN_FILE);
                globalFileEle->SetAttribute(XA_FILENAME, target);
            }
            globalPropsEle->InsertFirstChild(globalFileEle); // an existing element will get moved to the top
            // Update global File attributes with values from the current local File attributes
            globalFileEle->SetAttribute(XA_LANG, localFileEle->Attribute(XA_LANG));
            globalFileEle->SetAttribute(XA_FIRSTVISIBLELINE, localFileEle->Attribute(XA_FIRSTVISIBLELINE));
            LOGG(21, "lang = '%s', firstVisibleLine = %s", localFileEle->Attribute(XA_LANG), localFileEle->Attribute(XA_FIRSTVISIBLELINE));
            // Iterate over the local Mark elements for the current local File element
            deleteChildren(globalFileEle, XN_MARK); // XXX globalFileEle->DeleteChildren();
            localMarkEle = localFileEle->FirstChildElement(XN_MARK);
            while (localMarkEle) {
                globalMarkEle = globalDoc.NewElement(XN_MARK);
                globalFileEle->InsertEndChild(globalMarkEle);
                // Update global Mark attributes with values from the current local Mark attributes
                globalMarkEle->SetAttribute(XA_LINE, localMarkEle->Attribute(XA_LINE));
                LOGG(21, "Mark = %s", localMarkEle->Attribute(XA_LINE));
                localMarkEle = localMarkEle->NextSiblingElement(XN_MARK);
            }
            // Iterate over the local Fold elements for the current local File element
            deleteChildren(globalFileEle, XN_FOLD); // XXX
            localFoldEle = localFileEle->FirstChildElement(XN_FOLD);
            while (localFoldEle) {
                globalFoldEle = globalDoc.NewElement(XN_FOLD);
                globalFileEle->InsertEndChild(globalFoldEle);
                // Update global Fold attributes with values from the current local Fold attributes
                globalFoldEle->SetAttribute(XA_LINE, localFoldEle->Attribute(XA_LINE));
                LOGG(21, "Fold = %s", localFoldEle->Attribute(XA_LINE));
                localFoldEle = localFoldEle->NextSiblingElement(XN_FOLD);
            }
            // Next local File element
            localFileEle = localFileEle->NextSiblingElement(XN_FILE);
        }
        localViewEle = localViewEle->NextSiblingElement(XN_SUBVIEW);
    }

    // Add XML declaration if missing
    if (memcmp(globalDoc.FirstChild()->Value(), "xml", 3) != 0) {
        globalDoc.InsertFirstChild(globalDoc.NewDeclaration());
    }
    // Save changes to the properties file
    xmlErr = globalDoc.SaveFile(sys_getPropsFile());
    if (xmlErr != kXmlSuccess) {
        lastErr = ::GetLastError();
        msg::error(lastErr, L"%s: Error %i saving the global properties file.", _W(__FUNCTION__), xmlErr);
    }
}

/** Updates local (session) file properties from global file properties.
    When a session is about to be loaded, the session bookmarks and language
    are updated from the global properties, then the session is loaded. */
void updateSessionFromGlobal(LPWSTR sesFile)
{
    LPSTR buf;
    DWORD lastErr;
    tXmlError xmlErr;
    bool save = false;
    LPCSTR target;

    LOGF("%S", sesFile);

    // Load the properties file (global file properties)
    tXmlDoc globalDoc;
    xmlErr = globalDoc.LoadFile(sys_getPropsFile());
    if (xmlErr != kXmlSuccess) {
        lastErr = ::GetLastError();
        msg::error(lastErr, L"%s: Error %i loading the global properties file.", _W(__FUNCTION__), xmlErr);
        return;
    }
    tXmlEleP globalPropsEle, globalFileEle, globalMarkEle, globalFoldEle;
    tXmlHnd globalDocHnd(&globalDoc);
    globalPropsEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).ToElement();

    // Load the session file (file properties local to a session)
    tXmlDoc localDoc;
    xmlErr = localDoc.LoadFile(sesFile);
    if (xmlErr != kXmlSuccess) {
        lastErr = ::GetLastError();
        msg::error(lastErr, L"%s: Error %i loading session file \"%s\".", _W(__FUNCTION__), xmlErr, sesFile);
        return;
    }
    tXmlEleP localViewEle, localFileEle, localMarkEle, localFoldEle;
    tXmlHnd localDocHnd(&localDoc);

    // Iterate over the local View elements
    localViewEle = localDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_SESSION).FirstChildElement(XN_MAINVIEW).ToElement();
    while (localViewEle) {
        // Iterate over the local File elements
        localFileEle = localViewEle->FirstChildElement(XN_FILE);
        while (localFileEle) {
            // Find the global File element corresponding to the current local File element
            target = localFileEle->Attribute(XA_FILENAME);
            LOGG(22, "File = %s", target);
            globalFileEle = globalPropsEle->FirstChildElement(XN_FILE);
            while (globalFileEle) {
                if (globalFileEle->Attribute(XA_FILENAME, target)) {
                    break; // found it
                }
                globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
            }
            if (globalFileEle) {
                save = true;
                // Update current local File attributes with values from the global File attributes
                buf = (LPSTR)sys_alloc(utf8ToAscii(target) * sizeof CHAR);
                if (buf == NULL) {
                    return;
                }
                utf8ToAscii(target, buf); // NPP expects the pathname to be encoded like this
                localFileEle->SetAttribute(XA_FILENAME, buf);
                sys_free(buf);
                localFileEle->SetAttribute(XA_LANG, globalFileEle->Attribute(XA_LANG));
                LOGG(22, "lang = '%s'", globalFileEle->Attribute(XA_LANG));
                // Iterate over the global Mark elements for the current global File element
                deleteChildren(localFileEle, XN_MARK); // XXX localFileEle->DeleteChildren();
                globalMarkEle = globalFileEle->FirstChildElement(XN_MARK);
                while (globalMarkEle) {
                    localMarkEle = localDoc.NewElement(XN_MARK);
                    localFileEle->InsertEndChild(localMarkEle);
                    // Update local Mark attributes with values from the current global Mark attributes
                    localMarkEle->SetAttribute(XA_LINE, globalMarkEle->Attribute(XA_LINE));
                    LOGG(22, "Mark = %s", globalMarkEle->Attribute(XA_LINE));
                    globalMarkEle = globalMarkEle->NextSiblingElement(XN_MARK);
                }
                // Iterate over the global Fold elements for the current global File element
                deleteChildren(localFileEle, XN_FOLD); // XXX
                globalFoldEle = globalFileEle->FirstChildElement(XN_FOLD);
                while (globalFoldEle) {
                    localFoldEle = localDoc.NewElement(XN_FOLD);
                    localFileEle->InsertEndChild(localFoldEle);
                    // Update local Fold attributes with values from the current global Fold attributes
                    localFoldEle->SetAttribute(XA_LINE, globalFoldEle->Attribute(XA_LINE));
                    LOGG(22, "Fold = %s", globalFoldEle->Attribute(XA_LINE));
                    globalFoldEle = globalFoldEle->NextSiblingElement(XN_FOLD);
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

    if (save) {
        // Add XML declaration if missing
        if (memcmp(localDoc.FirstChild()->Value(), "xml", 3) != 0) {
            localDoc.InsertFirstChild(localDoc.NewDeclaration());
        }
        // Save changes to the session file
        xmlErr = localDoc.SaveFile(sesFile);
        if (xmlErr != kXmlSuccess) {
            lastErr = ::GetLastError();
            msg::error(lastErr, L"%s: Error %i saving session file \"%s\".", _W(__FUNCTION__), xmlErr, sesFile);
        }
    }
}

/** Updates document properties from global file properties.
    When an existing document is added to a session, its bookmarks and
    firstVisibleLine are updated from the global properties. */
void updateDocumentFromGlobal(INT bufferId)
{
    LPSTR mbPathname;
    WCHAR pathname[MAX_PATH];
    INT line, pos, view;
    HWND hNpp = sys_getNppHandle();

    LOGF("%i", bufferId);

    // Get pathname for bufferId
    ::SendMessage(hNpp, NPPM_GETFULLPATHFROMBUFFERID, bufferId, (LPARAM)pathname);
    mbPathname = str::utf16ToUtf8(pathname);
    if (mbPathname == NULL) {
        return;
    }
    LOGG(20, "File = %s", mbPathname);
    // Load the properties file (global file properties)
    tXmlDoc globalDoc;
    tXmlError xmlErr = globalDoc.LoadFile(sys_getPropsFile());
    if (xmlErr != kXmlSuccess) {
        DWORD lastErr = ::GetLastError();
        msg::error(lastErr, L"%s: Error %i loading the global properties file.", _W(__FUNCTION__), xmlErr);
        sys_free(mbPathname);
        return;
    }
    tXmlEleP globalFileEle, globalMarkEle, globalFoldEle;
    tXmlHnd globalDocHnd(&globalDoc);
    globalFileEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).FirstChildElement(XN_FILE).ToElement();

    // Find the global File element corresponding to mbPathname
    while (globalFileEle) {
        if (globalFileEle->Attribute(XA_FILENAME, mbPathname)) {
            break; // found it
        }
        globalFileEle = globalFileEle->NextSiblingElement(XN_FILE);
    }
    sys_free(mbPathname);
    if (!globalFileEle) { // not found
        return;
    }

    // TODO: Need to set lang here

    // Determine containing view and tab for bufferId
    pos = ::SendMessage(hNpp, NPPM_GETPOSFROMBUFFERID, bufferId, 0);
    LOGG(20, "Pos = 0x%X", pos);
    view = (pos & (1 << 30)) == 0 ? 1 : 2;

    // Iterate over the global Mark elements and set them in the active document
    globalMarkEle = globalFileEle->FirstChildElement(XN_MARK);
    while (globalMarkEle) {
        line = globalMarkEle->IntAttribute(XA_LINE);
        // Go to line and set mark
        ::SendMessage(sys_getSciHandle(view), SCI_GOTOLINE, line, 0);
        ::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_SEARCH_TOGGLE_BOOKMARK);
        LOGG(20, "Mark = %i", line);
        globalMarkEle = globalMarkEle->NextSiblingElement(XN_MARK);
    }

    // Iterate over the global Fold elements and set them in the active document
    globalFoldEle = globalFileEle->FirstChildElement(XN_FOLD);
    while (globalFoldEle) {
        line = globalFoldEle->IntAttribute(XA_LINE);
        // Go to line and set fold
        ::SendMessage(sys_getSciHandle(view), SCI_GOTOLINE, line, 0);
        ::SendMessage(hNpp, NPPM_MENUCOMMAND, 0, IDM_VIEW_FOLD_CURRENT);
        LOGG(20, "Fold = %i", line);
        globalFoldEle = globalFoldEle->NextSiblingElement(XN_FOLD);
    }

    // Move cursor to the last known firstVisibleLine
    line = globalFileEle->IntAttribute(XA_FIRSTVISIBLELINE);
    ::SendMessage(sys_getSciHandle(view), SCI_GOTOLINE, line, 0);
    LOGG(20, "firstVisibleLine = %i", line);
}

} // end namespace NppPlugin::prp

//------------------------------------------------------------------------------

namespace {

/** If buf is non-NULL, converts a UTF-8 string to a string where all chars < 32
    or > 126 are converted to entities.
    @return the number of bytes in the converted string including the terminator */
INT utf8ToAscii(LPCSTR str, LPSTR buf)
{
    INT bytes = 0;
    LPSTR b = buf;
    LPCSTR s = str;
    utf8::uint32_t cp;
    while (*s) {
        cp = utf8::unchecked::next(s);
        if (cp < 32 || cp > 126) {
            if (buf) {
                ::sprintf_s(b, 9, "&#x%04X;", cp);
                b += 8;
            }
            bytes += 8;
        }
        else {
            if (buf) {
                *b++ = (unsigned char)cp;
            }
            ++bytes;
        }
    }
    if (buf) {
        *b = 0;
    }
    return bytes + 1;
}

/** Removes global File elements whose files do not exist on disk. */
void removeMissingFilesFromGlobal()
{
    INT wLen;
    DWORD lastErr;
    tXmlError xmlErr;
    bool save = false;
    LPWSTR wPathname;
    LPCSTR mbPathname;
    tXmlEleP propsEle, fileEle, currentFileEle;

    LOGF("");

    // Load the properties file (global file properties)
    tXmlDoc globalDoc;
    xmlErr = globalDoc.LoadFile(sys_getPropsFile());
    if (xmlErr != kXmlSuccess) {
        lastErr = ::GetLastError();
        msg::error(lastErr, L"%s: Error %i loading the global properties file.", _W(__FUNCTION__), xmlErr);
        return;
    }
    tXmlHnd globalDocHnd(&globalDoc);
    propsEle = globalDocHnd.FirstChildElement(XN_NOTEPADPLUS).FirstChildElement(XN_FILEPROPERTIES).ToElement();
    fileEle = propsEle->FirstChildElement(XN_FILE);

    // Iterate over the File elements and remove those whose files do not exist
    while (fileEle) {
        mbPathname = fileEle->Attribute(XA_FILENAME);
        wPathname = str::utf8ToUtf16(mbPathname);
        if (wPathname == NULL) {
            continue; // XXX was: return;
        }
        currentFileEle = fileEle;
        fileEle = fileEle->NextSiblingElement(XN_FILE);
        if (!pth::fileExists(wPathname)) {
            save = true;
            propsEle->DeleteChild(currentFileEle);
            LOGG(20, "File = %s", mbPathname);
        }
        sys_free(wPathname);
    }

    if (save) {
        // Add XML declaration if missing
        if (memcmp(globalDoc.FirstChild()->Value(), "xml", 3) != 0) {
            globalDoc.InsertFirstChild(globalDoc.NewDeclaration());
        }
        // Save changes to the properties file
        xmlErr = globalDoc.SaveFile(sys_getPropsFile());
        if (xmlErr != kXmlSuccess) {
            lastErr = ::GetLastError();
            msg::error(lastErr, L"%s: Error %i saving the global properties file.", _W(__FUNCTION__), xmlErr);
        }
    }
}

/** Deletes parent's child elements having the given element name. */
void deleteChildren(tXmlEleP parent, LPCSTR eleName)
{
    tXmlEleP ele, tmp;

    ele = parent->FirstChildElement(eleName);
    while (ele) {
        tmp = ele;
        ele = ele->NextSiblingElement(eleName);
        parent->DeleteChild(tmp);
    }
}

} // end namespace

} // end namespace NppPlugin
