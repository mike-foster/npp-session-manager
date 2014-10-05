#-------------------------------------------------------------------------------
# SessionMgr nmake file

# 1. Check that you have environment variables PATH, INCLUDE and LIB defined.
# 2. Modify "setenv.cmd" with your toolchain paths.
# 3. Open a DOS box in the directory containing this makefile.
# 4. Run setenv, then run nmake.

PRJ=SessionMgr

O=obj
S=src
R=src\res
N=src\npp
X=src\xml

# http://msdn.microsoft.com/en-us/library/fwkeyyhe.aspx
CXX=cl
CXXFLAGS=/O2 /EHs /GR- /MT /nologo /W3 /WX- /Gd /Gm- /Fo$O\ /fp:fast /wd4995 \
         /errorReport:none /Zc:wchar_t /DWIN32 /DNDEBUG /D_WINDOWS /D_USRDLL \
         /D_WINDLL /DUNICODE /D_UNICODE /c

# http://msdn.microsoft.com/en-us/library/y0zzbyt4.aspx
LD=link
LIBS=user32.lib shell32.lib
LDFLAGS=/DLL /nologo /OUT:$O\$(PRJ).dll /INCREMENTAL:NO /MANIFEST:NO \
        /MACHINE:X86 /ERRORREPORT:NONE

RC=rc
RCFLAGS=/nologo /fo$O\$(PRJ).res
RESDEPS=$R\resource.rc $R\resource.h $R\version.rc2 $R\version.h
NPPDEPS=$N\PluginInterface.h $N\Scintilla.h $N\Notepad_plus_msgs.h $N\menuCmdID.h

#-------------------------------------------------------------------------------
# Targets

default: init $(PRJ)

init:
    if not exist $O\ mkdir $O

clean:
    if exist $O\ del $O\*.obj & del $O\$(PRJ).*

#-------------------------------------------------------------------------------
# Link object files to make dll

$(PRJ): $O\$(PRJ).obj $O\Config.obj $O\DlgDelete.obj $O\DlgNew.obj \
        $O\DlgRename.obj $O\DlgSessions.obj $O\DlgSettings.obj $O\DllMain.obj \
        $O\Menu.obj $O\System.obj $O\Util.obj $O\tinyxml2.obj $O\$(PRJ).res
    $(LD) $(LDFLAGS) $(LIBS) $?

#-------------------------------------------------------------------------------
# Compile cpp files

$O\$(PRJ).obj: $S\$(@B).cpp $S\$(@B).h $(NPPDEPS)
    $(CXX) $(CXXFLAGS) %s

$O\Config.obj: $S\$(@B).cpp $S\$(@B).h
    $(CXX) $(CXXFLAGS) %s

$O\DlgDelete.obj: $S\$(@B).cpp $S\$(@B).h
    $(CXX) $(CXXFLAGS) %s

$O\DlgNew.obj: $S\$(@B).cpp $S\$(@B).h
    $(CXX) $(CXXFLAGS) %s

$O\DlgRename.obj: $S\$(@B).cpp $S\$(@B).h
    $(CXX) $(CXXFLAGS) %s

$O\DlgSessions.obj: $S\$(@B).cpp $S\$(@B).h
    $(CXX) $(CXXFLAGS) %s

$O\DlgSettings.obj: $S\$(@B).cpp $S\$(@B).h
    $(CXX) $(CXXFLAGS) %s

$O\DllMain.obj: $S\$(@B).cpp
    $(CXX) $(CXXFLAGS) %s

$O\Menu.obj: $S\$(@B).cpp $S\$(@B).h $(NPPDEPS) $(RESDEPS)
    $(CXX) $(CXXFLAGS) %s

$O\System.obj: $S\$(@B).cpp $S\$(@B).h $(NPPDEPS)
    $(CXX) $(CXXFLAGS) %s

$O\Util.obj: $S\$(@B).cpp $S\$(@B).h
    $(CXX) $(CXXFLAGS) %s

$O\tinyxml2.obj: $X\$(@B).cpp $X\$(@B).h
    $(CXX) $(CXXFLAGS) %s

#-------------------------------------------------------------------------------
# Compile resource files

$O\$(PRJ).res: $(RESDEPS)
    $(RC) $(RCFLAGS) %s

#-------------------------------------------------------------------------------
