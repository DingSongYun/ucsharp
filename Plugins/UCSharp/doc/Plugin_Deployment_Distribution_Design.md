# UE5 C#插件部署和分发机制设计

## 1. 部署架构概述

### 1.1 部署系统整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    插件部署和分发架构图                         │
├─────────────────────────────────────────────────────────────────┤
│  开发环境 (Development Environment)                            │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  源码管理       │  │  构建系统       │  │  测试环境       │ │
│  │  - Git仓库      │  │  - CMake        │  │  - 单元测试     │ │
│  │  - 版本控制     │  │  - MSBuild      │  │  - 集成测试     │ │
│  │  - 分支管理     │  │  - 自动构建     │  │  - 性能测试     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  构建和打包 (Build & Package)                                 │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  插件构建       │  │  依赖管理       │  │  资源打包       │ │
│  │  - C++编译      │  │  - NuGet包      │  │  - 二进制文件   │ │
│  │  │  - 核心库     │  │  - .NET运行时   │  │  - 配置文件     │ │
│  │  - C#编译       │  │  - 第三方库     │  │  - 文档资源     │ │
│  │  - 绑定生成     │  │  - 版本管理     │  │  - 示例代码     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  分发渠道 (Distribution Channels)                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  官方市场       │  │  第三方平台     │  │  企业分发       │ │
│  │  - UE Marketplace│  │  - GitHub       │  │  - 私有仓库     │ │
│  │  - Epic Store   │  │  - NuGet Gallery│  │  - 内部服务器   │ │
│  │  - 官方网站     │  │  - 社区论坛     │  │  - 企业商店     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  安装和更新 (Installation & Updates)                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  安装器         │  │  更新管理       │  │  配置管理       │ │
│  │  - 自动检测     │  │  - 版本检查     │  │  - 环境配置     │ │
│  │  │  - UE5版本    │  │  - 增量更新     │  │  - 路径设置     │ │
│  │  - 依赖安装     │  │  - 回滚机制     │  │  - 权限管理     │ │
│  │  - 验证检查     │  │  - 通知系统     │  │  - 兼容性检查   │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  运行时管理 (Runtime Management)                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  插件加载       │  │  许可证管理     │  │  监控和诊断     │ │
│  │  - 动态加载     │  │  - 激活验证     │  │  - 使用统计     │ │
│  │  │  - 模块发现   │  │  - 试用期管理   │  │  - 错误报告     │ │
│  │  - 依赖解析     │  │  - 企业许可     │  │  - 性能监控     │ │
│  │  - 版本兼容     │  │  - 离线验证     │  │  - 崩溃分析     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 2. 构建系统设计

### 2.1 自动化构建流水线

```yaml
# .github/workflows/build-and-deploy.yml
name: Build and Deploy UE5 CSharp Plugin

on:
  push:
    branches: [ main, develop ]
    tags: [ 'v*' ]
  pull_request:
    branches: [ main ]

env:
  UE5_VERSION: '5.3'
  DOTNET_VERSION: '6.0'
  CMAKE_VERSION: '3.25'

jobs:
  build-windows:
    runs-on: windows-latest
    strategy:
      matrix:
        configuration: [Debug, Release]
        platform: [x64]
    
    steps:
    - name: Checkout Repository
      uses: actions/checkout@v3
      with:
        submodules: recursive
        fetch-depth: 0
    
    - name: Setup .NET
      uses: actions/setup-dotnet@v3
      with:
        dotnet-version: ${{ env.DOTNET_VERSION }}
    
    - name: Setup MSBuild
      uses: microsoft/setup-msbuild@v1.1
    
    - name: Cache UE5 Installation
      uses: actions/cache@v3
      with:
        path: C:\UnrealEngine
        key: ue5-${{ env.UE5_VERSION }}-${{ runner.os }}
    
    - name: Install UE5
      if: steps.cache-ue5.outputs.cache-hit != 'true'
      run: |
        # Download and install UE5
        Invoke-WebRequest -Uri "https://cdn.unrealengine.com/releases/ue5.3.zip" -OutFile "ue5.zip"
        Expand-Archive -Path "ue5.zip" -DestinationPath "C:\UnrealEngine"
    
    - name: Build C++ Plugin
      run: |
        cd Source
        cmake -B build -G "Visual Studio 17 2022" -A ${{ matrix.platform }}
        cmake --build build --config ${{ matrix.configuration }}
    
    - name: Build C# Runtime
      run: |
        cd CSharpRuntime
        dotnet restore
        dotnet build --configuration ${{ matrix.configuration }} --no-restore
    
    - name: Run Tests
      run: |
        cd Tests
        dotnet test --configuration ${{ matrix.configuration }} --no-build --verbosity normal
    
    - name: Package Plugin
      run: |
        powershell -File Scripts/PackagePlugin.ps1 -Configuration ${{ matrix.configuration }}
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: UE5CSharpPlugin-${{ matrix.configuration }}-${{ matrix.platform }}
        path: Packages/
  
  build-linux:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        configuration: [Debug, Release]
    
    steps:
    - name: Checkout Repository
      uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Setup .NET
      uses: actions/setup-dotnet@v3
      with:
        dotnet-version: ${{ env.DOTNET_VERSION }}
    
    - name: Install Dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake
    
    - name: Build Plugin
      run: |
        cd Source
        cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.configuration }}
        cmake --build build
    
    - name: Build C# Runtime
      run: |
        cd CSharpRuntime
        dotnet restore
        dotnet build --configuration ${{ matrix.configuration }}
    
    - name: Package Plugin
      run: |
        bash Scripts/PackagePlugin.sh ${{ matrix.configuration }}
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: UE5CSharpPlugin-Linux-${{ matrix.configuration }}
        path: Packages/
  
  deploy:
    needs: [build-windows, build-linux]
    runs-on: ubuntu-latest
    if: startsWith(github.ref, 'refs/tags/v')
    
    steps:
    - name: Download Artifacts
      uses: actions/download-artifact@v3
    
    - name: Create Release
      uses: softprops/action-gh-release@v1
      with:
        files: |
          UE5CSharpPlugin-Release-x64/*
          UE5CSharpPlugin-Linux-Release/*
        generate_release_notes: true
      env:
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
    
    - name: Publish to NuGet
      run: |
        dotnet nuget push **/*.nupkg --api-key ${{ secrets.NUGET_API_KEY }} --source https://api.nuget.org/v3/index.json
```

