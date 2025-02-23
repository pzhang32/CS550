@echo off
echo Checking for P2Pal executable...

if not exist "%~dp0\build\Release\P2Pal.exe" (
    echo Error: P2Pal.exe not found!
    echo Please run deploy.bat first
    pause
    exit /b 1
)

echo Starting P2Pal Test Instances...

REM Launch first instance
start "" "%~dp0\build\Release\P2Pal.exe"
echo Launched Instance 1
timeout /t 2

REM Launch second instance
start "" "%~dp0\build\Release\P2Pal.exe"
echo Launched Instance 2
timeout /t 2

REM Launch third instance
start "" "%~dp0\build\Release\P2Pal.exe"
echo Launched Instance 3

echo All test instances launched.
echo Please proceed with testing according to TESTING.md