<#
.SYNOPSIS
    UCSharp Automation Test Script - Based on UE5 Automation Framework

.DESCRIPTION
    This script provides a complete UCSharp automation testing solution based on Unreal Engine's Automation framework.
    Supports both local development and CI/CD environments, including test execution, result analysis, and report generation.

.PARAMETER TestFilter
    Test filter, defaults to "UCSharp"

.PARAMETER Timeout
    Test timeout in seconds, defaults to 600 seconds

.PARAMETER OutputDir
    Output directory, defaults to "TestResults"

.PARAMETER SkipBuild
    Skip project build

.PARAMETER Verbose
    Verbose output

.PARAMETER CI
    CI environment mode

.EXAMPLE
    .\RunAutomationTests.ps1
    .\RunAutomationTests.ps1 -TestFilter "UCSharp.CoreTests" -Timeout 300
    .\RunAutomationTests.ps1 -CI -SkipBuild
#>

param(
    [string]$TestFilter = "UCSharp",
    [int]$Timeout = 600,
    [string]$OutputDir = "TestResults",
    [switch]$SkipBuild,
    [switch]$Verbose,
    [switch]$CI
)

# Set error handling
$ErrorActionPreference = "Stop"

# Color output functions
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$ForegroundColor = "White",
        [string]$Prefix = ""
    )
    
    $timestamp = Get-Date -Format "HH:mm:ss"
    $fullMessage = "[$timestamp]$Prefix $Message"
    
    if ($CI) {
        Write-Host $fullMessage
    } else {
        Write-Host $fullMessage -ForegroundColor $ForegroundColor
    }
}

function Write-Info { param([string]$Message) Write-ColorOutput $Message "Cyan" "[INFO]" }
function Write-Success { param([string]$Message) Write-ColorOutput $Message "Green" "[SUCCESS]" }
function Write-Warning { param([string]$Message) Write-ColorOutput $Message "Yellow" "[WARNING]" }
function Write-Error { param([string]$Message) Write-ColorOutput $Message "Red" "[ERROR]" }

# Initialize test environment
function Initialize-TestEnvironment {
    Write-Info "Initializing test environment"
    
    # Get script directory and project root directory
    $script:ScriptDir = Split-Path -Parent $PSCommandPath
    $script:PluginRoot = Split-Path -Parent $ScriptDir
    $script:ProjectRoot = Split-Path -Parent (Split-Path -Parent $PluginRoot)
    
    Write-Info "Project root directory: $ProjectRoot"
    Write-Info "Plugin root directory: $PluginRoot"
    
    # Find UE5 installation path
    $script:UE5Path = Find-UE5Installation
    if (-not $UE5Path) {
        throw "UE5 installation path not found"
    }
    Write-Info "UE5 path: $UE5Path"
    
    # Create output directory
    $script:OutputPath = Join-Path $ProjectRoot $OutputDir
    if (-not (Test-Path $OutputPath)) {
        New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
    }
    Write-Info "Output directory: $OutputPath"
    
    # Set output file paths
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $script:LogPath = Join-Path $OutputPath "UCSharp_AutomationTest_$timestamp.log"
    $script:CsvPath = Join-Path $OutputPath "UCSharp_TestResults_$timestamp.csv"
    
    Write-Info "Log file: $LogPath"
    Write-Info "CSV file: $CsvPath"
}

# Find UE5 installation
function Find-UE5Installation {
    $possiblePaths = @(
        "D:\Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
        "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
        "C:\Program Files (x86)\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    )
    
    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            return $path
        }
    }
    
    # Try to find from registry
    try {
        $regPath = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.6"
        if (Test-Path $regPath) {
            $installLocation = Get-ItemProperty -Path $regPath -Name "InstalledDirectory" -ErrorAction SilentlyContinue
            if ($installLocation) {
                $editorPath = Join-Path $installLocation.InstalledDirectory "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
                if (Test-Path $editorPath) {
                    return $editorPath
                }
            }
        }
    } catch {
        Write-Warning "Cannot read UE5 installation path from registry"
    }
    
    return $null
}

