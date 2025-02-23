@echo off
echo Starting P2Pal Deployment...

REM Create build directory if it doesn't exist
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake ..
if errorlevel 1 goto error

REM Build the project
echo Building project...
cmake --build . --config Release
if errorlevel 1 goto error

REM Deploy with windeployqt
echo Deploying Qt dependencies...
windeployqt Release\P2Pal.exe
if errorlevel 1 goto error

echo Deployment completed successfully!
goto end

:error
echo Error occurred during deployment!
pause
exit /b 1

:end
echo.
echo Deployment completed. Files are ready in build/Release directory.
pause