@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
REM SHIFT also shifts %0, so take this folder first.
SET "cushyLintDir=%~dp0"
REM Paths are relative to the caller's directory, so make them absolute before the CD below. Use / throughout: a \
REM right before a closing quote would escape the quote for PikaCmd.
SET "args="
:nextArg
IF "%~1"=="" GOTO argsDone
SET "arg=%~1"
IF NOT "!arg:~1,1!"==":" IF NOT "!arg:~0,1!"=="\" IF NOT "!arg:~0,1!"=="/" SET "arg=%CD%\!arg!"
SET "arg=!arg:\=/!"
SET args=!args! "!arg!"
SHIFT
GOTO nextArg
:argsDone
CD /D "%cushyLintDir%"
CALL .\support\bootstrapPikaCmd.cmd || EXIT /B 1
.\build\PikaCmd.exe .\CushyLint.pika !args!