# Build project
function Build-Project {
    Write-Info "Starting project build"
    
    try {
        # Build C# project
        Write-Info "Building C# project"
        $managedDir = Join-Path $PluginRoot "Managed"
        if (Test-Path $managedDir) {
            Push-Location $managedDir
            try {
                & dotnet restore
                if ($LASTEXITCODE -ne 0) {
                    throw "C# project restore failed"
                }
                
                & dotnet build --configuration Release
                if ($LASTEXITCODE -ne 0) {
                    throw "C# project build failed"
                }
                Write-Success "C# project build successful"
            } finally {
                Pop-Location
            }
        }
        
        # Skip UE5 project file generation for now
        Write-Info "Skipping UE5 project file generation (not required for automation tests)"
        $projectFile = Join-Path $ProjectRoot "UCSharpProject.uproject"
        
        # Skip UE5 project build for now
        Write-Info "Skipping UE5 project build (using existing binaries)"
        
    } catch {
        Write-Error "Build failed: $($_.Exception.Message)"
        throw
    }
}

# Run automation tests
function Run-AutomationTests {
    Write-Info "Starting automation tests"
    Write-Info "Test filter: $TestFilter"
    Write-Info "Timeout: $Timeout seconds"
    
    try {
        $projectFile = Join-Path $ProjectRoot "UCSharpProject.uproject"
        
        # Build test arguments
        $arguments = @(
            "`"$projectFile`"",
            "-ExecCmds=Automation RunTests $TestFilter",
            "-TestExit=Automation Test Queue Empty",
            "-log=`"$LogPath`"",
            "-unattended",
            "-nopause",
            "-nullrhi",
            "-nosplash",
            "-noloadstartuppackages"
        )
        
        if ($CI) {
            $arguments += "-buildmachine"
        }
        
        # Check if UE5 executable exists
        if (-not (Test-Path $UE5Path)) {
            Write-Error "UE5 executable not found: $UE5Path"
            return $false
        }
        
        # Check if project file exists
        if (-not (Test-Path $projectFile)) {
            Write-Error "Project file not found: $projectFile"
            return $false
        }
        
        Write-Info "Starting test process"
        Write-Info "Command: $UE5Path $($arguments -join ' ')"
        
        Write-Info "Starting UE5 automation test process..."
        $stdoutFile = Join-Path $OutputPath "stdout.log"
        $stderrFile = Join-Path $OutputPath "stderr.log"
        
        $testProcess = Start-Process -FilePath $UE5Path -ArgumentList $arguments -PassThru -NoNewWindow -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile
        
        if (-not $testProcess) {
            Write-Error "Failed to start test process"
            return $false
        }
        
        Write-Info "Process started with PID: $($testProcess.Id)"
        
        # Wait for test completion or timeout
        $completed = $testProcess.WaitForExit($Timeout * 1000)
        
        if (-not $completed) {
            Write-Warning "Test timeout ($Timeout seconds), forcefully terminating process"
            $testProcess.Kill()
            $testProcess.WaitForExit()
            return $false
        }
        
        $exitCode = $testProcess.ExitCode
        Write-Info "Test process exit code: $exitCode"
        
        # Check if stdout/stderr files exist and show their content
        if (Test-Path $stdoutFile) {
            Write-Info "Standard output:"
            Get-Content $stdoutFile | Select-Object -Last 20 | ForEach-Object { Write-Info "  $_" }
        }
        
        if (Test-Path $stderrFile) {
            $stderrContent = Get-Content $stderrFile
            if ($stderrContent) {
                Write-Warning "Standard error:"
                $stderrContent | Select-Object -Last 20 | ForEach-Object { Write-Warning "  $_" }
            }
        }
        
        # Analyze test results
        return Analyze-TestResults $LogPath $CsvPath
        
    } catch {
        Write-Error "Exception occurred while running tests: $($_.Exception.Message)"
        return $false
    }
}

