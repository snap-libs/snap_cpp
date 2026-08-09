# ==============================================================================
# SNAP Engine Asset Initializer Script for Windows (PowerShell)
# ==============================================================================

[CmdletBinding()]
param (
    [Parameter(Position=0)]
    [string]$TargetDir = "",
    
    [alias("y")]
    [switch]$Yes
)

$ErrorActionPreference = "Stop"

$HF_REPO_URL = "https://huggingface.co/softguy777/snap-models"
$HF_ZIP_URL = "https://huggingface.co/softguy777/snap-models/resolve/main/snap-models.zip"

# 1. Resolve Target Directory
if ([string]::IsNullOrWhiteSpace($TargetDir)) {
    $TargetDir = Get-Location
}

if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

$AbsTargetDir = (Get-Item $TargetDir).FullName

Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "  SNAP Engine Initializer (Windows PowerShell)" -ForegroundColor Cyan
Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "  - Target SNAP Root: $AbsTargetDir" -ForegroundColor Yellow
Write-Host "==================================================================" -ForegroundColor Cyan

# 2. Dependency Check (MSVC C++ Redistributable x64 / ARM64)
Write-Host ""
Write-Host "[0/2] Checking C++ Runtime Dependencies..." -ForegroundColor Green
$VcRuntimeDll = Join-Path $env:SystemRoot "System32\vcruntime140.dll"

if (-not (Test-Path $VcRuntimeDll)) {
    Write-Host " [WARNING] Microsoft Visual C++ 2015-2022 Redistributable (x64/ARM64) is NOT detected!" -ForegroundColor Red
    Write-Host "           Required runtime (vcruntime140.dll) is missing for snap_cpp.dll execution." -ForegroundColor Yellow
    Write-Host "           Please download and install the official Microsoft C++ Redistributable:" -ForegroundColor Yellow
    Write-Host "           👉 x64:   https://aka.ms/vs/17/release/vc_redist.x64.exe" -ForegroundColor Cyan
    Write-Host "           👉 ARM64: https://aka.ms/vs/17/release/vc_redist.arm64.exe" -ForegroundColor Cyan
} else {
    Write-Host " [OK] MSVC C++ Runtime (vcruntime140.dll) detected in System32." -ForegroundColor Green
}

# 3. Interactive Confirmation
if (-not $Yes) {
    $Confirm = Read-Host "Do you want to set '$AbsTargetDir' as SNAP Root and download models? (y/N)"
    if ($Confirm -notmatch "^[yY]([eE][sS])?$") {
        Write-Host "[CANCELLED] Initialization aborted by user." -ForegroundColor Red
        exit 0
    }
}

$ModelsDir = Join-Path $AbsTargetDir "models"
Write-Host ""
Write-Host "[1/2] Downloading SNAP models & lexicons into: $ModelsDir" -ForegroundColor Green

# 4. High-Speed Direct Model Archive Download
$TempZip = Join-Path $env:TEMP "snap_models_$([Guid]::NewGuid().ToString('N')).zip"
$CurlCmd = Get-Command "curl.exe" -ErrorAction SilentlyContinue

try {
    Write-Host " -> Downloading SNAP model package (fast direct download)..." -ForegroundColor Gray
    if ($CurlCmd) {
        curl.exe -L --progress-bar $HF_ZIP_URL -o $TempZip
    } else {
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri $HF_ZIP_URL -OutFile $TempZip
    }
    
    Write-Host " -> Extracting model package into $ModelsDir..." -ForegroundColor Gray
    if (-not (Test-Path $ModelsDir)) {
        New-Item -ItemType Directory -Path $ModelsDir -Force | Out-Null
    }
    Expand-Archive -Path $TempZip -DestinationPath $ModelsDir -Force
    
    # Flatten if unzipped into nested snap-models folder
    $NestedDir = Join-Path $ModelsDir "snap-models"
    if (Test-Path $NestedDir) {
        Get-ChildItem -Path $NestedDir | Move-Item -Destination $ModelsDir -Force
        Remove-Item $NestedDir -Recurse -Force -ErrorAction SilentlyContinue
    }
} finally {
    if (Test-Path $TempZip) {
        Remove-Item $TempZip -Force -ErrorAction SilentlyContinue
    }
}

# 5. Environment Variable Setup (Windows Registry / User Env)
Write-Host ""
Write-Host "[2/2] Configuring Environment Variable (SNAP_HOME)..." -ForegroundColor Green

if (-not $Yes) {
    $SetEnv = Read-Host "Would you like to set SNAP_HOME user environment variable to '$AbsTargetDir'? (y/N)"
} else {
    $SetEnv = "y"
}

if ($SetEnv -match "^[yY]([eE][sS])?$") {
    [Environment]::SetEnvironmentVariable("SNAP_HOME", $AbsTargetDir, "User")
    Write-Host " [OK] SNAP_HOME environment variable successfully set for User." -ForegroundColor Green
    Write-Host "      (Please restart active PowerShell/CMD terminals to reflect the change)" -ForegroundColor Yellow
} else {
    Write-Host " -> Skipped setting environment variable." -ForegroundColor Gray
}

# 6. Asset Self-Verification Check
Write-Host ""
Write-Host "[Verification] Checking asset integrity..." -ForegroundColor Green
$ManifestPath = Join-Path $ModelsDir "manifest.json"
if (Test-Path $ManifestPath) {
    Write-Host " [OK] Models manifest successfully verified: $ManifestPath" -ForegroundColor Green
} else {
    Write-Host " [WARNING] manifest.json missing in $ModelsDir!" -ForegroundColor Red
}

Write-Host ""
Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "  Initialization Complete!" -ForegroundColor Green
Write-Host "  You can now run SNAP C++ / Python SDK." -ForegroundColor Green
Write-Host "==================================================================" -ForegroundColor Cyan
