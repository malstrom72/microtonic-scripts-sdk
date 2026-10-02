@ECHO OFF
REM Render every static .ivg under the product resources folder and examples\ to
REM PNG with tools\IVG2PNG, so IVG syntax and renderer errors are caught early.
REM
REM Usage: tools\validate-static-ivg.cmd [output-dir]
REM Environment: IVG2PNG (renderer path), IVG_FONTS (font directory).
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

CD /D "%~dp0.."

IF "%IVG2PNG%"=="" SET "IVG2PNG=tools\IVG2PNG\IVG2PNG.exe"
IF "%IVG_FONTS%"=="" SET "IVG_FONTS=IVG\fonts"
IF "%~1"=="" (
	SET "output_dir=%TEMP%\%product_id%-static-ivg-validation"
) ELSE (
	SET "output_dir=%~1"
)

IF NOT EXIST "%IVG2PNG%" (
	CALL tools\build-ivg2png.cmd release native || EXIT /B 1
)

MKDIR "%output_dir%" >NUL 2>&1

SET status=0
SET "root=%CD%"
REM Each file is rendered from its own directory, so make the paths absolute.
SET "renderer=%IVG2PNG%"
IF NOT "%renderer:~1,1%"==":" IF NOT "%renderer:~0,1%"=="\" SET "renderer=%root%\%renderer%"
SET "font_dir=%IVG_FONTS%"
IF NOT "%font_dir:~1,1%"==":" IF NOT "%font_dir:~0,1%"=="\" SET "font_dir=%root%\%font_dir%"
IF NOT "%output_dir:~1,1%"==":" IF NOT "%output_dir:~0,1%"=="\" SET "output_dir=%root%\%output_dir%"

FOR /R "%resources_dir%" %%F IN (*.ivg) DO (
	IF /I "%%~xF"==".ivg" CALL :renderFile "%%F"
)
FOR /R "examples" %%F IN (*.ivg) DO (
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
