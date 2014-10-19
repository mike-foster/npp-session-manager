@echo off

:: Usage: x [c] [b] [d]
:: where c = clean, b = build, d = deploy

set npp_plugin_dir=c:\bin\npp\plugins
set sessionmgr_old_ver=0.7.1
set sessionmgr_new_ver=0.8.2

set do_c=0
set do_b=0
set do_d=0
for %%a in (%*) do (
    if "%%a"=="c" set do_c=1
    if "%%a"=="b" set do_b=1
    if "%%a"=="d" set do_d=1
)

if %do_c% equ 1 goto clean
if %do_b% equ 1 goto build
if %do_d% equ 1 goto deploy
echo Nothing to do
goto silent_exit

:clean

echo Cleaning...
nmake clean
if %do_b% equ 1 goto build
if %do_d% equ 1 goto deploy
goto success_exit

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

:deploy

echo Deploying...
if exist %npp_plugin_dir%\SessionMgr.dll (
    if not exist %npp_plugin_dir%\SessionMgr-%sessionmgr_old_ver%.dll (
        ren %npp_plugin_dir%\SessionMgr.dll SessionMgr-%sessionmgr_old_ver%.dll
        if errorlevel 1 (
            echo Backup ERROR 1
            goto error_exit
        )
    )
    else (
        echo Backup ERROR 2
        goto error_exit
    )
)
if exist %npp_plugin_dir%\SessionMgr-%sessionmgr_old_ver%.dll (
    if not exist %npp_plugin_dir%\SessionMgr-%sessionmgr_old_ver%.dll.bak (
        ren %npp_plugin_dir%\SessionMgr-%sessionmgr_old_ver%.dll SessionMgr-%sessionmgr_old_ver%.dll.bak
        if errorlevel 1 (
            echo Backup ERROR 3
            goto error_exit
        )
    )
    else (
        echo Backup ERROR 4
        goto error_exit
    )
)

copy /y obj\SessionMgr.dll %npp_plugin_dir%\SessionMgr-%sessionmgr_new_ver%.dll
if errorlevel 1 (
    echo Deploy ERROR
    goto error_exit
)

:success_exit

echo SUCCESS
exit /b 0

:error_exit

exit /b 1

:silent_exit

exit /b 0
