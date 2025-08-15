# UCSharp Test System Validation Script
# Validates the new automation test framework configuration

param(
    [switch]$Verbose,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

# Color output functions
function Write-Info { param([string]$Message) Write-Host "[INFO] $Message" -ForegroundColor Cyan }
function Write-Success { param([string]$Message) Write-Host "[SUCCESS] $Message" -ForegroundColor Green }
function Write-Warning { param([string]$Message) Write-Host "[WARNING] $Message" -ForegroundColor Yellow }
function Write-Error { param([string]$Message) Write-Host "[ERROR] $Message" -ForegroundColor Red }
function Write-Debug { param([string]$Message) if ($Verbose) { Write-Host "[DEBUG] $Message" -ForegroundColor Gray } }

# Validation functions
function Test-FileExists {
    param([string]$FilePath, [string]$Description)
    
    if (Test-Path $FilePath) {
        Write-Success "✓ $Description exists: $FilePath"
        return $true
    } else {
        Write-Error "✗ $Description missing: $FilePath"
        return $false
    }
}

function Test-DirectoryExists {
    param([string]$DirPath, [string]$Description)
    
    if (Test-Path $DirPath -PathType Container) {
        Write-Success "✓ $Description exists: $DirPath"
        return $true
    } else {
        Write-Warning "⚠ $Description missing: $DirPath"
        return $false
    }
}

function Test-ConfigurationFile {
    param([string]$ConfigPath)
    
    Write-Info "Validating configuration file: $ConfigPath"
    
    if (-not (Test-Path $ConfigPath)) {
        Write-Error "Configuration file does not exist"
        return $false
    }
    
    try {
        $configContent = Get-Content $ConfigPath
        $requiredSections = @(
            '[/Script/AutomationController.AutomationControllerSettings]',
            '[UCSharp.TestGroups]',
            '[UCSharp.Runtime]',
            '[UCSharp.Reporting]'
        )
        
        $foundSections = 0
        foreach ($section in $requiredSections) {
            if ($configContent -contains $section) {
                Write-Success "  ✓ Found section: $section"
                $foundSections++
            } else {
                Write-Warning "  ⚠ Missing section: $section"
            }
        }
        
        if ($foundSections -eq $requiredSections.Count) {
            Write-Success "Configuration file validation passed"
            return $true
        } else {
            Write-Warning "Configuration file partially validated ($foundSections/$($requiredSections.Count))"
            return $false
        }
        
    } catch {
        Write-Error "Failed to read configuration file: $($_.Exception.Message)"
        return $false
    }
}

function Test-PowerShellScript {
    param([string]$ScriptPath)
    
    Write-Info "Validating PowerShell script: $ScriptPath"
    
    if (-not (Test-Path $ScriptPath)) {
        Write-Error "Script file does not exist"
        return $false
    }
    
    try {
        # Syntax check
        $null = [System.Management.Automation.PSParser]::Tokenize((Get-Content $ScriptPath -Raw), [ref]$null)
        Write-Success "PowerShell script syntax validation passed"
        
        # Check key functions
        $scriptContent = Get-Content $ScriptPath -Raw
        $requiredFunctions = @(
            'Initialize-TestEnvironment',
            'Build-Project',
            'Run-AutomationTests',
            'Analyze-TestResults',
            'Generate-TestReport'
        )
        
        $foundFunctions = 0
        foreach ($func in $requiredFunctions) {
            if ($scriptContent -match "function $func") {
                Write-Success "  ✓ Found function: $func"
                $foundFunctions++
            } else {
                Write-Warning "  ⚠ Missing function: $func"
            }
        }
        
        if ($foundFunctions -eq $requiredFunctions.Count) {
            Write-Success "PowerShell script structure validation passed"
            return $true
        } else {
            Write-Warning "PowerShell script partially validated ($foundFunctions/$($requiredFunctions.Count))"
            return $false
        }
        
    } catch {
        Write-Error "PowerShell script validation failed: $($_.Exception.Message)"
        return $false
    }
}

function Test-UE5Installation {
    param([string]$UE5Path)
    
    Write-Info "Validating UE5 installation: $UE5Path"
    
    if (Test-Path $UE5Path) {
        Write-Success "UE5 editor executable exists"
        return $true
    } else {
        Write-Error "UE5 editor executable does not exist"
        return $false
    }
}

function Test-DotNetEnvironment {
    Write-Info "Validating .NET environment"
    
    try {
        $dotnetVersion = dotnet --version 2>$null
        if ($dotnetVersion) {
            Write-Success ".NET version: $dotnetVersion"
            
            # Check if .NET 8.0 is available
            $sdks = dotnet --list-sdks 2>$null
            $net8Sdk = $sdks | Where-Object { $_ -match "8\." }
            if ($net8Sdk) {
                Write-Success ".NET 8.0 SDK available"
                return $true
            } else {
                Write-Warning ".NET 8.0 SDK not available"
                return $false
            }
        } else {
            Write-Error ".NET not installed or not in PATH"
            return $false
        }
    } catch {
        Write-Error ".NET environment check failed: $($_.Exception.Message)"
        return $false
    }
}

function Test-ProjectStructure {
    param([string]$ProjectRoot)
    
    Write-Info "Validating project structure"
    
    $requiredPaths = @(
        @{ Path = "$ProjectRoot\UCSharpProject.uproject"; Type = "File"; Description = "Project file" },
        @{ Path = "$ProjectRoot\Plugins\UCSharp"; Type = "Directory"; Description = "UCSharp plugin directory" },
        @{ Path = "$ProjectRoot\Plugins\UCSharp\Source"; Type = "Directory"; Description = "Plugin source directory" },
        @{ Path = "$ProjectRoot\Plugins\UCSharp\Managed"; Type = "Directory"; Description = "C# code directory" },
        @{ Path = "$ProjectRoot\Config"; Type = "Directory"; Description = "Configuration directory" }
    )
    
    $validPaths = 0
    foreach ($pathInfo in $requiredPaths) {
        if ($pathInfo.Type -eq "File") {
            if (Test-FileExists $pathInfo.Path $pathInfo.Description) {
                $validPaths++
            }
        } else {
            if (Test-DirectoryExists $pathInfo.Path $pathInfo.Description) {
                $validPaths++
            }
        }
    }
    
    if ($validPaths -eq $requiredPaths.Count) {
        Write-Success "Project structure validation passed"
        return $true
    } else {
        Write-Warning "Project structure partially validated ($validPaths/$($requiredPaths.Count))"
        return $false
    }
}

# Main validation function
function Main {
    Write-Info "=== UCSharp Test System Validation Started ==="
    
    # Get paths
    $scriptDir = Split-Path -Parent $PSCommandPath
    $pluginRoot = Split-Path -Parent $scriptDir
    $projectRoot = Split-Path -Parent (Split-Path -Parent $pluginRoot)
    
    Write-Debug "Script directory: $scriptDir"
    Write-Debug "Plugin root: $pluginRoot"
    Write-Debug "Project root: $projectRoot"
    
    Write-Info "Project root: $projectRoot"
    Write-Info "Plugin root: $pluginRoot"
    
    $testResults = @()
    
    # 1. Validate project structure
    $testResults += Test-ProjectStructure $projectRoot
    
    # 2. Validate UE5 installation
    $ue5Path = "D:\Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    $testResults += Test-UE5Installation $ue5Path
    
    # 3. Validate .NET environment
    $testResults += Test-DotNetEnvironment
    
    # 4. Validate automation test script
    $automationScript = Join-Path $scriptDir "RunAutomationTests.ps1"
    $testResults += Test-PowerShellScript $automationScript
    
    # 5. Validate configuration file
    $configFile = Join-Path $projectRoot "Config\DefaultAutomationTests.ini"
    $testResults += Test-ConfigurationFile $configFile
    
    # 6. Validate GitHub Actions workflow
    $workflowFile = Join-Path $projectRoot ".github\workflows\automated-tests.yml"
    $testResults += Test-FileExists $workflowFile "GitHub Actions workflow file"
    
    # Calculate results
    $passedTests = ($testResults | Where-Object { $_ -eq $true }).Count
    $totalTests = $testResults.Count
    
    Write-Info "=== Validation Results Summary ==="
    Write-Info "Total validations: $totalTests"
    Write-Success "Passed validations: $passedTests"
    
    if ($passedTests -lt $totalTests) {
        Write-Warning "Failed validations: $($totalTests - $passedTests)"
    }
    
    $successRate = [math]::Round(($passedTests / $totalTests) * 100, 2)
    Write-Info "Success rate: $successRate%"
    
    if ($successRate -ge 90) {
        Write-Success "=== Test System Validation Passed ==="
        return $true
    } elseif ($successRate -ge 70) {
        Write-Warning "=== Test System Partially Validated ==="
        Write-Warning "Recommend fixing failed validations before running full tests"
        return $false
    } else {
        Write-Error "=== Test System Validation Failed ==="
        Write-Error "Please fix critical issues and re-validate"
        return $false
    }
}

# Show help information
function Show-Help {
    Write-Host 'UCSharp Test System Validation Script'
    Write-Host ''
    Write-Host 'Usage: .\ValidateTestSystem.ps1 [parameters]'
    Write-Host ''
    Write-Host 'Parameters:'
    Write-Host '  -Verbose     Show detailed output'
    Write-Host '  -SkipBuild   Skip quick test validation'
    Write-Host '  -Help        Show this help information'
    Write-Host ''
    Write-Host 'Examples:'
    Write-Host '  .\ValidateTestSystem.ps1 -Verbose'
    Write-Host '  .\ValidateTestSystem.ps1 -SkipBuild'
}

# Check for help parameter
if ($args -contains "-Help" -or $args -contains "--help" -or $args -contains "/?") {
    Show-Help
    exit 0
}

# Run main function
try {
    $result = Main
    if ($result) {
        exit 0
    } else {
        exit 1
    }
} catch {
    Write-Error "Exception during validation: $($_.Exception.Message)"
    Write-Debug $_.ScriptStackTrace
    exit 1
}