# Analyze test results
function Analyze-TestResults {
    param(
        [string]$LogPath,
        [string]$CsvPath
    )
    
    Write-Info "Analyzing test results"
    
    $results = @{
        TotalTests = 0
        PassedTests = 0
        FailedTests = 0
        SkippedTests = 0
        FailedTestDetails = @()
        Duration = 0
        Success = $false
    }
    
    if (-not (Test-Path $LogPath)) {
        Write-Warning "Log file does not exist: $LogPath"
        return $results
    }
    
    try {
        $logContent = Get-Content $LogPath -Raw
        
        # Parse test results
        $testPattern = "LogAutomationController: Test (\w+): (.+?) \((\w+)\)"
        $matches = [regex]::Matches($logContent, $testPattern)
        
        foreach ($match in $matches) {
            $results.TotalTests++
            $testResult = $match.Groups[3].Value
            $testName = $match.Groups[2].Value
            
            switch ($testResult) {
                "Passed" { $results.PassedTests++ }
                "Failed" { 
                    $results.FailedTests++
                    $results.FailedTestDetails += @{
                        TestName = $testName
                        Result = $testResult
                    }
                }
                "Skipped" { $results.SkippedTests++ }
            }
        }
        
        # Parse test duration
        $durationPattern = "LogAutomationController: Test completed in ([\d\.]+) seconds"
        $durationMatch = [regex]::Match($logContent, $durationPattern)
        if ($durationMatch.Success) {
            $results.Duration = [double]$durationMatch.Groups[1].Value
        }
        
        $results.Success = ($results.FailedTests -eq 0) -and ($results.TotalTests -gt 0)
        
        # Output result summary
        Write-Info "=== Test Results Summary ==="
        Write-Info "Total tests: $($results.TotalTests)"
        Write-Success "Passed: $($results.PassedTests)"
        if ($results.FailedTests -gt 0) {
            Write-Error "Failed: $($results.FailedTests)"
        } else {
            Write-Info "Failed: $($results.FailedTests)"
        }
        Write-Info "Skipped: $($results.SkippedTests)"
        Write-Info "Duration: $($results.Duration) seconds"
        
        if ($results.FailedTestDetails.Count -gt 0) {
            Write-Warning "Failed tests:"
            foreach ($failedTest in $results.FailedTestDetails) {
                Write-Warning "  - $($failedTest.TestName): $($failedTest.Result)"
            }
        }
        
        # Generate CSV report
        Export-TestResultsToCsv $results $CsvPath
        
        return $results
        
    } catch {
        Write-Error "Exception occurred while analyzing test results: $($_.Exception.Message)"
        return $results
    }
}

# Export test results to CSV
function Export-TestResultsToCsv {
    param(
        [hashtable]$TestResults,
        [string]$CsvPath
    )
    
    try {
        $csvData = @()
        $csvData += "TestName,Result,Duration"
        
        foreach ($failedTest in $TestResults.FailedTestDetails) {
            $csvData += "$($failedTest.TestName),$($failedTest.Result),"
        }
        
        # Add summary row
        $summary = "SUMMARY,Total=$($TestResults.TotalTests);Passed=$($TestResults.PassedTests);Failed=$($TestResults.FailedTests);Skipped=$($TestResults.SkippedTests),$($TestResults.Duration)"
        $csvData += $summary
        
        $csvData | Out-File -FilePath $CsvPath -Encoding UTF8
        Write-Success "CSV report generated: $CsvPath"
        
    } catch {
        Write-Warning "Failed to generate CSV report: $($_.Exception.Message)"
    }
}

# Main function
function Main {
    try {
        Write-Info "=== UCSharp Automation Tests Started ==="
        
        # Initialize environment
        Initialize-TestEnvironment
        
        # Build project (if needed)
        if (-not $SkipBuild) {
            Build-Project
        } else {
            Write-Info "Skipping project build"
        }
        
        # Run tests
        $testResults = Run-AutomationTests
        
        if ($testResults) {
            # Output final results
            if ($testResults.Success) {
                Write-Success "=== All Tests Passed ==="
                exit 0
            } else {
                Write-Error "=== Tests Failed ==="
                exit 1
            }
        } else {
            Write-Error "=== Test Execution Failed ==="
            exit 1
        }
        
    } catch {
        Write-Error "Script execution failed: $($_.Exception.Message)"
        Write-Error $_.ScriptStackTrace
        exit 1
    }
}

# Execute main function
Main