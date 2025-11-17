@echo off
setlocal

echo Generating Visual Studio project files for UCSharpProject...
echo.
echo Note: This requires Unreal Engine 5 to be installed and accessible.
echo If you have UE5 installed in a custom location, please modify this script accordingly.
echo.
echo Common UE5 installation paths:
echo - C:\Program Files\Epic Games\UE_5.x
echo - C:\Program Files (x86)\Epic Games\UE_5.x
echo - Custom installation directory
echo.
echo To generate project files manually:
echo 1. Right-click on UCSharpProject.uproject
echo 2. Select "Generate Visual Studio project files"
echo.
echo Alternatively, use UnrealBuildTool directly:
echo.

REM Set UE engine path
set UE_ENGINE_PATH=D:\Games\UE_5.6
set BUILD_TOOL=%UE_ENGINE_PATH%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe

REM Get project path
set PROJECT_DIR=%~dp0
set PROJECT_FILE=%PROJECT_DIR%UCSharpProject.uproject

REM Check if engine path exists
if not exist "%BUILD_TOOL%" (
    echo Error: Cannot find UnrealBuildTool: %BUILD_TOOL%
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

echo Executing: "%BUILD_TOOL%" -projectfiles -project="%PROJECT_FILE%" -game -rocket -progress
echo.

"%BUILD_TOOL%" -projectfiles -project="%PROJECT_FILE%" -game -rocket -progress

REM Check result
if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================
    echo Project files generated successfully!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Failed to generate project files! Error code: %ERRORLEVEL%
    echo ========================================
    pause
    exit /b %ERRORLEVEL%
)

endlocal
echo.