### 2.2 打包脚本

```powershell
# Scripts/PackagePlugin.ps1
param(
    [Parameter(Mandatory=$true)]
    [string]$Configuration,
    [string]$OutputPath = "Packages",
    [string]$Version = "1.0.0"
)

$ErrorActionPreference = "Stop"

Write-Host "Packaging UE5 CSharp Plugin - Configuration: $Configuration" -ForegroundColor Green

# 创建输出目录
$PackageDir = Join-Path $OutputPath "UE5CSharpPlugin-$Version-$Configuration"
if (Test-Path $PackageDir) {
    Remove-Item $PackageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $PackageDir -Force | Out-Null

# 复制插件文件
$PluginStructure = @{
    "Binaries" = @(
        "Source/build/$Configuration/*.dll",
        "Source/build/$Configuration/*.pdb",
        "CSharpRuntime/bin/$Configuration/**/*.dll",
        "CSharpRuntime/bin/$Configuration/**/*.pdb"
    )
    "Content" = @(
        "Content/**/*"
    )
    "Source" = @(
        "Source/**/*.h",
        "Source/**/*.cpp",
        "Source/**/*.cs"
    )
    "Config" = @(
        "Config/**/*.ini"
    )
    "Documentation" = @(
        "Documentation/**/*",
        "README.md",
        "CHANGELOG.md",
        "LICENSE"
    )
    "Examples" = @(
        "Examples/**/*"
    )
}

foreach ($folder in $PluginStructure.Keys) {
    $targetDir = Join-Path $PackageDir $folder
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    
    foreach ($pattern in $PluginStructure[$folder]) {
        $files = Get-ChildItem -Path $pattern -Recurse -ErrorAction SilentlyContinue
        foreach ($file in $files) {
            $relativePath = $file.FullName.Substring((Get-Location).Path.Length + 1)
            $targetPath = Join-Path $PackageDir $relativePath
            $targetFolder = Split-Path $targetPath -Parent
            
            if (!(Test-Path $targetFolder)) {
                New-Item -ItemType Directory -Path $targetFolder -Force | Out-Null
            }
            
            Copy-Item $file.FullName $targetPath -Force
            Write-Host "Copied: $relativePath" -ForegroundColor Gray
        }
    }
}

# 生成插件描述文件
$PluginDescriptor = @{
    FileVersion = 3
    Version = 1
    VersionName = $Version
    FriendlyName = "UE5 C# Scripting Support"
    Description = "Provides C# scripting support for Unreal Engine 5"
    Category = "Scripting"
    CreatedBy = "UE5 CSharp Team"
    CreatedByURL = "https://github.com/ue5-csharp/plugin"
    DocsURL = "https://docs.ue5-csharp.com"
    MarketplaceURL = ""
    SupportURL = "https://github.com/ue5-csharp/plugin/issues"
    EngineVersion = "5.3.0"
    CanContainContent = $true
    IsBetaVersion = $false
    IsExperimentalVersion = $false
    Installed = $false
    Modules = @(
        @{
            Name = "UCSharpRuntime"
            Type = "Runtime"
            LoadingPhase = "PreDefault"
            PlatformAllowList = @("Win64", "Linux", "Mac")
        },
        @{
            Name = "UCSharpEditor"
            Type = "Editor"
            LoadingPhase = "Default"
            PlatformAllowList = @("Win64", "Linux", "Mac")
        }
    )
    Plugins = @(
        @{
            Name = "CoreUObject"
            Enabled = $true
        },
        @{
            Name = "Engine"
            Enabled = $true
        },
        @{
            Name = "UnrealEd"
            Enabled = $true
        }
    )
} | ConvertTo-Json -Depth 10

$PluginDescriptor | Out-File -FilePath (Join-Path $PackageDir "UCSharpPlugin.uplugin") -Encoding UTF8

# 创建安装脚本
$InstallScript = @'
@echo off
echo Installing UE5 CSharp Plugin...

set UE5_PATH=%1
if "%UE5_PATH%"=="" (
    echo Usage: install.bat "C:\Path\To\UE5"
    pause
    exit /b 1
)

if not exist "%UE5_PATH%\Engine" (
    echo Error: UE5 installation not found at %UE5_PATH%
    pause
    exit /b 1
)

set PLUGIN_PATH=%UE5_PATH%\Engine\Plugins\Marketplace\UCSharpPlugin

echo Creating plugin directory...
if exist "%PLUGIN_PATH%" (
    rmdir /s /q "%PLUGIN_PATH%"
)
mkdir "%PLUGIN_PATH%"

echo Copying plugin files...
xcopy /s /e /y ".\*" "%PLUGIN_PATH%\"

echo Plugin installed successfully!
echo Please restart Unreal Editor to use the plugin.
pause
'@

$InstallScript | Out-File -FilePath (Join-Path $PackageDir "install.bat") -Encoding ASCII

# 创建卸载脚本
$UninstallScript = @'
@echo off
echo Uninstalling UE5 CSharp Plugin...

set UE5_PATH=%1
if "%UE5_PATH%"=="" (
    echo Usage: uninstall.bat "C:\Path\To\UE5"
    pause
    exit /b 1
)

set PLUGIN_PATH=%UE5_PATH%\Engine\Plugins\Marketplace\UCSharpPlugin

if exist "%PLUGIN_PATH%" (
    echo Removing plugin directory...
    rmdir /s /q "%PLUGIN_PATH%"
    echo Plugin uninstalled successfully!
) else (
    echo Plugin not found at %PLUGIN_PATH%
)

pause
'@

$UninstallScript | Out-File -FilePath (Join-Path $PackageDir "uninstall.bat") -Encoding ASCII

# 创建ZIP包
$ZipPath = Join-Path $OutputPath "UE5CSharpPlugin-$Version-$Configuration.zip"
Compress-Archive -Path "$PackageDir\*" -DestinationPath $ZipPath -Force

Write-Host "Package created successfully: $ZipPath" -ForegroundColor Green
Write-Host "Package size: $((Get-Item $ZipPath).Length / 1MB) MB" -ForegroundColor Yellow
```

