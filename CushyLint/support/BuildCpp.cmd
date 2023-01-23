@SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
@IF NOT EXIST "%SC_ROOT%" ( ECHO ERROR: SC_ROOT environment variable not correctly set. & EXIT /B 1 )
@CALL %SC_ROOT%Tools\bin\SelectMSVC.cmd || EXIT /B 1


IF "%CPP_TARGET%"=="" SET CPP_TARGET=release
IF "%CPP_MODEL%"=="" SET CPP_MODEL=64

IF "%~1"=="debug" (
	SET CPP_TARGET=debug
	SHIFT
) ELSE IF "%~1"=="beta" (
	SET CPP_TARGET=beta
	SHIFT
) ELSE IF "%~1"=="release" (
	SET CPP_TARGET=release
	SHIFT
)

IF "%~1"=="32" (
	SET CPP_MODEL=32
	SHIFT
) ELSE IF "%~1"=="64" (
	SET CPP_MODEL=64
	SHIFT
)

IF "%CPP_TARGET%"=="debug" (
	SET CPP_OPTIONS=/Od /MTd /GS /Zi /D DEBUG %CPP_OPTIONS%
) ELSE IF "%CPP_TARGET%"=="beta" (
	SET CPP_OPTIONS=/O2 /GL /MTd /GS /Zi /D DEBUG %CPP_OPTIONS%
) ELSE IF "%CPP_TARGET%"=="release" (
	SET CPP_OPTIONS=/O2 /GL /MT /GS- /D NDEBUG %CPP_OPTIONS%
) ELSE (
	ECHO Unrecognized CPP_TARGET %CPP_TARGET%
	EXIT /B 1
)

IF "%CPP_MODEL%"=="64" (
	SET vcvarsConfig=x86_amd64
) ELSE IF "%CPP_MODEL%"=="32" (
	SET CPP_OPTIONS=/arch:SSE2 %CPP_OPTIONS%
	SET vcvarsConfig=x86
) ELSE (
	ECHO Unrecognized CPP_MODEL %CPP_MODEL%
	EXIT /B 1
)

SET args=%1
SET name=%~n1
SHIFT

SET CPP_OPTIONS=/W3 /EHsc /D "WIN32" /D "_CONSOLE" /D "_CRT_SECURE_NO_WARNINGS" /D "_SCL_SECURE_NO_WARNINGS" %CPP_OPTIONS%

IF "%name%"=="" (
	ECHO BuildCpp [debug^|beta^|release] [32^|64 ^(bit^)] ^<output.exe^> ^<source files and other compiler arguments^>
	ECHO You can also use the environment variables: CPP_MSVC_VERSION, CPP_TARGET, CPP_MODEL and CPP_OPTIONS
	EXIT /B 1
)

:argLoop
	IF "%~1"=="" goto argLoopEnd
	SET "args=%args% %1"
	SHIFT
goto argLoop
:argLoopEnd


SET temppath=%TEMP:"=%\%name%_%RANDOM%
MKDIR "%temppath%" >NUL 2>&1
ECHO Compiling %name% %CPP_TARGET% %CPP_MODEL% using MSVC V%CPP_MSVC_VERSION%
ECHO %CPP_OPTIONS% /Fe%args%
ECHO.
cl %CPP_OPTIONS% /errorReport:queue /Fo"%temppath%\\" /Fe%args% >"%temppath%\buildlog.txt"
IF ERRORLEVEL 1 (
	TYPE "%temppath%\buildlog.txt"
	ECHO Compilation of %name% failed
	DEL /Q "%temppath%\*" >NUL 2>&1
	RMDIR /Q "%temppath%" >NUL 2>&1
	EXIT /B 1
) ELSE (
	ECHO Compiled %name%
	DEL /Q "%temppath%\*" >NUL 2>&1
	RMDIR /Q "%temppath%\" >NUL 2>&1

	EXIT /B 0
)
