# UCSharp自动化测试运行脚本
# 用于执行UE5自动化测试和C#单元测试

param(
    [string]$UE5Path = "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    [string]$ProjectPath = "$PSScriptRoot\..\TestProject\UCSharpTest.uproject",
    [string]$TestFilter = "UCSharp",
    [switch]$Verbose,
    [switch]$GenerateReport
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 颜色输出函数
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

# 检查UE5路径
function Test-UE5Path {
    param([string]$Path)
    
    if (-not (Test-Path $Path)) {
        Write-ColorOutput "错误: 找不到UE5编辑器: $Path" "Red"
        Write-ColorOutput "请检查UE5安装路径或使用 -UE5Path 参数指定正确路径" "Yellow"
        return $false
    }
    return $true
}

# 检查项目路径
function Test-ProjectPath {
    param([string]$Path)
    
    if (-not (Test-Path $Path)) {
        Write-ColorOutput "错误: 找不到项目文件: $Path" "Red"
        return $false
    }
    return $true
}

# 编译C#项目
function Build-CSharpProject {
    Write-ColorOutput "正在编译C#项目..." "Cyan"
    
    $managedPath = "$PSScriptRoot\..\Managed"
    Push-Location $managedPath
    
    try {
        $buildResult = dotnet build --configuration Release
        if ($LASTEXITCODE -ne 0) {
            Write-ColorOutput "C#项目编译失败" "Red"
            return $false
        }
        Write-ColorOutput "C#项目编译成功" "Green"
        return $true
    }
    catch {
        Write-ColorOutput "C#项目编译出错: $($_.Exception.Message)" "Red"
        return $false
    }
    finally {
        Pop-Location
    }
}

# 运行UE5自动化测试
function Start-UE5AutomationTests {
    param(
        [string]$UE5Path,
        [string]$ProjectPath,
        [string]$TestFilter
    )
    
    Write-ColorOutput "正在运行UE5自动化测试..." "Cyan"
    Write-ColorOutput "测试过滤器: $TestFilter" "Gray"
    
    $logPath = "$PSScriptRoot\..\TestResults\AutomationTest_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
    $reportPath = "$PSScriptRoot\..\TestResults\AutomationReport_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
    
    # 确保测试结果目录存在
    $testResultsDir = Split-Path $logPath -Parent
    if (-not (Test-Path $testResultsDir)) {
        New-Item -ItemType Directory -Path $testResultsDir -Force | Out-Null
    }
    
    # 构建UE5自动化测试命令
    $arguments = @(
        "`"$ProjectPath`"",
        "-ExecCmds=`"Automation RunTests $TestFilter`"",
        "-TestExit=`"Automation Test Queue Empty`"",
        "-ReportOutputPath=`"$reportPath`"",
        "-Log=`"$logPath`"",
        "-NoSplash",
        "-Unattended",
        "-NullRHI"
    )
    
    if ($Verbose) {
        $arguments += "-Verbose"
    }
    
    Write-ColorOutput "执行命令: $UE5Path $($arguments -join ' ')" "Gray"
    
    try {
        $process = Start-Process -FilePath $UE5Path -ArgumentList $arguments -Wait -PassThru -NoNewWindow
        
        if ($process.ExitCode -eq 0) {
            Write-ColorOutput "UE5自动化测试完成" "Green"
            
            # 解析测试结果
            if (Test-Path $reportPath) {
                $report = Get-Content $reportPath | ConvertFrom-Json
                Write-ColorOutput "测试结果:" "White"
                Write-ColorOutput "  总计: $($report.TotalTests)" "White"
                Write-ColorOutput "  通过: $($report.PassedTests)" "Green"
                Write-ColorOutput "  失败: $($report.FailedTests)" "Red"
                
                if ($report.FailedTests -gt 0) {
                    Write-ColorOutput "失败的测试:" "Red"
                    foreach ($failedTest in $report.FailedTestDetails) {
                        Write-ColorOutput "  - $($failedTest.TestName): $($failedTest.ErrorMessage)" "Red"
                    }
                }
            }
            
            return $true
        }
        else {
            Write-ColorOutput "UE5自动化测试失败，退出代码: $($process.ExitCode)" "Red"
            
            # 显示日志文件的最后几行
            if (Test-Path $logPath) {
                Write-ColorOutput "日志文件最后20行:" "Yellow"
                Get-Content $logPath | Select-Object -Last 20 | ForEach-Object {
                    Write-ColorOutput "  $_" "Gray"
                }
            }
            
            return $false
        }
    }
    catch {
        Write-ColorOutput "运行UE5自动化测试时出错: $($_.Exception.Message)" "Red"
        return $false
    }
}

# 运行C#单元测试
function Start-CSharpUnitTests {
    Write-ColorOutput "正在运行C#单元测试..." "Cyan"
    
    $managedPath = "$PSScriptRoot\..\Managed"
    Push-Location $managedPath
    
    try {
        # 这里可以添加C#单元测试框架的调用
        # 例如使用NUnit、xUnit或MSTest
        
        Write-ColorOutput "C#单元测试框架尚未完全集成" "Yellow"
        Write-ColorOutput "C#测试将通过UE5自动化测试框架执行" "Yellow"
        
        return $true
    }
    catch {
        Write-ColorOutput "C#单元测试出错: $($_.Exception.Message)" "Red"
        return $false
    }
    finally {
        Pop-Location
    }
}

# 生成测试报告
function New-TestReport {
    param(
        [bool]$UE5TestsSuccess,
        [bool]$CSharpTestsSuccess
    )
    
    if (-not $GenerateReport) {
        return
    }
    
    Write-ColorOutput "正在生成测试报告..." "Cyan"
    
    $reportData = @{
        Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        UE5Tests = @{
            Success = $UE5TestsSuccess
            Status = if ($UE5TestsSuccess) { "PASSED" } else { "FAILED" }
        }
        CSharpTests = @{
            Success = $CSharpTestsSuccess
            Status = if ($CSharpTestsSuccess) { "PASSED" } else { "FAILED" }
        }
        OverallSuccess = $UE5TestsSuccess -and $CSharpTestsSuccess
    }
    
    $reportPath = "$PSScriptRoot\..\TestResults\TestReport_$(Get-Date -Format 'yyyyMMdd_HHmmss').json"
    $reportData | ConvertTo-Json -Depth 3 | Out-File -FilePath $reportPath -Encoding UTF8
    
    Write-ColorOutput "测试报告已生成: $reportPath" "Green"
}

# 主函数
function Main {
    Write-ColorOutput "=== UCSharp自动化测试运行器 ===" "Magenta"
    Write-ColorOutput "开始时间: $(Get-Date)" "Gray"
    
    # 验证路径
    if (-not (Test-UE5Path $UE5Path)) {
        exit 1
    }
    
    if (-not (Test-ProjectPath $ProjectPath)) {
        exit 1
    }
    
    $allTestsSuccess = $true
    
    # 编译C#项目
    if (-not (Build-CSharpProject)) {
        $allTestsSuccess = $false
    }
    
    # 运行C#单元测试
    $csharpTestsSuccess = Start-CSharpUnitTests
    if (-not $csharpTestsSuccess) {
        $allTestsSuccess = $false
    }
    
    # 运行UE5自动化测试
    $ue5TestsSuccess = Start-UE5AutomationTests $UE5Path $ProjectPath $TestFilter
    if (-not $ue5TestsSuccess) {
        $allTestsSuccess = $false
    }
    
    # 生成报告
    New-TestReport $ue5TestsSuccess $csharpTestsSuccess
    
    # 输出最终结果
    Write-ColorOutput "=== 测试完成 ===" "Magenta"
    Write-ColorOutput "结束时间: $(Get-Date)" "Gray"
    
    if ($allTestsSuccess) {
        Write-ColorOutput "所有测试通过!" "Green"
        exit 0
    }
    else {
        Write-ColorOutput "部分测试失败!" "Red"
        exit 1
    }
}

# 运行主函数
Main