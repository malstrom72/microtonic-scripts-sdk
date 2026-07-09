@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

IF "%CPP_TARGET%"=="" SET CPP_TARGET=release
IF "%CPP_MODEL%"=="" SET CPP_MODEL=native

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

IF "%~1"=="x86" (
	SET CPP_MODEL=x86
	SHIFT
) ELSE IF "%~1"=="x64" (
	SET CPP_MODEL=x64
	SHIFT
) ELSE IF "%~1"=="arm64" (
	SET CPP_MODEL=arm64
	SHIFT
) ELSE IF "%~1"=="native" (
	SET CPP_MODEL=native
	SHIFT
)

IF "%CPP_TARGET%"=="debug" (
	SET CPP_OPTIONS=/Od /MTd /GS /Z7 /D DEBUG %CPP_OPTIONS%
) ELSE IF "%CPP_TARGET%"=="beta" (
	SET CPP_OPTIONS=/O2 /GL /MTd /GS /Z7 /D DEBUG %CPP_OPTIONS%
) ELSE IF "%CPP_TARGET%"=="release" (
	SET CPP_OPTIONS=/O2 /GL /MT /GS- /D NDEBUG %CPP_OPTIONS%
) ELSE (
	ECHO Unrecognized CPP_TARGET %CPP_TARGET%
	EXIT /B 1
)

SET CPP_EFFECTIVE_MODEL=%CPP_MODEL%
IF "%CPP_MODEL%"=="native" (
	IF /I "%PROCESSOR_ARCHITEW6432%"=="ARM64" (
		SET CPP_EFFECTIVE_MODEL=arm64
	) ELSE IF /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
		SET CPP_EFFECTIVE_MODEL=arm64
	) ELSE IF /I "%PROCESSOR_ARCHITEW6432%"=="AMD64" (
		SET CPP_EFFECTIVE_MODEL=x64
	) ELSE IF /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
		SET CPP_EFFECTIVE_MODEL=x64
	) ELSE (
		SET CPP_EFFECTIVE_MODEL=x86
	)
)

IF "%CPP_EFFECTIVE_MODEL%"=="arm64" (
	SET vcvarsConfig=arm64
) ELSE IF "%CPP_EFFECTIVE_MODEL%"=="x64" (
	SET vcvarsConfig=amd64
) ELSE IF "%CPP_EFFECTIVE_MODEL%"=="x86" (
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
	ECHO BuildCpp [debug^|beta^|release] [x86^|x64^|arm64^|native] ^<output.exe^> ^<source files and other compiler arguments^>
	ECHO You can also use the environment variables: CPP_MSVC_VERSION, CPP_TARGET, CPP_MODEL and CPP_OPTIONS
	EXIT /B 1
)

:argLoop
	IF "%~1"=="" goto argLoopEnd
	SET "args=%args% %1"
	SHIFT
goto argLoop
:argLoopEnd

SET pfpath=%ProgramFiles(x86)%
IF NOT DEFINED pfpath SET pfpath=%ProgramFiles%

IF NOT DEFINED VCINSTALLDIR (
	IF EXIST "%pfpath%\Microsoft Visual Studio\Installer\vswhere.exe" (
		IF "%CPP_MSVC_VERSION%"=="" (
			SET "range= "
		) else (
			SET "range=-version [%CPP_MSVC_VERSION%,%CPP_MSVC_VERSION%]"
		)
		for /f "usebackq tokens=*" %%a in (`"%pfpath%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -legacy !range! -products * -property installationPath`) do set vsInstallPath=%%a
		IF EXIST "!vsInstallPath!\VC\Auxiliary\Build\vcvarsall.bat" (
			ECHO setting up: %vcvarsConfig%
			CALL "!vsInstallPath!\VC\Auxiliary\Build\vcvarsall.bat" %vcvarsConfig% >NUL
			GOTO foundTools
		)
		IF EXIST "!vsInstallPath!\VC\vcvarsall.bat" (
			ECHO setting up: %vcvarsConfig%
			CALL "!vsInstallPath!\VC\vcvarsall.bat" %vcvarsConfig% >NUL
			GOTO foundTools
		)
		ECHO Could not find Visual C++ in one of the standard paths.
	) else (
		IF "%CPP_MSVC_VERSION%"=="" (
			FOR /L %%v IN (14,-1,9) DO (
				IF EXIST "%pfpath%\Microsoft Visual Studio %%v.0\VC\vcvarsall.bat" (
					SET CPP_MSVC_VERSION=%%v
					ECHO setting up: %vcvarsConfig%
					CALL "%pfpath%\Microsoft Visual Studio %%v.0\VC\vcvarsall.bat" %vcvarsConfig% >NUL
					GOTO foundTools
				)
			)
			ECHO Could not find Visual C++ in one of the standard paths.
		) ELSE (
			IF EXIST "%pfpath%\Microsoft Visual Studio %CPP_MSVC_VERSION%.0\VC\vcvarsall.bat" (
				ECHO setting up: %vcvarsConfig%
				CALL "%pfpath%\Microsoft Visual Studio %CPP_MSVC_VERSION%.0\VC\vcvarsall.bat" %vcvarsConfig% >NUL
				GOTO foundTools
			)
			ECHO Could not find Visual C++ version %CPP_MSVC_VERSION% in the standard path.
		)
	)
	ECHO Manually run vcvarsall.bat first or run this batch from a Visual Studio command line.
	EXIT /B 1
)
:foundTools

SET temppath=%TEMP:"=%\%name%_%RANDOM%
MKDIR "%temppath%" >NUL 2>&1
ECHO Compiling %name% %CPP_TARGET% %CPP_MODEL% using %VCINSTALLDIR%
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
