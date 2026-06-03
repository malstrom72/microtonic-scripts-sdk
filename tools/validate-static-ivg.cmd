@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

CD /D "%~dp0.."

IF "%IVG2PNG%"=="" SET "IVG2PNG=tools\IVG2PNG\IVG2PNG.exe"
IF "%IVG_FONTS%"=="" SET "IVG_FONTS=IVG\fonts"
IF "%~1"=="" (
	SET "output_dir=%TEMP%\microtonic-static-ivg-validation"
) ELSE (
	SET "output_dir=%~1"
)

IF NOT EXIST "%IVG2PNG%" (
	CALL tools\build-ivg2png.cmd release native || EXIT /B 1
)

MKDIR "%output_dir%" >NUL 2>&1

SET status=0
SET "root=%CD%"
FOR /R "Microtonic Resources" %%F IN (*.ivg) DO (
	IF /I "%%~xF"==".ivg" CALL :renderFile "%%F"
)
FOR /R "examples" %%F IN (*.ivg) DO (
	IF /I "%%~xF"==".ivg" CALL :renderFile "%%F"
)
FOR /R "IVG\tests" %%F IN (*.ivg) DO (
	IF /I "%%~xF"==".ivg" CALL :renderFile "%%F"
)

EXIT /B %status%

:renderFile
SET "ivg_file=%~1"
SET "rel=%ivg_file%"
CALL SET "rel=%%rel:%root%\=%%"
SET "safe=%rel:\=__%"
SET "output_file=%output_dir%\%safe%.png"
SET "log_file=%output_file%.log"
"%IVG2PNG%" --fast --fonts "%IVG_FONTS%" "%ivg_file%" "%output_file%" >"%log_file%" 2>&1
IF ERRORLEVEL 1 (
	FINDSTR /C:"does not exist" "%log_file%" >NUL 2>&1
	IF NOT ERRORLEVEL 1 (
		ECHO skipped dynamic %rel%
	) ELSE (
		TYPE "%log_file%" >&2
		ECHO failed %rel% >&2
		SET status=1
	)
) ELSE (
	ECHO rendered %rel% -^> %output_file%
)
EXIT /B 0
