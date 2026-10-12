@ECHO OFF
REM Render static .ivg files to PNG with tools\IVG2PNG, so IVG syntax and renderer
REM errors are caught early.
REM
REM Usage: tools\validate-static-ivg.cmd [--out <dir>] [<path> ...]
REM
REM Each <path> is an .ivg file or a folder that is searched recursively, relative
REM to the current directory. Without paths, the SDK's own examples\ and product
REM resources folder are checked. PNGs and logs go to --out, by default a temp
REM folder named after the product.
REM
REM Environment: IVG2PNG (renderer path), IVG_FONTS (font directory), both
REM relative to the SDK root unless absolute.
REM
REM Files the command-line renderer cannot render on its own are reported and
REM skipped, not failed: dynamic files ("Variable <name> does not exist"), files
REM that use IVG include statements ("Could not include file") and helper files
REM without top-level bounds ("Undeclared bounds"). tools/validate-static-ivg.sh
REM applies the same rules.
REM
REM Shared between the Microtonic and Synplant script SDKs. Only the product
REM configuration block below differs between the two copies.
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ---- Product configuration -------------------------------------------------
SET "product_id=microtonic"
SET "resources_dir=Microtonic Resources"
REM ---- End of product configuration ------------------------------------------

REM SHIFT also shifts %0, so take the SDK root before parsing the arguments.
SET "sdk_root=%~dp0.."
REM Paths are relative to the caller's directory, so resolve them before the CD below.
SET "output_dir="
SET path_count=0
:parseArgs
IF "%~1"=="" GOTO argsDone
SET "arg=%~1"
IF "%~1"=="--out" (
	IF "%~2"=="" GOTO usage
	SET "output_dir=%~f2"
	SHIFT
	SHIFT
	GOTO parseArgs
)
IF "!arg:~0,1!"=="-" (
	ECHO Unknown option: %1 1>&2
	GOTO usage
)
IF NOT EXIST "%~1" (
	ECHO No such file or folder: %1 1>&2
	EXIT /B 2
)
SET /A path_count+=1
SET "path_!path_count!=%~f1"
SHIFT
GOTO parseArgs

:usage
ECHO Usage: tools\validate-static-ivg.cmd [--out ^<dir^>] [^<path^> ...] 1>&2
EXIT /B 2

:argsDone
CD /D "%sdk_root%"
SET "root=%CD%"
IF %path_count%==0 (
	SET "path_1=%root%\%resources_dir%"
	SET "path_2=%root%\examples"
	SET path_count=2
)

IF "%IVG2PNG%"=="" SET "IVG2PNG=tools\IVG2PNG\IVG2PNG.exe"
IF "%IVG_FONTS%"=="" SET "IVG_FONTS=IVG\fonts"
IF "%output_dir%"=="" SET "output_dir=%TEMP%\%product_id%-static-ivg-validation"

IF NOT EXIST "%IVG2PNG%" (
	CALL tools\build-ivg2png.cmd release native || EXIT /B 1
)

MKDIR "%output_dir%" >NUL 2>&1

SET status=0
SET count=0
REM Each file is rendered from its own directory, so make the tool paths absolute.
SET "renderer=%IVG2PNG%"
IF NOT "%renderer:~1,1%"==":" IF NOT "%renderer:~0,1%"=="\" SET "renderer=%root%\%renderer%"
SET "font_dir=%IVG_FONTS%"
IF NOT "%font_dir:~1,1%"==":" IF NOT "%font_dir:~0,1%"=="\" SET "font_dir=%root%\%font_dir%"

FOR /L %%I IN (1,1,%path_count%) DO CALL :scanPath "!path_%%I!"
IF %count%==0 ECHO No .ivg files found 1>&2
EXIT /B %status%

REM FOR /R cannot take a FOR variable as its root, hence this subroutine.
:scanPath
SET "scan=%~1"
IF "%scan:~-1%"=="\" SET "scan=%scan:~0,-1%"
REM Name each output after the file's path below the given path's parent, which keeps names short.
FOR %%P IN ("%scan%") DO SET "base=%%~dpP"
IF EXIST "%scan%\" (
	FOR /R "%scan%" %%F IN (*.ivg) DO (
		IF /I "%%~xF"==".ivg" CALL :renderFile "%%F"
	)
) ELSE (
	CALL :renderFile "%scan%"
)
EXIT /B 0

:renderFile
SET /A count+=1
SET "ivg_file=%~1"
SET "rel=%ivg_file%"
CALL SET "rel=%%rel:%base%=%%"
SET "safe=%rel:\=__%"
SET "output_file=%output_dir%\%safe%.png"
SET "log_file=%output_file%.log"
PUSHD "%~dp1" >NUL
"%renderer%" --fast --fonts "%font_dir%" "%~nx1" "%output_file%" >"%log_file%" 2>&1
SET "render_error=%ERRORLEVEL%"
POPD >NUL
IF NOT "%render_error%"=="0" (
	FINDSTR /R /C:"Variable .* does not exist" "%log_file%" >NUL 2>&1
	IF NOT ERRORLEVEL 1 (
		ECHO skipped dynamic %rel%
	) ELSE (
		FINDSTR /C:"Could not include file" "%log_file%" >NUL 2>&1
		IF NOT ERRORLEVEL 1 (
			ECHO skipped include-dependent %rel%
		) ELSE (
			FINDSTR /C:"Undeclared bounds" "%log_file%" >NUL 2>&1
			IF NOT ERRORLEVEL 1 (
				ECHO skipped helper %rel%
			) ELSE (
				TYPE "%log_file%" >&2
				ECHO failed %rel% >&2
				SET status=1
			)
		)
	)
) ELSE (
	ECHO rendered %rel% -^> %output_file%
)
EXIT /B 0
