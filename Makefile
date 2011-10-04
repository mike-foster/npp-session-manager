#-------------------------------------------------------------------------------
# SessionMgr nmake file

# Build procedure:
# 1. Check that you have environment variables PATH, INCLUDE and LIB defined.
# 2. Modify "setenv.cmd" with your paths.
# 3. Open a DOS box in the SessionMgr root directory. Run setenv, then run nmake.

PRJ=SessionMgr

O=obj
S=src
R=src\res
N=src\npp

RESDEPS=$R\resource.rc $R\resource.h $R\version.rc2 $R\version.h
NPPDEPS=$N\PluginInterface.h $N\Scintilla.h $N\Notepad_plus_msgs.h $N\menuCmdID.h

CC=cl
CFLAGS=/O2 /EHs /GR- /MT /nologo /W3 /WX- /Gd /Gm- /Fo$O\ /fp:fast /wd4995 \
       /errorReport:none /Zc:wchar_t /DWIN32 /DNDEBUG /D_WINDOWS /D_USRDLL \
       /D_WINDLL /c

LD=link
LIBS=user32.lib shell32.lib
LFLAGS=/DLL /nologo /OUT:$O\$(PRJ).dll /INCREMENTAL:NO /MANIFEST:NO \
       /MACHINE:X86 /ERRORREPORT:NONE

RC=rc
RFLAGS=/nologo /fo$O\$(PRJ).res

#-------------------------------------------------------------------------------

default: init $(PRJ)

all: clean init $(PRJ)

$(PRJ): $O\$(PRJ).obj $O\Config.obj $O\DlgDelete.obj $O\DlgNew.obj \
        $O\DlgRename.obj $O\DlgSessions.obj $O\DlgSettings.obj $O\DllMain.obj \
        $O\Menu.obj $O\System.obj $O\Util.obj $O\$(PRJ).res
    $(LD) $(LFLAGS) $(LIBS) $?

$O\$(PRJ).obj: $S\$(@B).cpp $S\$(@B).h $(NPPDEPS)
    $(CC) $(CFLAGS) %s

$O\Config.obj: $S\$(@B).cpp $S\$(@B).h
    $(CC) $(CFLAGS) %s

$O\DlgDelete.obj: $S\$(@B).cpp $S\$(@B).h
    $(CC) $(CFLAGS) %s

$O\DlgNew.obj: $S\$(@B).cpp $S\$(@B).h
    $(CC) $(CFLAGS) %s

$O\DlgRename.obj: $S\$(@B).cpp $S\$(@B).h
    $(CC) $(CFLAGS) %s

$O\DlgSessions.obj: $S\$(@B).cpp $S\$(@B).h
    $(CC) $(CFLAGS) %s

$O\DlgSettings.obj: $S\$(@B).cpp $S\$(@B).h
    $(CC) $(CFLAGS) %s

$O\DllMain.obj: $S\$(@B).cpp
    $(CC) $(CFLAGS) %s

$O\Menu.obj: $S\$(@B).cpp $S\$(@B).h $(NPPDEPS) $(RESDEPS)
    $(CC) $(CFLAGS) %s

$O\System.obj: $S\$(@B).cpp $S\$(@B).h $(NPPDEPS)
    $(CC) $(CFLAGS) %s

$O\Util.obj: $S\$(@B).cpp $S\$(@B).h
    $(CC) $(CFLAGS) %s

$O\$(PRJ).res: $(RESDEPS)
    $(RC) $(RFLAGS) %s

#-------------------------------------------------------------------------------

init:
    if not exist $O\ mkdir $O

clean:
    if exist $O\ del $O\*.obj & del $O\$(PRJ).*

#-------------------------------------------------------------------------------
