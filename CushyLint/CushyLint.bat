@ECHO OFF
CD /D "%~dp0"
CALL .\support\bootstrapPikaCmd.cmd || EXIT /B 1
.\build\PikaCmd.exe .\CushyLint.pika %*
