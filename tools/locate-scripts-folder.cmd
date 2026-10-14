@ECHO OFF
REM Run locate-scripts-folder.ps1 with the same arguments, output and exit codes.
REM Windows' default PowerShell execution policy blocks running .ps1 files
REM directly, so this bypasses it for this one script only.
REM
REM Usage: tools\locate-scripts-folder.cmd [-Identifier <name>] [-Verify]
SETLOCAL ENABLEEXTENSIONS
CD /D "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0locate-scripts-folder.ps1" %*
EXIT /B %ERRORLEVEL%
