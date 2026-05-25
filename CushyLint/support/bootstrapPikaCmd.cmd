@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
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
	EXIT /B %ERRORLEVEL%
)

IF NOT EXIST "%sourceFile%" (
	ECHO Missing PikaCmd source: "%sourceFile%"
	EXIT /B 1
)

MKDIR "%rootDir%\build" >NUL 2>&1

SET needsRebuild=0
IF NOT EXIST "%output%" SET needsRebuild=1
IF "%needsRebuild%"=="0" "%output%" -h >NUL 2>NUL || SET needsRebuild=1
IF "%needsRebuild%"=="0" IF NOT EXIST "%stamp%" (
	CALL :checkVersionOrWriteStamp || SET needsRebuild=1
)
IF "%needsRebuild%"=="0" CALL :checkStamp || SET needsRebuild=1

IF "%needsRebuild%"=="1" (
	SET "tmp=%output%.tmp.exe"
	DEL /Q "%tmp%" >NUL 2>&1
	CALL "%buildScript%" release native "%tmp%" /D "PLATFORM_STRING=WINDOWS" "%sourceFile%" || EXIT /B 1
	PUSHD "%pikaDir%" || EXIT /B 1
	"%tmp%" unittests.pika >NUL || ( POPD & EXIT /B 1 )
	"%tmp%" systoolsTests.pika || ( POPD & EXIT /B 1 )
	POPD
	MOVE /Y "%tmp%" "%output%" >NUL || EXIT /B 1
	CALL :writeStamp || EXIT /B 1
)

EXIT /B 0

:hashSource
FOR /F "usebackq skip=1 tokens=1" %%H IN (`CERTUTIL -hashfile "%sourceFile%" SHA256`) DO (
	SET "line=%%H"
	IF NOT "!line:~0,8!"=="CertUtil" (
		ECHO !line!
		EXIT /B 0
	)
)
EXIT /B 1

:checkStamp
SET /P oldStamp=<"%stamp%"
FOR /F "usebackq tokens=*" %%H IN (`CALL "%~f0" --hash-only`) DO SET "newStamp=%%H"
IF "%oldStamp%"=="%newStamp%" EXIT /B 0
EXIT /B 1

:checkVersionOrWriteStamp
CALL :expectedVersion || EXIT /B 1
CALL :actualVersion || EXIT /B 1
IF NOT "%expectedVersion%"=="%actualVersion%" EXIT /B 1
CALL :writeStamp
EXIT /B %ERRORLEVEL%

:expectedVersion
FOR /F "tokens=3" %%V IN ('FINDSTR /C:"PIKA_SCRIPT_VERSION" "%sourceFile%"') DO (
	SET "expectedVersion=%%~V"
	EXIT /B 0
)
EXIT /B 1

:actualVersion
FOR /F "usebackq tokens=*" %%V IN (`"%output%" "{ print(VERSION) }" 2^>NUL`) DO (
	SET "actualVersion=%%V"
	EXIT /B 0
)
EXIT /B 1

:writeStamp
FOR /F "usebackq tokens=*" %%H IN (`CALL "%~f0" --hash-only`) DO SET "newStamp=%%H"
>"%stamp%" ECHO %newStamp%
EXIT /B 0
