@echo off
setlocal

REM UCSharp Project Build Script
REM Usage: build.bat [Config] [Platform] [Target]
REM Example: build.bat Development Win64 UCSharpProjectEditor

REM Set default parameters
set CONFIG=%1
set PLATFORM=%2
set TARGET=%3

REM Use default values if no parameters provided
if "%CONFIG%"=="" set CONFIG=Development
if "%PLATFORM%"=="" set PLATFORM=Win64
if "%TARGET%"=="" set TARGET=UCSharpProjectEditor

REM Set UE engine path
set UE_ENGINE_PATH=D:\Games\UE_5.6
set BUILD_TOOL=%UE_ENGINE_PATH%\Engine\Build\BatchFiles\Build.bat

REM Get project path
set PROJECT_DIR=%~dp0
set PROJECT_FILE=%PROJECT_DIR%UCSharpProject.uproject

echo Building UCSharpGenerator UHT plugin assembly...
pushd "%PROJECT_DIR%Plugins\UCSharp\Source\UCSharpGenerator"
dotnet build -c Development /p:EngineDir="%UE_ENGINE_PATH%\Engine"
if %ERRORLEVEL% neq 0 (
    popd
    echo Error: UCSharpGenerator build failed
    exit /b %ERRORLEVEL%
)
popd

REM Check if engine path exists
if not exist "%BUILD_TOOL%" (
    echo Error: Cannot find UE build tool: %BUILD_TOOL%
    echo Please check if UE engine installation path is correct
    pause
    exit /b 1
)

REM Check if project file exists
if not exist "%PROJECT_FILE%" (
    echo Error: Cannot find project file: %PROJECT_FILE%
    pause
    exit /b 1
)

echo ========================================
echo UCSharp Project Build Script
echo ========================================
echo Config: %CONFIG%
echo Platform: %PLATFORM%
echo Target: %TARGET%
echo Project: %PROJECT_FILE%
echo ========================================
echo.

REM Execute build
echo Starting build...
"%BUILD_TOOL%" %TARGET% %PLATFORM% %CONFIG% -Project="%PROJECT_FILE%" -WaitMutex -FromMsBuild

REM Check build result
if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================
    echo Build completed successfully!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Build failed! Error code: %ERRORLEVEL%
    echo ========================================
    pause
    exit /b %ERRORLEVEL%
)

endlocal
