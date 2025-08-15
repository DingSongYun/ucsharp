@echo off
setlocal enabledelayedexpansion

REM UCSharp自动化测试运行脚本 (批处理版本)
REM 用于快速执行测试

echo ===================================
echo UCSharp 自动化测试运行器
echo ===================================
echo.

REM 设置默认路径
set "UE5_PATH=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "PROJECT_PATH=%~dp0..\TestProject\UCSharpTest.uproject"
set "TEST_FILTER=UCSharp"

REM 检查UE5路径
if not exist "%UE5_PATH%" (
    echo [错误] 找不到UE5编辑器: %UE5_PATH%
    echo 请修改脚本中的UE5_PATH变量或安装UE5 5.6
    pause
    exit /b 1
)

REM 检查项目路径
if not exist "%PROJECT_PATH%" (
    echo [错误] 找不到项目文件: %PROJECT_PATH%
    pause
    exit /b 1
)

echo [信息] UE5路径: %UE5_PATH%
echo [信息] 项目路径: %PROJECT_PATH%
echo [信息] 测试过滤器: %TEST_FILTER%
echo.

REM 编译C#项目
echo [步骤 1/3] 编译C#项目...
cd /d "%~dp0..\Managed"
dotnet build --configuration Release
if errorlevel 1 (
    echo [错误] C#项目编译失败
    pause
    exit /b 1
)
echo [成功] C#项目编译完成
echo.

REM 创建测试结果目录
set "TEST_RESULTS_DIR=%~dp0..\TestResults"
if not exist "%TEST_RESULTS_DIR%" (
    mkdir "%TEST_RESULTS_DIR%"
)

REM 设置日志文件路径
for /f "tokens=1-4 delims=/ " %%a in ('date /t') do set "DATE_STAMP=%%c%%a%%b"
for /f "tokens=1-2 delims=: " %%a in ('time /t') do set "TIME_STAMP=%%a%%b"
set "LOG_PATH=%TEST_RESULTS_DIR%\AutomationTest_%DATE_STAMP%_%TIME_STAMP%.log"
set "REPORT_PATH=%TEST_RESULTS_DIR%\AutomationReport_%DATE_STAMP%_%TIME_STAMP%.json"

REM Run UE5 automation tests
echo [Step 2/3] Running UE5 automation tests...
echo [INFO] Log file: %LOG_PATH%
echo [INFO] Report file: %REPORT_PATH%
echo.

"%UE5_PATH%" "%PROJECT_PATH%" -ExecCmds="Automation RunTests %TEST_FILTER%" -TestExit="Automation Test Queue Empty" -ReportOutputPath="%REPORT_PATH%" -Log="%LOG_PATH%" -NoSplash -Unattended -NullRHI

if errorlevel 1 (
    echo [ERROR] UE5 automation tests failed
    echo [INFO] Check log file for details: %LOG_PATH%
    
    REM Show last few lines of log file
    if exist "%LOG_PATH%" (
        echo.
        echo [LOG] Last 10 lines:
        powershell -Command "Get-Content '%LOG_PATH%' | Select-Object -Last 10"
    )
    
    pause
    exit /b 1
)

echo [SUCCESS] UE5 automation tests completed
echo.

REM 显示测试结果
echo [步骤 3/3] 分析测试结果...
if exist "%REPORT_PATH%" (
    echo [信息] 测试报告已生成: %REPORT_PATH%
    
    REM 使用PowerShell解析JSON报告
    powershell -Command "$report = Get-Content '%REPORT_PATH%' | ConvertFrom-Json; Write-Host '[结果] 总计测试:' $report.TotalTests; Write-Host '[结果] 通过测试:' $report.PassedTests -ForegroundColor Green; Write-Host '[结果] 失败测试:' $report.FailedTests -ForegroundColor Red"
else (
    echo [警告] 未找到测试报告文件
)

echo.
echo ===================================
echo 测试完成!
echo ===================================
echo [信息] 日志文件: %LOG_PATH%
echo [信息] 报告文件: %REPORT_PATH%
echo.

pause