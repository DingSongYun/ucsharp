# UCSharp Advanced Automation Test Runner
# 基于UE5 Automation框架的完整测试解决方案
# 支持本地开发和CI/CD环境

param(
    [string]$UE5Path = "D:\Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
    [string]$ProjectPath = "",
    [string]$TestFilter = "UCSharp",
    [string]$OutputDir = "",
    [string]$ConfigFile = "",
    [switch]$Verbose,
    [switch]$CIMode,
    [switch]$GenerateReport,
    [switch]$SkipBuild,
    [int]$Timeout = 600
)

# 设置错误处理
$ErrorActionPreference = "Stop"

# 颜色输出函数
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White",
        [string]$Prefix = ""
    )
    
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $fullMessage = "[$timestamp]$Prefix $Message"
    
    if ($CIMode) {
        Write-Host $fullMessage
    } else {
        Write-Host $fullMessage -ForegroundColor $Color
    }
}

function Write-Info { param([string]$Message) Write-ColorOutput $Message "Cyan" " [INFO]" }
function Write-Success { param([string]$Message) Write-ColorOutput $Message "Green" " [SUCCESS]" }
function Write-Warning { param([string]$Message) Write-ColorOutput $Message "Yellow" " [WARNING]" }
function Write-Error { param([string]$Message) Write-ColorOutput $Message "Red" " [ERROR]" }
function Write-Debug { param([string]$Message) if ($Verbose) { Write-ColorOutput $Message "Gray" " [DEBUG]" } }

# 初始化脚本
function Initialize-TestEnvironment {
    Write-Info "初始化UCSharp自动化测试环境"
    
    # 获取脚本目录和项目根目录
    $script:ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $script:PluginRoot = Split-Path -Parent $ScriptDir
    $script:ProjectRoot = Split-Path -Parent (Split-Path -Parent $PluginRoot)
    
    # 设置默认路径
    if (-not $ProjectPath) {
        $script:ProjectPath = Join-Path $ProjectRoot "UCSharpProject.uproject"
    } else {
        $script:ProjectPath = $ProjectPath
    }
    
    if (-not $OutputDir) {
        $script:OutputDir = Join-Path $ProjectRoot "TestResults"
    } else {
        $script:OutputDir = $OutputDir
    }
    
    if (-not $ConfigFile) {
        $script:ConfigFile = Join-Path $ProjectRoot "Config\DefaultAutomationTests.ini"
    } else {
        $script:ConfigFile = $ConfigFile
    }
    
    # 创建输出目录
    if (-not (Test-Path $OutputDir)) {
        New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
        Write-Info "创建测试结果目录: $OutputDir"
    }
    
    # 验证必要文件
    if (-not (Test-Path $ProjectPath)) {
        throw "项目文件不存在: $ProjectPath"
    }
    
    if (-not (Test-Path $UE5Path)) {
        throw "UE5编辑器不存在: $UE5Path"
    }
    
    Write-Info "项目路径: $ProjectPath"
    Write-Info "UE5路径: $UE5Path"
    Write-Info "输出目录: $OutputDir"
    Write-Info "测试过滤器: $TestFilter"
}

