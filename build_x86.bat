@echo off
cd /d "%~dp0"

call:build vtfmerge
call:build vtfsplit
call:build vtfzapmain
call:build vtfzapmips
call:build vtfzapthumb
call:build vtfzapreflectivity
call:build vtfzapalpha
call:build vtfcopyalpha
call:build vtfpatch
call:build vtfgenmips
call:build vtfgenthumb
call:build vtfflags
call:build vtf72

exit /b %errorlevel%

:build
clang++ -O2 %1.cxx -o ..\%1.exe -llib\x86\VTFLib
exit /b 0
