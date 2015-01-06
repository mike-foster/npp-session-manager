@echo off

set sesmgr_old_ver=1.2
set sesmgr_new_ver=1.2.1
set release_dir=c:\prj\npp-session-manager\dist\%sesmgr_new_ver%
set zip_exe="c:\Program Files\7-Zip\7z.exe"
set md5_exe=c:\bin\md5sums\md5sums.exe
set npp_plugin_dir=c:\bin\npp\plugins

set do_c=0
set do_b=0
set do_d=0
set do_r=0
set args="x%*"
if not %args:c=%==%args% set do_c=1
if not %args:b=%==%args% set do_b=1
if not %args:d=%==%args% set do_d=1
if not %args:r=%==%args% set do_r=1

if %do_c% equ 1 goto clean
if %do_b% equ 1 goto build
if %do_d% equ 1 goto deploy
if %do_r% equ 1 goto release

echo Usage: x (c|b|d|r)[ ]...
echo where c = clean, b = build, d = deploy, r = release
goto silent_exit

::------------------------------------------------------------------------------
:clean

echo Cleaning...
nmake clean
if %do_b% equ 1 goto build
if %do_d% equ 1 goto deploy
goto success_exit

::------------------------------------------------------------------------------
:build

echo Building...
nmake > nmake.log
if errorlevel 1 (
    echo Build ERROR
    type nmake.log
    goto error_exit
)
if %do_d% equ 1 goto deploy
goto success_exit

::------------------------------------------------------------------------------
:deploy

echo Deploying...
if exist %npp_plugin_dir%\SessionMgr.dll (
    if not exist %npp_plugin_dir%\SessionMgr-%sesmgr_old_ver%.dll (
        ren %npp_plugin_dir%\SessionMgr.dll SessionMgr-%sesmgr_old_ver%.dll
        if errorlevel 1 (
            echo Backup ERROR 1
            goto error_exit
        )
    ) else (
        echo Backup ERROR 2
        goto error_exit
    )
)
if exist %npp_plugin_dir%\SessionMgr-%sesmgr_old_ver%.dll (
    if not exist %npp_plugin_dir%\SessionMgr-%sesmgr_old_ver%.dll.bak (
        ren %npp_plugin_dir%\SessionMgr-%sesmgr_old_ver%.dll SessionMgr-%sesmgr_old_ver%.dll.bak
        if errorlevel 1 (
            echo Backup ERROR 3
            goto error_exit
        )
    ) else (
        echo Backup ERROR 4
        goto error_exit
    )
)
xcopy /y /q obj\SessionMgr.dll %npp_plugin_dir%\SessionMgr-%sesmgr_new_ver%.dll
if errorlevel 1 (
    echo Deploy ERROR
    goto error_exit
)
if %do_r% equ 1 goto release
goto success_exit

::------------------------------------------------------------------------------
:release

echo Releasing...
if exist %release_dir% (
    rmdir /s %release_dir%
)
xcopy /t /e /q dist\template %release_dir%\
if errorlevel 1 (
    echo Release ERROR 1
    goto error_exit
)
xcopy /q obj\SessionMgr.dll %release_dir%\plugin\
xcopy /q doc\* %release_dir%\plugin\doc\
%zip_exe% a -r -tzip %release_dir%\SessionMgr-%sesmgr_new_ver%-plugin.zip %release_dir%\plugin\*
if errorlevel 1 (
    echo Release ERROR 2
    goto error_exit
)
xcopy /q license.txt %release_dir%\source\npp-session-manager\
xcopy /q Makefile %release_dir%\source\npp-session-manager\
xcopy /q README %release_dir%\source\npp-session-manager\
xcopy /q setenv.cmd %release_dir%\source\npp-session-manager\
xcopy /q x.cmd %release_dir%\source\npp-session-manager\
xcopy /s /q src\* %release_dir%\source\npp-session-manager\src\
xcopy /q doc\* %release_dir%\source\npp-session-manager\doc\
%zip_exe% a -r -tzip %release_dir%\SessionMgr-%sesmgr_new_ver%-source.zip %release_dir%\source\*
if errorlevel 1 (
    echo Release ERROR 3
    goto error_exit
)
%md5_exe% %release_dir%\*.zip > %release_dir%\md5sums.txt

::------------------------------------------------------------------------------

:success_exit

echo SUCCESS
exit /b 0

:error_exit

exit /b 1

:silent_exit

exit /b 0