# 构建项目
function Build-Project {
    if ($SkipBuild) {
        Write-Info "跳过项目构建"
        return $true
    }
    
    Write-Info "开始构建项目"
    
    # 构建C#项目
    $managedDir = Join-Path $PluginRoot "Managed"
    if (Test-Path $managedDir) {
        Write-Info "构建C#项目"
        Push-Location $managedDir
        try {
            $buildOutput = dotnet build --configuration Release --verbosity minimal 2>&1
            if ($LASTEXITCODE -ne 0) {
                Write-Error "C#项目构建失败"
                Write-Debug $buildOutput
                return $false
            }
            Write-Success "C#项目构建成功"
        } finally {
            Pop-Location
        }
    }
    
    # 构建UE5项目
    Write-Info "构建UE5项目"
    $buildToolPath = $UE5Path -replace "UnrealEditor-Cmd.exe", "..\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
    
    $buildArgs = @(
        "UCSharpProjectEditor",
        "Win64",
        "Development",
        "-Project=`"$ProjectPath`"",
        "-WaitMutex"
    )
    
    try {
        $buildProcess = Start-Process -FilePath $buildToolPath -ArgumentList $buildArgs -Wait -PassThru -NoNewWindow
        if ($buildProcess.ExitCode -ne 0) {
            Write-Error "UE5项目构建失败，退出代码: $($buildProcess.ExitCode)"
            return $false
        }
        Write-Success "UE5项目构建成功"
        return $true
    } catch {
        Write-Error "构建过程中发生异常: $($_.Exception.Message)"
        return $false
    }
}

# 运行自动化测试
function Run-AutomationTests {
    Write-Info "开始运行UE5自动化测试"
    
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $logPath = Join-Path $OutputDir "AutomationTest_$timestamp.log"
    $reportPath = Join-Path $OutputDir "AutomationReport_$timestamp.json"
    $csvPath = Join-Path $OutputDir "AutomationResults_$timestamp.csv"
    
    # 构建测试命令参数
    $testCommands = @(
        "Automation RunTests $TestFilter",
        "Automation SetFilter $TestFilter"
    )
    
    if ($GenerateReport) {
        $testCommands += "Automation ExportResults `"$csvPath`""
    }
    
    $testCommands += "Automation Test Queue Empty"
    
    $arguments = @(
        "`"$ProjectPath`"",
        "-ExecCmds=`"$($testCommands -join '; ')`"",
        "-TestExit=`"Automation Test Queue Empty`"",
        "-ReportOutputPath=`"$reportPath`"",
        "-Log=`"$logPath`"",
        "-NoSplash",
        "-Unattended",
        "-NullRHI",
        "-NoSound"
    )
    
    if ($CIMode) {
        $arguments += "-buildmachine"
    }
    
    Write-Debug "测试命令: $UE5Path $($arguments -join ' ')"
    Write-Info "日志文件: $logPath"
    Write-Info "报告文件: $reportPath"
    
    try {
        # 启动测试进程
        $testProcess = Start-Process -FilePath $UE5Path -ArgumentList $arguments -PassThru -NoNewWindow
        
        # 等待测试完成或超时
        $completed = $testProcess.WaitForExit($Timeout * 1000)
        
        if (-not $completed) {
            Write-Warning "测试超时 ($Timeout 秒)，强制终止进程"
            $testProcess.Kill()
            $testProcess.WaitForExit()
            return $false
        }
        
        $exitCode = $testProcess.ExitCode
        Write-Info "测试进程退出代码: $exitCode"
        
        # 分析测试结果
        return Analyze-TestResults $logPath $reportPath $csvPath
        
    } catch {
        Write-Error "运行测试时发生异常: $($_.Exception.Message)"
        return $false
    }
}

# 分析测试结果
function Analyze-TestResults {
    param(
        [string]$LogPath,
        [string]$ReportPath,
        [string]$CsvPath
    )
    
    Write-Info "分析测试结果"
    
    $testResults = @{
        TotalTests = 0
        PassedTests = 0
        FailedTests = 0
        SkippedTests = 0
        Warnings = 0
        Errors = 0
        Duration = 0
        FailedTestDetails = @()
    }
    
    # 分析日志文件
    if (Test-Path $LogPath) {
        Write-Info "分析日志文件: $LogPath"
        $logContent = Get-Content $LogPath
        
        foreach ($line in $logContent) {
            if ($line -match "LogAutomationController.*Test Completed.*Result='([^']+)'.*Name='([^']+)'") {
                $result = $matches[1]
                $testName = $matches[2]
                
                $testResults.TotalTests++
                
                switch ($result) {
                    "Passed" { $testResults.PassedTests++ }
                    "Failed" { 
                        $testResults.FailedTests++
                        $testResults.FailedTestDetails += @{
                            TestName = $testName
                            Result = $result
                            Line = $line
                        }
                    }
                    "Skipped" { $testResults.SkippedTests++ }
                }
            }
            
            if ($line -match "LogAutomationController.*Warning") {
                $testResults.Warnings++
            }
            
            if ($line -match "LogAutomationController.*Error") {
                $testResults.Errors++
            }
        }
    }
    
    # 分析JSON报告文件
    if (Test-Path $ReportPath) {
        Write-Info "分析JSON报告文件: $ReportPath"
        try {
            $reportContent = Get-Content $ReportPath | ConvertFrom-Json
            # 根据实际的JSON结构更新测试结果
            if ($reportContent.TotalTests) {
                $testResults.TotalTests = $reportContent.TotalTests
            }
            if ($reportContent.PassedTests) {
                $testResults.PassedTests = $reportContent.PassedTests
            }
            if ($reportContent.FailedTests) {
                $testResults.FailedTests = $reportContent.FailedTests
            }
        } catch {
            Write-Warning "无法解析JSON报告文件: $($_.Exception.Message)"
        }
    }
    
    # 输出测试结果摘要
    Write-Info "=== 测试结果摘要 ==="
    Write-Info "总测试数: $($testResults.TotalTests)"
    Write-Success "通过测试: $($testResults.PassedTests)"
    
    if ($testResults.FailedTests -gt 0) {
        Write-Error "失败测试: $($testResults.FailedTests)"
    } else {
        Write-Success "失败测试: $($testResults.FailedTests)"
    }
    
    if ($testResults.SkippedTests -gt 0) {
        Write-Warning "跳过测试: $($testResults.SkippedTests)"
    }
    
    if ($testResults.Warnings -gt 0) {
        Write-Warning "警告数量: $($testResults.Warnings)"
    }
    
    if ($testResults.Errors -gt 0) {
        Write-Error "错误数量: $($testResults.Errors)"
    }
    
    # 输出失败测试详情
    if ($testResults.FailedTestDetails.Count -gt 0) {
        Write-Warning "=== 失败测试详情 ==="
        foreach ($failedTest in $testResults.FailedTestDetails) {
            Write-Error "- $($failedTest.TestName)"
        }
    }
    
    # 生成测试报告
    if ($GenerateReport) {
        Generate-TestReport $testResults
    }
    
    # 返回测试是否成功
    return ($testResults.FailedTests -eq 0 -and $testResults.Errors -eq 0)
}

# 生成测试报告
function Generate-TestReport {
    param($TestResults)
    
    Write-Info "生成测试报告"
    
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $reportPath = Join-Path $OutputDir "UCSharp_TestReport_$(Get-Date -Format 'yyyyMMdd_HHmmss').html"
    
    $htmlContent = @"
<!DOCTYPE html>
<html>
<head>
    <title>UCSharp 自动化测试报告</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .summary { margin: 20px 0; }
        .passed { color: green; }
        .failed { color: red; }
        .warning { color: orange; }
        .details { margin-top: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <div class="header">
        <h1>UCSharp 自动化测试报告</h1>
        <p>生成时间: $timestamp</p>
        <p>测试过滤器: $TestFilter</p>
    </div>
    
    <div class="summary">
        <h2>测试摘要</h2>
        <table>
            <tr><th>指标</th><th>数量</th></tr>
            <tr><td>总测试数</td><td>$($TestResults.TotalTests)</td></tr>
            <tr><td class="passed">通过测试</td><td class="passed">$($TestResults.PassedTests)</td></tr>
            <tr><td class="failed">失败测试</td><td class="failed">$($TestResults.FailedTests)</td></tr>
            <tr><td class="warning">跳过测试</td><td class="warning">$($TestResults.SkippedTests)</td></tr>
            <tr><td class="warning">警告数量</td><td class="warning">$($TestResults.Warnings)</td></tr>
            <tr><td class="failed">错误数量</td><td class="failed">$($TestResults.Errors)</td></tr>
        </table>
    </div>
"@
    
    if ($TestResults.FailedTestDetails.Count -gt 0) {
        $htmlContent += @"
    <div class="details">
        <h2>失败测试详情</h2>
        <table>
            <tr><th>测试名称</th><th>结果</th></tr>
"@
        foreach ($failedTest in $TestResults.FailedTestDetails) {
            $htmlContent += "            <tr><td>$($failedTest.TestName)</td><td class=\"failed\">$($failedTest.Result)</td></tr>`n"
        }
        $htmlContent += "        </table>`n    </div>`n"
    }
    
    $htmlContent += @"
