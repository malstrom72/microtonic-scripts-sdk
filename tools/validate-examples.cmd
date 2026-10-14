@ECHO OFF
REM Validate the bundled example packages end to end: Cushy schema, JavaScript, and
REM static IVG. This is the single "are the examples valid?" entry point; it
REM orchestrates the per-language validators and adds the CushyLint pass that none
REM of them cover.
REM
REM Scope:
REM   - Cushy: every package under examples\ with the product's package extension,
REM            plus the top-level console package
REM   - JS:    delegated to validate-js.cmd with its default scope (the console
REM            package plus everything under examples\, including loose example
REM            scripts that are not inside a package)
REM   - IVG:   delegated to validate-static-ivg.cmd (example .ivg plus the shared
REM            product resources folder the examples depend on)
REM
REM Exits non-zero if any section fails, after running them all so you see every
REM problem in one run.
REM
REM The Windows counterpart of tools/validate-examples.sh, with the same output and
REM exit codes.
REM
REM Shared between the Microtonic and Synplant script SDKs. Only the product
REM configuration block below differs between the two copies.
SETLOCAL ENABLEEXTENSIONS

REM ---- Product configuration -------------------------------------------------
SET "product_id=microtonic"
SET "sdk_name=microtonic-scripts-sdk"
SET "resources_dir=Microtonic Resources"
SET "package_ext=mtscript"
SET "console_package=JSConsole.mtscript"
REM ---- End of product configuration ------------------------------------------

CD /D "%~dp0.."
SET "sdk_root=%CD%"

:makeTempRoot
SET "tmp_root=%TEMP%\%product_id%-examples.%RANDOM%%RANDOM%"
IF EXIST "%tmp_root%" GOTO makeTempRoot
MKDIR "%tmp_root%" >NUL 2>&1
IF NOT EXIST "%tmp_root%\" (
	>&2 ECHO Could not create a temporary directory.
	EXIT /B 1
)
SET mirror_ready=0
SET "failures="
SET "log_file=%tmp_root%\cushylint.log"

ECHO == Cushy schema (CushyLint) ==
REM Example packages: everything under examples\ with the product's package
REM extension, plus the top-level console package.
FOR /D %%P IN ("examples\*.%package_ext%") DO CALL :checkPackage "%%P"
IF EXIST "%console_package%\" CALL :checkPackage "%console_package%"

ECHO(
ECHO == JavaScript (ESLint) ==
CALL tools\validate-js.cmd
IF ERRORLEVEL 1 GOTO javaScriptFailed
ECHO ok   JavaScript
GOTO javaScriptDone
:javaScriptFailed
SET "failures=%failures% javascript"
:javaScriptDone

ECHO(
ECHO == Static IVG ==
CALL tools\validate-static-ivg.cmd
IF ERRORLEVEL 1 GOTO staticIvgFailed
ECHO ok   static IVG
GOTO staticIvgDone
:staticIvgFailed
SET "failures=%failures% static-ivg"
:staticIvgDone

ECHO(
RMDIR /S /Q "%tmp_root%" >NUL 2>&1
IF NOT DEFINED failures GOTO passed
>&2 ECHO FAILED:%failures%
EXIT /B 1
:passed
ECHO All example checks passed.
EXIT /B 0

REM Runs CushyLint on package %1, given relative to the SDK root.
:checkPackage
SET "pkg=%~1"
SET "shown=%pkg:\=/%"
REM Skip packages without a .cushy (nothing for CushyLint to check).
DIR /S /B /A-D "%pkg%\*.cushy" >NUL 2>&1
IF NOT ERRORLEVEL 1 GOTO hasCushy
ECHO skipped (no .cushy) %shown%
EXIT /B 0
:hasCushy
CALL :setPackageForCushyLint
REM Run CushyLint from its own folder; it changes to that folder itself, and
REM PUSHD/POPD restore ours. Pass a C:/... path: CushyLint accepts / as a
REM directory slash on Windows, and the trailing slash tells it the argument is a
REM package directory.
PUSHD "%sdk_root%\CushyLint"
CALL .\CushyLint.bat "%cushy_pkg:\=/%/" >"%log_file%" 2>&1
SET "lint_error=%ERRORLEVEL%"
POPD
IF NOT "%lint_error%"=="0" GOTO packageFailed
ECHO ok   %shown%
EXIT /B 0
:packageFailed
>&2 ECHO FAIL %shown%
TYPE "%log_file%" 1>&2
SET "failures=%failures% cushy: %shown%"
EXIT /B 0

REM Sets cushy_pkg to the directory CushyLint should check for package pkg.
:setPackageForCushyLint
SET "cushy_pkg=%sdk_root%\%pkg%"
FINDSTR /S /M /L /C:"references/%sdk_name%/" "%pkg%\*.schema" >NUL 2>&1
IF ERRORLEVEL 1 EXIT /B 0

REM Some bundled examples double as copy-ready external-project examples, so
REM their .schema files point at ../../references/<sdk>/. Validate the same
REM package contents in a temporary project-shaped mirror instead of
REM rewriting the checked-in schema. The mirror holds copies (not links) of
REM the resources folder and the CushyLint schemas it includes, so cleanup
REM can never reach back into the SDK checkout.
SET "ref_root=%tmp_root%\project\references\%sdk_name%"
IF "%mirror_ready%"=="1" GOTO mirrorReady
MKDIR "%tmp_root%\project\examples" "%ref_root%\CushyLint" >NUL 2>&1
XCOPY "%sdk_root%\%resources_dir%" "%ref_root%\%resources_dir%\" /E /I /H /Q /Y >NUL
COPY /Y "%sdk_root%\CushyLint\*.schema" "%ref_root%\CushyLint\" >NUL
SET mirror_ready=1
:mirrorReady
SET "cushy_pkg=%tmp_root%\project\%pkg%"
RMDIR /S /Q "%cushy_pkg%" >NUL 2>&1
XCOPY "%sdk_root%\%pkg%" "%cushy_pkg%\" /E /I /H /Q /Y >NUL
EXIT /B 0