## 3. 分发渠道设计

### 3.1 官方市场分发

```cpp
// UCSharpMarketplaceIntegration.h
#pragma once

#include "CoreMinimal.h"
#include "Http.h"

// 市场平台类型
ENUM_CLASS_FLAGS(EMarketplacePlatform)
{
    None = 0,
    UnrealMarketplace = 1 << 0,
    EpicGamesStore = 1 << 1,
    Steam = 1 << 2,
    GitHub = 1 << 3,
    NuGetGallery = 1 << 4
};

// 插件信息
struct UCSHARPRUNTIME_API FPluginMarketplaceInfo
{
    FString PluginId;
    FString Name;
    FString Version;
    FString Description;
    FString Author;
    FString Category;
    TArray<FString> Tags;
    FString IconUrl;
    FString DownloadUrl;
    FString DocumentationUrl;
    FString SupportUrl;
    float Price;
    bool bIsFree;
    int32 Downloads;
    float Rating;
    int32 ReviewCount;
    
    FPluginMarketplaceInfo()
        : Price(0.0f)
        , bIsFree(true)
        , Downloads(0)
        , Rating(0.0f)
        , ReviewCount(0)
    {}
};

// 市场集成管理器
class UCSHARPRUNTIME_API UCSharpMarketplaceIntegration
{
public:
    static UCSharpMarketplaceIntegration& Get();
    
    // 初始化和清理
    bool Initialize();
    void Shutdown();
    
    // 插件发布
    bool PublishPlugin(EMarketplacePlatform Platform, const FPluginMarketplaceInfo& PluginInfo, const FString& PackagePath);
    bool UpdatePlugin(EMarketplacePlatform Platform, const FString& PluginId, const FPluginMarketplaceInfo& PluginInfo);
    bool UnpublishPlugin(EMarketplacePlatform Platform, const FString& PluginId);
    
    // 插件搜索和下载
    TArray<FPluginMarketplaceInfo> SearchPlugins(EMarketplacePlatform Platform, const FString& Query, const FString& Category = TEXT(""));
    bool DownloadPlugin(EMarketplacePlatform Platform, const FString& PluginId, const FString& DownloadPath);
    bool InstallPlugin(const FString& PackagePath, const FString& InstallPath);
    
    // 版本管理
    TArray<FString> GetAvailableVersions(EMarketplacePlatform Platform, const FString& PluginId);
    bool CheckForUpdates(const FString& PluginId, FString& LatestVersion);
    bool DownloadUpdate(const FString& PluginId, const FString& Version);
    
    // 统计和分析
    bool SubmitDownloadStats(const FString& PluginId, const FString& Version);
    bool SubmitUsageStats(const FString& PluginId, const TMap<FString, FString>& Stats);
    bool SubmitCrashReport(const FString& PluginId, const FString& CrashData);
    
    // 事件回调
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPluginDownloaded, const FString& /*PluginId*/, bool /*bSuccess*/);
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPluginInstalled, const FString& /*PluginId*/, bool /*bSuccess*/);
    DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPluginUpdateAvailable, const FString& /*PluginId*/, const FString& /*CurrentVersion*/, const FString& /*LatestVersion*/);
    
    FOnPluginDownloaded OnPluginDownloaded;
    FOnPluginInstalled OnPluginInstalled;
    FOnPluginUpdateAvailable OnPluginUpdateAvailable;
    
private:
    UCSharpMarketplaceIntegration() = default;
    
    // HTTP请求处理
    bool SendHttpRequest(const FString& Url, const FString& Method, const FString& Content, TFunction<void(FHttpResponsePtr, bool)> Callback);
    void HandleHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(FHttpResponsePtr, bool)> Callback);
    
    // 平台特定实现
    bool PublishToUnrealMarketplace(const FPluginMarketplaceInfo& PluginInfo, const FString& PackagePath);
    bool PublishToGitHub(const FPluginMarketplaceInfo& PluginInfo, const FString& PackagePath);
    bool PublishToNuGet(const FPluginMarketplaceInfo& PluginInfo, const FString& PackagePath);
    
    // 数据解析
    FPluginMarketplaceInfo ParsePluginInfo(const FString& JsonData);
    TArray<FPluginMarketplaceInfo> ParsePluginList(const FString& JsonData);
    
    // 配置管理
    void LoadConfiguration();
    void SaveConfiguration();
    
    // 数据成员
    TMap<EMarketplacePlatform, FString> PlatformApiKeys;
    TMap<EMarketplacePlatform, FString> PlatformBaseUrls;
    FString CacheDirectory;
    
    static UCSharpMarketplaceIntegration* Instance;
};
```

