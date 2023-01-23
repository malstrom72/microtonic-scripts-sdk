@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
CD /D "%~dp0"
MKDIR ..\build >NUL 2>&1
CALL BuildCpp.cmd release 64 ..\build\MakaronCmd.exe /I .\ .\MakaronCmd.cpp .\Makaron.cpp
