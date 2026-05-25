@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
CD /D "%~dp0"

SET "supportDir=%CD%"
FOR %%I IN ("%supportDir%\..") DO SET "rootDir=%%~fI"
SET "buildScript=%supportDir%\BuildCpp.cmd"
SET "output=%rootDir%\build\MakaronCmd.exe"
SET "stamp=%output%.sha256"

IF "%~1"=="--hash-only" (
	CALL :hashSources
	EXIT /B %ERRORLEVEL%
)

MKDIR "%rootDir%\build" >NUL 2>&1

SET needsRebuild=0
IF NOT EXIST "%output%" SET needsRebuild=1
IF "%needsRebuild%"=="0" ECHO. | "%output%" - - >NUL 2>NUL || SET needsRebuild=1
IF "%needsRebuild%"=="0" IF NOT EXIST "%stamp%" (
	CALL :writeStamp || SET needsRebuild=1
)
IF "%needsRebuild%"=="0" CALL :checkStamp || SET needsRebuild=1

IF "%needsRebuild%"=="1" (
	SET "tmp=%output%.tmp.exe"
	DEL /Q "%tmp%" >NUL 2>&1
	CALL "%buildScript%" release native "%tmp%" /I "%supportDir%" "%supportDir%\MakaronCmd.cpp" "%supportDir%\Makaron.cpp" || EXIT /B 1
	ECHO. | "%tmp%" - - >NUL 2>NUL || EXIT /B 1
	MOVE /Y "%tmp%" "%output%" >NUL || EXIT /B 1
	CALL :writeStamp || EXIT /B 1
)

EXIT /B 0

:hashSources
SET "hashInput=%TEMP%\makaron-%RANDOM%-%RANDOM%.hashinput"
DEL /Q "%hashInput%" >NUL 2>&1
FOR %%F IN ("%supportDir%\MakaronCmd.cpp" "%supportDir%\Makaron.cpp" "%supportDir%\Makaron.h" "%buildScript%") DO (
	CERTUTIL -hashfile "%%~fF" SHA256 | FINDSTR /R "^[0-9A-F][0-9A-F]" >>"%hashInput%" || EXIT /B 1
)
FOR /F "usebackq skip=1 tokens=1" %%H IN (`CERTUTIL -hashfile "%hashInput%" SHA256`) DO (
	SET "line=%%H"
	IF NOT "!line:~0,8!"=="CertUtil" (
		DEL /Q "%hashInput%" >NUL 2>&1
		ECHO !line!
		EXIT /B 0
	)
)
DEL /Q "%hashInput%" >NUL 2>&1
EXIT /B 1

:checkStamp
SET /P oldStamp=<"%stamp%"
FOR /F "usebackq tokens=*" %%H IN (`CALL "%~f0" --hash-only`) DO SET "newStamp=%%H"
IF "%oldStamp%"=="%newStamp%" EXIT /B 0
EXIT /B 1

:writeStamp
FOR /F "usebackq tokens=*" %%H IN (`CALL "%~f0" --hash-only`) DO SET "newStamp=%%H"
>"%stamp%" ECHO %newStamp%
EXIT /B 0