### 3.2 自动更新系统

```csharp
// UnrealEngine.Tools.UpdateManager.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Http;
using System.Threading.Tasks;
using System.Text.Json;

namespace UnrealEngine.Tools
{
    /// <summary>
    /// 更新信息
    /// </summary>
    public class UpdateInfo
    {
        public string Version { get; set; }
        public string ReleaseNotes { get; set; }
        public DateTime ReleaseDate { get; set; }
        public string DownloadUrl { get; set; }
        public long FileSize { get; set; }
        public string Checksum { get; set; }
        public bool IsSecurityUpdate { get; set; }
        public bool IsRequired { get; set; }
        public List<string> SupportedUEVersions { get; set; } = new List<string>();
    }
    
    /// <summary>
    /// 更新配置
    /// </summary>
    public class UpdateConfig
    {
        public bool AutoCheckForUpdates { get; set; } = true;
        public bool AutoDownloadUpdates { get; set; } = false;
        public bool AutoInstallUpdates { get; set; } = false;
        public int CheckIntervalHours { get; set; } = 24;
        public string UpdateChannel { get; set; } = "stable"; // stable, beta, alpha
        public bool IncludePrereleases { get; set; } = false;
        public string BackupDirectory { get; set; } = "Backups";
        public int MaxBackups { get; set; } = 5;
    }
    
    /// <summary>
    /// 更新管理器
    /// </summary>
    public static class UpdateManager
    {
        private static readonly HttpClient _httpClient = new HttpClient();
        private static UpdateConfig _config = new UpdateConfig();
        private static readonly string _configPath = "update-config.json";
        private static readonly string _updateApiUrl = "https://api.ue5-csharp.com/updates";
        
        public static event Action<UpdateInfo> UpdateAvailable;
        public static event Action<string, int> DownloadProgress; // filename, percentage
        public static event Action<bool, string> UpdateCompleted; // success, message
        
        /// <summary>
        /// 初始化更新管理器
        /// </summary>
        public static void Initialize()
        {
            LoadConfig();
            
            if (_config.AutoCheckForUpdates)
            {
                _ = Task.Run(async () =>
                {
                    while (true)
                    {
                        await CheckForUpdatesAsync();
                        await Task.Delay(TimeSpan.FromHours(_config.CheckIntervalHours));
                    }
                });
            }
        }
        
        /// <summary>
        /// 检查更新
        /// </summary>
        public static async Task<UpdateInfo> CheckForUpdatesAsync()
        {
            try
            {
                var currentVersion = GetCurrentVersion();
                var requestUrl = $"{_updateApiUrl}/check?version={currentVersion}&channel={_config.UpdateChannel}&ue_version={GetUEVersion()}";
                
                var response = await _httpClient.GetStringAsync(requestUrl);
                var updateInfo = JsonSerializer.Deserialize<UpdateInfo>(response);
                
                if (updateInfo != null && IsNewerVersion(updateInfo.Version, currentVersion))
                {
                    UE.LogInfo($"Update available: {updateInfo.Version}");
                    UpdateAvailable?.Invoke(updateInfo);
                    return updateInfo;
                }
                
                return null;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to check for updates: {ex.Message}");
                return null;
            }
        }
        
        /// <summary>
        /// 下载更新
        /// </summary>
        public static async Task<bool> DownloadUpdateAsync(UpdateInfo updateInfo)
        {
            try
            {
                var fileName = $"UE5CSharpPlugin-{updateInfo.Version}.zip";
                var downloadPath = Path.Combine(Path.GetTempPath(), fileName);
                
                using var response = await _httpClient.GetAsync(updateInfo.DownloadUrl, HttpCompletionOption.ResponseHeadersRead);
                response.EnsureSuccessStatusCode();
                
                var totalBytes = response.Content.Headers.ContentLength ?? 0;
                var downloadedBytes = 0L;
                
                using var contentStream = await response.Content.ReadAsStreamAsync();
                using var fileStream = new FileStream(downloadPath, FileMode.Create, FileAccess.Write, FileShare.None, 8192, true);
                
                var buffer = new byte[8192];
                int bytesRead;
                
                while ((bytesRead = await contentStream.ReadAsync(buffer, 0, buffer.Length)) > 0)
                {
                    await fileStream.WriteAsync(buffer, 0, bytesRead);
                    downloadedBytes += bytesRead;
                    
                    if (totalBytes > 0)
                    {
                        var percentage = (int)((downloadedBytes * 100) / totalBytes);
                        DownloadProgress?.Invoke(fileName, percentage);
                    }
                }
                
                // 验证校验和
                if (!string.IsNullOrEmpty(updateInfo.Checksum))
                {
                    var actualChecksum = CalculateChecksum(downloadPath);
                    if (actualChecksum != updateInfo.Checksum)
                    {
                        File.Delete(downloadPath);
                        throw new InvalidOperationException("Downloaded file checksum mismatch");
                    }
                }
                
                UE.LogInfo($"Update downloaded successfully: {downloadPath}");
                
                if (_config.AutoInstallUpdates)
                {
                    return await InstallUpdateAsync(downloadPath);
                }
                
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to download update: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 安装更新
        /// </summary>
        public static async Task<bool> InstallUpdateAsync(string updatePackagePath)
        {
            try
            {
                // 创建备份
                var backupPath = await CreateBackupAsync();
                if (string.IsNullOrEmpty(backupPath))
                {
                    UE.LogWarning("Failed to create backup, proceeding with installation");
                }
                
                // 停止相关服务
                await StopServicesAsync();
                
                // 解压更新包
                var tempExtractPath = Path.Combine(Path.GetTempPath(), "UE5CSharpUpdate");
                if (Directory.Exists(tempExtractPath))
                {
                    Directory.Delete(tempExtractPath, true);
                }
                
                System.IO.Compression.ZipFile.ExtractToDirectory(updatePackagePath, tempExtractPath);
                
                // 复制文件
                var pluginPath = GetPluginInstallPath();
                await CopyDirectoryAsync(tempExtractPath, pluginPath, true);
                
                // 清理临时文件
                Directory.Delete(tempExtractPath, true);
                File.Delete(updatePackagePath);
                
                // 重启服务
                await StartServicesAsync();
                
                UE.LogInfo("Update installed successfully");
                UpdateCompleted?.Invoke(true, "Update installed successfully");
                
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to install update: {ex.Message}");
                
                // 尝试恢复备份
                await RestoreBackupAsync();
                
                UpdateCompleted?.Invoke(false, $"Update installation failed: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 回滚到上一个版本
        /// </summary>
        public static async Task<bool> RollbackAsync()
        {
            try
            {
                var backupPath = GetLatestBackupPath();
                if (string.IsNullOrEmpty(backupPath) || !Directory.Exists(backupPath))
                {
                    UE.LogError("No backup available for rollback");
                    return false;
                }
                
                await StopServicesAsync();
                
                var pluginPath = GetPluginInstallPath();
                
                // 删除当前安装
                if (Directory.Exists(pluginPath))
                {
                    Directory.Delete(pluginPath, true);
                }
                
                // 恢复备份
                await CopyDirectoryAsync(backupPath, pluginPath, true);
                
                await StartServicesAsync();
                
                UE.LogInfo("Rollback completed successfully");
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to rollback: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 获取更新配置
        /// </summary>
        public static UpdateConfig GetConfig()
        {
            return _config;
        }
        
        /// <summary>
        /// 设置更新配置
        /// </summary>
        public static void SetConfig(UpdateConfig config)
        {
            _config = config;
            SaveConfig();
        }
        
        private static void LoadConfig()
        {
            try
            {
                if (File.Exists(_configPath))
                {
                    var json = File.ReadAllText(_configPath);
                    _config = JsonSerializer.Deserialize<UpdateConfig>(json) ?? new UpdateConfig();
                }
            }
            catch (Exception ex)
            {
                UE.LogWarning($"Failed to load update config: {ex.Message}");
                _config = new UpdateConfig();
            }
        }
        
        private static void SaveConfig()
        {
            try
            {
                var json = JsonSerializer.Serialize(_config, new JsonSerializerOptions { WriteIndented = true });
                File.WriteAllText(_configPath, json);
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to save update config: {ex.Message}");
            }
        }
        
        private static string GetCurrentVersion()
        {
            // 从插件描述文件或程序集信息获取当前版本
            return "1.0.0"; // 实际实现需要读取版本信息
        }
        
        private static string GetUEVersion()
        {
            // 获取当前UE5版本
            return "5.3.0"; // 实际实现需要检测UE版本
        }
        
        private static bool IsNewerVersion(string newVersion, string currentVersion)
        {
            try
            {
                var newVer = new Version(newVersion);
                var currentVer = new Version(currentVersion);
                return newVer > currentVer;
            }
            catch
            {
                return false;
            }
        }
        
        private static string CalculateChecksum(string filePath)
        {
            using var sha256 = System.Security.Cryptography.SHA256.Create();
            using var stream = File.OpenRead(filePath);
            var hash = sha256.ComputeHash(stream);
            return Convert.ToHexString(hash);
        }
        
        private static async Task<string> CreateBackupAsync()
        {
            try
            {
                var backupDir = Path.Combine(_config.BackupDirectory, $"backup-{DateTime.Now:yyyyMMdd-HHmmss}");
                Directory.CreateDirectory(backupDir);
                
                var pluginPath = GetPluginInstallPath();
                await CopyDirectoryAsync(pluginPath, backupDir, true);
                
                // 清理旧备份
                CleanupOldBackups();
                
                return backupDir;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to create backup: {ex.Message}");
                return null;
            }
        }
        
        private static void CleanupOldBackups()
        {
            try
            {
                if (!Directory.Exists(_config.BackupDirectory))
                    return;
                
                var backups = Directory.GetDirectories(_config.BackupDirectory)
                    .Select(d => new DirectoryInfo(d))
                    .OrderByDescending(d => d.CreationTime)
                    .Skip(_config.MaxBackups)
                    .ToArray();
                
                foreach (var backup in backups)
                {
                    backup.Delete(true);
                }
            }
            catch (Exception ex)
            {
                UE.LogWarning($"Failed to cleanup old backups: {ex.Message}");
            }
        }
        
        private static string GetLatestBackupPath()
        {
            try
            {
                if (!Directory.Exists(_config.BackupDirectory))
                    return null;
                
                return Directory.GetDirectories(_config.BackupDirectory)
                    .Select(d => new DirectoryInfo(d))
                    .OrderByDescending(d => d.CreationTime)
                    .FirstOrDefault()?.FullName;
            }
            catch
            {
                return null;
            }
        }
        
        private static async Task RestoreBackupAsync()
        {
            var backupPath = GetLatestBackupPath();
            if (!string.IsNullOrEmpty(backupPath))
            {
                await RollbackAsync();
            }
        }
        
        private static async Task StopServicesAsync()
        {
            // 停止相关服务和进程
            await Task.Delay(100); // 占位实现
        }
        
        private static async Task StartServicesAsync()
        {
            // 启动相关服务和进程
            await Task.Delay(100); // 占位实现
        }
        
        private static string GetPluginInstallPath()
        {
            // 获取插件安装路径
            return @"C:\UnrealEngine\Engine\Plugins\Marketplace\UCSharpPlugin"; // 示例路径
        }
        
        private static async Task CopyDirectoryAsync(string sourceDir, string destDir, bool recursive)
        {
            var dir = new DirectoryInfo(sourceDir);
            
            if (!dir.Exists)
                throw new DirectoryNotFoundException($"Source directory not found: {sourceDir}");
            
            DirectoryInfo[] dirs = dir.GetDirectories();
            Directory.CreateDirectory(destDir);
            
            foreach (FileInfo file in dir.GetFiles())
            {
                string targetFilePath = Path.Combine(destDir, file.Name);
                file.CopyTo(targetFilePath, true);
            }
            
            if (recursive)
            {
                foreach (DirectoryInfo subDir in dirs)
                {
                    string newDestinationDir = Path.Combine(destDir, subDir.Name);
                    await CopyDirectoryAsync(subDir.FullName, newDestinationDir, true);
                }
            }
        }
    }
}
```

