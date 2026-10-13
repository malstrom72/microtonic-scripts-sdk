@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
SET "scriptFile=%~f0"
CD /D "%~dp0"

SET "supportDir=%CD%"
FOR %%I IN ("%supportDir%\..") DO SET "rootDir=%%~fI"
SET "pikaDir=%supportDir%\PikaCmd"
SET "sourceFile=%pikaDir%\PikaCmdAmalgam.cpp"
SET "buildScript=%supportDir%\BuildCpp.cmd"
SET "output=%rootDir%\build\PikaCmd.exe"
SET "stamp=%output%.sha256"

IF "%~1"=="--hash-only" (
	CALL :hashSource
	EXIT /B !ERRORLEVEL!
)

IF NOT EXIST "%sourceFile%" (
	ECHO Missing PikaCmd source: "%sourceFile%"
	EXIT /B 1
)

MKDIR "%rootDir%\build" >NUL 2>&1

SET needsRebuild=0
IF NOT EXIST "%output%" SET needsRebuild=1
IF "%needsRebuild%"=="0" "%output%" -h >NUL 2>NUL || SET needsRebuild=1
IF "%needsRebuild%"=="0" IF NOT EXIST "%stamp%" SET needsRebuild=1
IF "%needsRebuild%"=="0" CALL :checkStamp || SET needsRebuild=1

SET "tmpOut=%output%.tmp.exe"
IF "%needsRebuild%"=="1" (
	DEL /Q "%tmpOut%" >NUL 2>&1
	CALL "%buildScript%" release native "%tmpOut%" /D "PLATFORM_STRING=WINDOWS" "%sourceFile%" || EXIT /B 1
	PUSHD "%pikaDir%" || EXIT /B 1
	"%tmpOut%" unittests.pika >NUL || ( POPD & EXIT /B 1 )
	"%tmpOut%" systoolsTests.pika || ( POPD & EXIT /B 1 )
	POPD
	MOVE /Y "%tmpOut%" "%output%" >NUL || EXIT /B 1
	CALL :writeStamp || EXIT /B 1
)

EXIT /B 0

REM Hashes the source, the build script and this script, like bootstrapPikaCmd.sh, so a change to how PikaCmd is
REM built also triggers a rebuild. Hashing file contents only (not paths) keeps a committed stamp valid in any checkout.
:hashSource
SET "hashInput=%TEMP%\pikacmd-%RANDOM%-%RANDOM%.hashinput"
DEL /Q "%hashInput%" >NUL 2>&1
FOR %%F IN ("%sourceFile%" "%buildScript%" "%scriptFile%") DO (
	CERTUTIL -hashfile "%%~fF" SHA256 | FINDSTR /I /R "^[0-9a-f][0-9a-f]*$" >>"%hashInput%" || EXIT /B 1
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