</body>
</html>
"@
    
    $htmlContent | Out-File -FilePath $reportPath -Encoding UTF8
    Write-Success "测试报告已生成: $reportPath"
}

# 主函数
function Main {
    try {
        Write-Info "=== UCSharp 自动化测试开始 ==="
        
        # 初始化环境
        Initialize-TestEnvironment
        
        # 构建项目
        if (-not (Build-Project)) {
            throw "项目构建失败"
        }
        
        # 运行测试
        $testSuccess = Run-AutomationTests
        
        if ($testSuccess) {
            Write-Success "=== 所有测试通过 ==="
            exit 0
        } else {
            Write-Error "=== 测试失败 ==="
            exit 1
        }
        
    } catch {
        Write-Error "测试运行失败: $($_.Exception.Message)"
        Write-Debug $_.ScriptStackTrace
        exit 1
    }
}

# 显示帮助信息
function Show-Help {
    Write-Host @"
UCSharp 自动化测试运行器

用法:
    .\RunAutomationTests.ps1 [参数]

参数:
    -UE5Path <路径>        UE5编辑器可执行文件路径
    -ProjectPath <路径>    项目文件(.uproject)路径
    -TestFilter <过滤器>   测试过滤器 (默认: UCSharp)
    -OutputDir <目录>      测试结果输出目录
    -ConfigFile <文件>     自动化测试配置文件
    -Verbose              显示详细输出
    -CIMode               CI/CD模式 (禁用颜色输出)
    -GenerateReport       生成HTML测试报告
    -SkipBuild            跳过项目构建
    -Timeout <秒>         测试超时时间 (默认: 600秒)
    -Help                 显示此帮助信息

示例:
    .\RunAutomationTests.ps1 -Verbose -GenerateReport
    .\RunAutomationTests.ps1 -TestFilter "UCSharp.Core" -Timeout 300
    .\RunAutomationTests.ps1 -CIMode -SkipBuild
"@
}

# 检查是否需要显示帮助
if ($args -contains "-Help" -or $args -contains "--help" -or $args -contains "/?") {
    Show-Help
    exit 0
}

# 运行主函数
Main