## 4. 许可证管理系统

### 4.1 许可证验证

```csharp
// UnrealEngine.Licensing.LicenseManager.cs
using System;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace UnrealEngine.Licensing
{
    /// <summary>
    /// 许可证类型
    /// </summary>
    public enum LicenseType
    {
        Trial,
        Personal,
        Professional,
        Enterprise,
        Educational,
        OpenSource
    }
    
    /// <summary>
    /// 许可证信息
    /// </summary>
    public class LicenseInfo
    {
        public string LicenseKey { get; set; }
        public LicenseType Type { get; set; }
        public string Licensee { get; set; }
        public string Organization { get; set; }
        public DateTime IssueDate { get; set; }
        public DateTime ExpiryDate { get; set; }
        public int MaxUsers { get; set; }
        public List<string> AllowedFeatures { get; set; } = new List<string>();
        public Dictionary<string, object> CustomProperties { get; set; } = new Dictionary<string, object>();
        public string Signature { get; set; }
        
        public bool IsValid => DateTime.Now <= ExpiryDate;
        public bool IsExpired => DateTime.Now > ExpiryDate;
        public TimeSpan TimeRemaining => ExpiryDate - DateTime.Now;
    }
    
    /// <summary>
    /// 许可证管理器
    /// </summary>
    public static class LicenseManager
    {
        private static LicenseInfo _currentLicense;
        private static readonly string _licenseFilePath = "license.dat";
        private static readonly string _publicKey = "<RSA_PUBLIC_KEY>"; // 实际部署时使用真实公钥
        
        public static event Action<LicenseInfo> LicenseChanged;
        public static event Action LicenseExpired;
        public static event Action<int> LicenseExpiringWarning; // days remaining
        
        /// <summary>
        /// 初始化许可证管理器
        /// </summary>
        public static void Initialize()
        {
            LoadLicense();
            
            // 启动许可证监控
            _ = Task.Run(async () =>
            {
                while (true)
                {
                    CheckLicenseStatus();
                    await Task.Delay(TimeSpan.FromHours(1));
                }
            });
        }
        
        /// <summary>
        /// 安装许可证
        /// </summary>
        public static bool InstallLicense(string licenseKey)
        {
            try
            {
                var licenseInfo = ParseLicenseKey(licenseKey);
                if (licenseInfo == null)
                {
                    UE.LogError("Invalid license key format");
                    return false;
                }
                
                if (!VerifyLicenseSignature(licenseInfo))
                {
                    UE.LogError("License signature verification failed");
                    return false;
                }
                
                if (licenseInfo.IsExpired)
                {
                    UE.LogError("License has expired");
                    return false;
                }
                
                _currentLicense = licenseInfo;
                SaveLicense();
                
                UE.LogInfo($"License installed successfully: {licenseInfo.Type} license for {licenseInfo.Licensee}");
                LicenseChanged?.Invoke(_currentLicense);
                
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to install license: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 移除许可证
        /// </summary>
        public static void RemoveLicense()
        {
            _currentLicense = null;
            
            if (File.Exists(_licenseFilePath))
            {
                File.Delete(_licenseFilePath);
            }
            
            UE.LogInfo("License removed");
            LicenseChanged?.Invoke(null);
        }
        
        /// <summary>
        /// 获取当前许可证
        /// </summary>
        public static LicenseInfo GetCurrentLicense()
        {
            return _currentLicense;
        }
        
        /// <summary>
        /// 检查功能是否被许可
        /// </summary>
        public static bool IsFeatureAllowed(string featureName)
        {
            if (_currentLicense == null)
            {
                // 试用模式下允许基本功能
                return IsTrialFeature(featureName);
            }
            
            if (_currentLicense.IsExpired)
            {
                return false;
            }
            
            return _currentLicense.AllowedFeatures.Contains(featureName) || 
                   _currentLicense.AllowedFeatures.Contains("*");
        }
        
        /// <summary>
        /// 检查用户数量限制
        /// </summary>
        public static bool CheckUserLimit(int currentUsers)
        {
            if (_currentLicense == null)
            {
                return currentUsers <= 1; // 试用版限制1个用户
            }
            
            return currentUsers <= _currentLicense.MaxUsers;
        }
        
        /// <summary>
        /// 生成试用许可证
        /// </summary>
        public static bool GenerateTrialLicense(string userEmail, int trialDays = 30)
        {
            try
            {
                var trialLicense = new LicenseInfo
                {
                    LicenseKey = GenerateTrialKey(),
                    Type = LicenseType.Trial,
                    Licensee = userEmail,
                    IssueDate = DateTime.Now,
                    ExpiryDate = DateTime.Now.AddDays(trialDays),
                    MaxUsers = 1,
                    AllowedFeatures = GetTrialFeatures()
                };
                
                _currentLicense = trialLicense;
                SaveLicense();
                
                UE.LogInfo($"Trial license generated for {userEmail}, expires on {trialLicense.ExpiryDate:yyyy-MM-dd}");
                LicenseChanged?.Invoke(_currentLicense);
                
                return true;
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to generate trial license: {ex.Message}");
                return false;
            }
        }
        
        /// <summary>
        /// 在线验证许可证
        /// </summary>
        public static async Task<bool> ValidateLicenseOnlineAsync()
        {
            if (_currentLicense == null)
                return false;
                
            try
            {
                using var httpClient = new HttpClient();
                var validationData = new
                {
                    licenseKey = _currentLicense.LicenseKey,
                    machineId = GetMachineId(),
                    version = GetPluginVersion()
                };
                
                var json = JsonSerializer.Serialize(validationData);
                var content = new StringContent(json, Encoding.UTF8, "application/json");
                
                var response = await httpClient.PostAsync("https://api.ue5-csharp.com/license/validate", content);
                
                if (response.IsSuccessStatusCode)
                {
                    var responseJson = await response.Content.ReadAsStringAsync();
                    var result = JsonSerializer.Deserialize<Dictionary<string, object>>(responseJson);
                    
                    return result.ContainsKey("valid") && (bool)result["valid"];
                }
                
                return false;
            }
            catch (Exception ex)
            {
                UE.LogWarning($"Online license validation failed: {ex.Message}");
                // 离线模式下继续使用本地验证
                return _currentLicense.IsValid;
            }
        }
        
        private static void LoadLicense()
        {
            try
            {
                if (File.Exists(_licenseFilePath))
                {
                    var encryptedData = File.ReadAllBytes(_licenseFilePath);
                    var decryptedData = DecryptLicenseData(encryptedData);
                    _currentLicense = JsonSerializer.Deserialize<LicenseInfo>(decryptedData);
                    
                    if (_currentLicense != null && !VerifyLicenseSignature(_currentLicense))
                    {
                        UE.LogWarning("License signature verification failed, removing invalid license");
                        _currentLicense = null;
                        File.Delete(_licenseFilePath);
                    }
                }
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to load license: {ex.Message}");
                _currentLicense = null;
            }
        }
        
        private static void SaveLicense()
        {
            try
            {
                if (_currentLicense != null)
                {
                    var json = JsonSerializer.Serialize(_currentLicense);
                    var encryptedData = EncryptLicenseData(json);
                    File.WriteAllBytes(_licenseFilePath, encryptedData);
                }
            }
            catch (Exception ex)
            {
                UE.LogError($"Failed to save license: {ex.Message}");
            }
        }
        
        private static void CheckLicenseStatus()
        {
            if (_currentLicense == null)
                return;
                
            if (_currentLicense.IsExpired)
            {
                UE.LogWarning("License has expired");
                LicenseExpired?.Invoke();
                return;
            }
            
            var daysRemaining = (int)_currentLicense.TimeRemaining.TotalDays;
            if (daysRemaining <= 7 && daysRemaining > 0)
            {
                UE.LogWarning($"License expires in {daysRemaining} days");
                LicenseExpiringWarning?.Invoke(daysRemaining);
            }
        }
        
        private static LicenseInfo ParseLicenseKey(string licenseKey)
        {
            try
            {
                // 解析Base64编码的许可证密钥
                var decodedBytes = Convert.FromBase64String(licenseKey);
                var json = Encoding.UTF8.GetString(decodedBytes);
                return JsonSerializer.Deserialize<LicenseInfo>(json);
            }
            catch
            {
                return null;
            }
        }
        
        private static bool VerifyLicenseSignature(LicenseInfo license)
        {
            try
            {
                // 创建待验证的数据
                var dataToVerify = $"{license.LicenseKey}{license.Type}{license.Licensee}{license.ExpiryDate:yyyy-MM-dd}";
                var dataBytes = Encoding.UTF8.GetBytes(dataToVerify);
                var signatureBytes = Convert.FromBase64String(license.Signature);
                
                // 使用RSA公钥验证签名
                using var rsa = RSA.Create();
                rsa.ImportFromPem(_publicKey);
                
                return rsa.VerifyData(dataBytes, signatureBytes, HashAlgorithmName.SHA256, RSASignaturePadding.Pkcs1);
            }
            catch
            {
                return false;
            }
        }
        
        private static bool IsTrialFeature(string featureName)
        {
            var trialFeatures = GetTrialFeatures();
            return trialFeatures.Contains(featureName);
        }
        
        private static List<string> GetTrialFeatures()
        {
            return new List<string>
            {
                "basic_scripting",
                "debugging",
                "code_completion",
                "simple_api_access"
            };
        }
        
        private static string GenerateTrialKey()
        {
            return Guid.NewGuid().ToString("N")[..16].ToUpper();
        }
        
        private static string GetMachineId()
        {
            // 生成机器唯一标识
            var machineInfo = $"{Environment.MachineName}{Environment.UserName}{Environment.OSVersion}";
            using var sha256 = SHA256.Create();
            var hash = sha256.ComputeHash(Encoding.UTF8.GetBytes(machineInfo));
            return Convert.ToHexString(hash)[..16];
        }
        
        private static string GetPluginVersion()
        {
            return "1.0.0"; // 实际实现需要读取版本信息
        }
        
        private static byte[] EncryptLicenseData(string data)
        {
            // 简单的加密实现，实际部署时应使用更强的加密
            var bytes = Encoding.UTF8.GetBytes(data);
            var key = Encoding.UTF8.GetBytes("UE5CSharpLicenseKey123456789012")[..32];
            
            using var aes = Aes.Create();
            aes.Key = key;
            aes.GenerateIV();
            
            using var encryptor = aes.CreateEncryptor();
            using var ms = new MemoryStream();
            ms.Write(aes.IV, 0, aes.IV.Length);
            
            using var cs = new CryptoStream(ms, encryptor, CryptoStreamMode.Write);
            cs.Write(bytes, 0, bytes.Length);
            cs.FlushFinalBlock();
            
            return ms.ToArray();
        }
        
        private static string DecryptLicenseData(byte[] encryptedData)
        {
            var key = Encoding.UTF8.GetBytes("UE5CSharpLicenseKey123456789012")[..32];
            
            using var aes = Aes.Create();
            aes.Key = key;
            
            var iv = new byte[16];
            Array.Copy(encryptedData, 0, iv, 0, 16);
            aes.IV = iv;
            
            using var decryptor = aes.CreateDecryptor();
            using var ms = new MemoryStream(encryptedData, 16, encryptedData.Length - 16);
            using var cs = new CryptoStream(ms, decryptor, CryptoStreamMode.Read);
            using var reader = new StreamReader(cs);
            
            return reader.ReadToEnd();
        }
    }
}
```

