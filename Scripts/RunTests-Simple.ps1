# UCSharp Simple Test Runner
# A simplified PowerShell script for running UCSharp tests

Write-Host "===================================" -ForegroundColor Cyan
Write-Host "UCSharp Simple Test Runner" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan
Write-Host ""

# Set paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$ManagedDir = Join-Path $ProjectRoot "Managed"
$TestResultsDir = Join-Path $ProjectRoot "TestResults"
$ProjectFile = Join-Path $ProjectRoot "TestProject\UCSharpTest.uproject"

# Common UE5 installation paths
$PossibleUE5Paths = @(
    "D:\Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
)

# Find UE5 installation
$UE5Path = $null
foreach ($path in $PossibleUE5Paths) {
    if (Test-Path $path) {
        $UE5Path = $path
        break
    }
}

Write-Host "[INFO] Project Root: $ProjectRoot" -ForegroundColor Green
Write-Host "[INFO] Managed Directory: $ManagedDir" -ForegroundColor Green
Write-Host "[INFO] Project File: $ProjectFile" -ForegroundColor Green
Write-Host "[INFO] Test Results Directory: $TestResultsDir" -ForegroundColor Green

if ($UE5Path) {
    Write-Host "[INFO] UE5 Found: $UE5Path" -ForegroundColor Green
} else {
    Write-Host "[WARNING] UE5 not found in common locations" -ForegroundColor Yellow
    Write-Host "[INFO] Will skip UE5 automation tests" -ForegroundColor Yellow
}
Write-Host ""

# Check if project file exists
if (-not (Test-Path $ProjectFile)) {
    Write-Host "[ERROR] Project file not found: $ProjectFile" -ForegroundColor Red
    exit 1
}

# Create test results directory
if (-not (Test-Path $TestResultsDir)) {
    New-Item -ItemType Directory -Path $TestResultsDir -Force | Out-Null
    Write-Host "[INFO] Created test results directory" -ForegroundColor Green
}

# Step 1: Build C# project
Write-Host "[Step 1/3] Building C# project..." -ForegroundColor Cyan
if (Test-Path $ManagedDir) {
    Push-Location $ManagedDir
    try {
        $buildResult = dotnet build --configuration Release 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[SUCCESS] C# project build completed" -ForegroundColor Green
        } else {
            Write-Host "[ERROR] C# project build failed" -ForegroundColor Red
            Write-Host $buildResult -ForegroundColor Red
            Pop-Location
            exit 1
        }
    } finally {
        Pop-Location
    }
} else {
    Write-Host "[WARNING] Managed directory not found, skipping C# build" -ForegroundColor Yellow
}
Write-Host ""

# Step 2: Run UE5 automation tests (if UE5 is available)
if ($UE5Path) {
    Write-Host "[Step 2/3] Running UE5 automation tests..." -ForegroundColor Cyan
    
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $logPath = Join-Path $TestResultsDir "AutomationTest_$timestamp.log"
    $reportPath = Join-Path $TestResultsDir "AutomationReport_$timestamp.json"
    
    Write-Host "[INFO] Log file: $logPath" -ForegroundColor Green
    Write-Host "[INFO] Report file: $reportPath" -ForegroundColor Green
    
    $arguments = @(
        "`"$ProjectFile`"",
        "-ExecCmds=`"Automation RunTests UCSharp`"",
        "-TestExit=`"Automation Test Queue Empty`"",
        "-ReportOutputPath=`"$reportPath`"",
        "-Log=`"$logPath`"",
        "-NoSplash",
        "-Unattended",
        "-NullRHI"
    )
    
    Write-Host "[INFO] Running command: $UE5Path $($arguments -join ' ')" -ForegroundColor Gray
    
    try {
        $process = Start-Process -FilePath $UE5Path -ArgumentList $arguments -Wait -PassThru -NoNewWindow
        
        if ($process.ExitCode -eq 0) {
            Write-Host "[SUCCESS] UE5 automation tests completed" -ForegroundColor Green
        } else {
            Write-Host "[ERROR] UE5 automation tests failed with exit code: $($process.ExitCode)" -ForegroundColor Red
            
            # Show log file if it exists
            if (Test-Path $logPath) {
                Write-Host "[LOG] Last 20 lines of log file:" -ForegroundColor Yellow
                Get-Content $logPath | Select-Object -Last 20 | ForEach-Object {
                    Write-Host "  $_" -ForegroundColor Gray
                }
            }
        }
    } catch {
        Write-Host "[ERROR] Failed to run UE5 automation tests: $($_.Exception.Message)" -ForegroundColor Red
    }
} else {
    Write-Host "[Step 2/3] Skipping UE5 automation tests (UE5 not found)" -ForegroundColor Yellow
}
Write-Host ""

# Step 3: Analyze results
Write-Host "[Step 3/3] Analyzing test results..." -ForegroundColor Cyan

$reportFiles = Get-ChildItem -Path $TestResultsDir -Filter "AutomationReport_*.json" | Sort-Object LastWriteTime -Descending

if ($reportFiles.Count -gt 0) {
    $latestReport = $reportFiles[0]
    Write-Host "[INFO] Latest test report: $($latestReport.Name)" -ForegroundColor Green
    
    try {
        $reportContent = Get-Content $latestReport.FullName | ConvertFrom-Json
        
        Write-Host "[RESULT] Total Tests: $($reportContent.TotalTests)" -ForegroundColor White
        Write-Host "[RESULT] Passed Tests: $($reportContent.PassedTests)" -ForegroundColor Green
        Write-Host "[RESULT] Failed Tests: $($reportContent.FailedTests)" -ForegroundColor $(if ($reportContent.FailedTests -gt 0) { 'Red' } else { 'Green' })
        
        if ($reportContent.FailedTests -gt 0 -and $reportContent.FailedTestDetails) {
            Write-Host "[DETAILS] Failed test details:" -ForegroundColor Yellow
            foreach ($failedTest in $reportContent.FailedTestDetails) {
                Write-Host "  - $($failedTest.TestName): $($failedTest.ErrorMessage)" -ForegroundColor Red
            }
        }
    } catch {
        Write-Host "[WARNING] Could not parse test report: $($_.Exception.Message)" -ForegroundColor Yellow
    }
} else {
    Write-Host "[WARNING] No test reports found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "===================================" -ForegroundColor Cyan
Write-Host "Test Run Completed!" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan

# List all files in test results directory
if (Test-Path $TestResultsDir) {
    $resultFiles = Get-ChildItem -Path $TestResultsDir
    if ($resultFiles.Count -gt 0) {
        Write-Host "[INFO] Test result files:" -ForegroundColor Green
        foreach ($file in $resultFiles) {
            Write-Host "  - $($file.Name) ($([math]::Round($file.Length / 1KB, 2)) KB)" -ForegroundColor Gray
        }
    }
}

Write-Host ""
Write-Host "Press any key to continue..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")