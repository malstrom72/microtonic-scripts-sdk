@ECHO OFF
REM Lint Sonic Charge script JavaScript (ES5 "script" syntax) with ESLint.
REM
REM Usage: tools\validate-js.cmd [file-or-directory ...]
REM
REM The Windows counterpart of tools/validate-js.sh, with the same arguments,
REM output and exit codes.
REM
REM Shared between the Microtonic and Synplant script SDKs. Only the product
REM configuration block below differs between the two copies.
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ---- Product configuration -------------------------------------------------
REM ESLint config file under tools\ and the console package linted by default.
SET "eslint_config=eslint.microtonic.config.mjs"
SET "console_package=JSConsole.mtscript"
REM ---- End of product configuration ------------------------------------------

REM ESLint used through npx when the SDK has no local node_modules\.bin\eslint.
REM Pinned to a major version so a new ESLint major cannot change results
REM unannounced; the configs are flat configs written for ESLint 10.
SET "eslint_package=eslint@10"

FOR %%R IN ("%~dp0..") DO SET "sdk_root=%%~fR"
SET "config=%sdk_root%\tools\%eslint_config%"

SET "local_eslint=%sdk_root%\node_modules\.bin\eslint.cmd"

SET "list="
SET count=0
SET status=0

REM With no arguments, validate the console package plus everything under
REM examples\ (packages, loose example scripts and subfolders). When paths are
REM supplied, keep the caller's working directory so files outside the SDK
REM checkout are validated instead of being ignored by ESLint's flat-config base
REM path.
IF "%~1"=="" (
	CD /D "%sdk_root%"
	CALL :addTarget "%console_package%"
	CALL :addTarget "examples"
	GOTO targetsDone
)
:parseArgs
IF "%~1"=="" GOTO targetsDone
CALL :addTarget "%~1"
SHIFT
GOTO parseArgs

:targetsDone
IF %count%==0 (
	>&2 ECHO No JavaScript files found.
	EXIT /B 0
)
IF DEFINED list CALL :runEslint
EXIT /B %status%

:addTarget
SET "target=%~1"
IF EXIST "%target%\" (
	IF "!target:~-1!"=="\" SET "target=!target:~0,-1!"
	IF "!target:~-1!"=="/" SET "target=!target:~0,-1!"
	FOR /F "delims=" %%F IN ('DIR /S /B /A-D "!target!\*.js" 2^>NUL') DO (
		IF /I "%%~xF"==".js" CALL :addFile "%%F"
	)
) ELSE (
	CALL :addFile "%target%"
)
EXIT /B 0

REM The cmd.exe command line is limited to 8191 characters, so long file lists
REM are linted in several ESLint runs.
:addFile
SET /A count+=1
SET list=!list! "%~1"
IF NOT "!list:~6000,1!"=="" CALL :runEslint
EXIT /B 0

REM Keeps the highest ESLint exit code: 1 for lint errors, 2 for a fatal error.
REM npx is not quoted: a batch file found on PATH under a quoted name gets the
REM wrong %~dp0, and npx.cmd uses it to find npm.
:runEslint
IF EXIST "%local_eslint%" (
	CALL "%local_eslint%" --no-config-lookup -c "%config%" !list!
) ELSE (
	CALL npx --yes %eslint_package% --no-config-lookup -c "%config%" !list!
)
IF %ERRORLEVEL% GTR %status% SET status=%ERRORLEVEL%
SET "list="
EXIT /B 0
