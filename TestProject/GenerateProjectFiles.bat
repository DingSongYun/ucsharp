@echo off
echo Generating Visual Studio project files for UCSharpTest...
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
echo 1. Right-click on UCSharpTest.uproject
echo 2. Select "Generate Visual Studio project files"
echo.
echo Alternatively, use UnrealBuildTool directly:
echo "[UE5_PATH]\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="UCSharpTest.uproject" -game -rocket -progress
echo.
pause