## 5. 总结

本设计文档详细阐述了UE5 C#插件的部署和分发机制，主要包括：

### 5.1 核心特性

1. **自动化构建系统**
   - CI/CD流水线
   - 多平台支持
   - 自动化测试
   - 包管理和分发

2. **多渠道分发**
   - 官方市场集成
   - 第三方平台支持
   - 企业私有分发
   - 版本管理

3. **智能更新系统**
   - 自动更新检查
   - 增量更新下载
   - 备份和回滚
   - 配置管理

4. **许可证管理**
   - 多种许可证类型
   - 在线/离线验证
   - 试用期管理
   - 功能权限控制

### 5.2 技术优势

- **标准化流程**：使用业界标准的CI/CD工具和流程
- **多平台支持**：支持Windows、Linux、Mac等主流平台
- **安全可靠**：包含完整的签名验证和加密机制
- **用户友好**：提供简单易用的安装和更新体验

### 5.3 实施建议

1. **分阶段部署**：先实现基础功能，再逐步完善高级特性
2. **安全优先**：重视代码签名、许可证验证等安全机制
3. **用户体验**：提供清晰的安装指南和故障排除文档
4. **监控反馈**：建立完善的使用统计和错误报告系统

这套部署和分发机制将确保UE5 C#插件能够安全、可靠地交付给用户，并提供良好的维护和更新体验。