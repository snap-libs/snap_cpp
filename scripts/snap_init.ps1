# ==============================================================================
# SNAP Engine Asset Initializer Script for Windows (PowerShell)
# ==============================================================================

[CmdletBinding()]
param (
    [Parameter(Position=0)]
    [string]$TargetDir = "",
    
    [alias("l", "language")]
    [string]$Lang = "all",

    [alias("y")]
    [switch]$Yes
)

$ErrorActionPreference = "Stop"

$HF_REPO_URL = "https://huggingface.co/softguy777/snap-weights"
$HF_ARCHIVE_URL = "https://huggingface.co/softguy777/snap-weights/archive/main.zip"

$ALL_LANGS = @("ko", "ja", "en")

# 0. Parse Target Languages
$LangInput = $Lang.ToLower().Trim()
if ([string]::IsNullOrWhiteSpace($LangInput) -or $LangInput -eq "all" -or $LangInput -eq "*") {
    $TargetLangs = $ALL_LANGS
    $IsAllLangs = $true
} else {
    $Parsed = $LangInput.Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
    $TargetLangs = $Parsed | Where-Object { $ALL_LANGS -contains $_ }
    if ($TargetLangs.Count -eq 0) {
        Write-Host " [WARNING] No valid language specified in '$Lang'. Defaulting to ALL languages." -ForegroundColor Yellow
        $TargetLangs = $ALL_LANGS
        $IsAllLangs = $true
    } else {
        $IsAllLangs = ($TargetLangs.Count -eq $ALL_LANGS.Count)
    }
}

# Helper to prune unselected language folders
function Prune-UnselectedLanguages {
    param(
        [string]$ModelsPath,
        [string[]]$KeepLangs
    )
    foreach ($l in $ALL_LANGS) {
        if ($KeepLangs -notcontains $l) {
            $DirToPrune = Join-Path $ModelsPath $l
            if (Test-Path $DirToPrune) {
                Write-Host " -> Pruning unselected language directory: $DirToPrune" -ForegroundColor Gray
                Remove-Item $DirToPrune -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
    }
}

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
Write-Host "  - Target SNAP Root : $AbsTargetDir" -ForegroundColor Yellow
Write-Host "  - Target Languages : $($TargetLangs -join ', ')" -ForegroundColor Yellow
Write-Host "==================================================================" -ForegroundColor Cyan

# 2. Dependency Check (MSVC C++ Redistributable x64 / ARM64)
Write-Host ""
Write-Host "[0/2] Checking C++ Runtime Dependencies..." -ForegroundColor Green
$VcRuntimeDll = Join-Path $env:SystemRoot "System32\vcruntime140.dll"

if (-not (Test-Path $VcRuntimeDll)) {
    Write-Host " [WARNING] Microsoft Visual C++ 2015-2022 Redistributable is NOT detected!" -ForegroundColor Red
    Write-Host "           Required runtime (vcruntime140.dll) is missing for snap_cpp.dll execution." -ForegroundColor Yellow
} else {
    Write-Host " [OK] MSVC C++ Runtime (vcruntime140.dll) detected in System32." -ForegroundColor Green
}

# 3. Interactive Confirmation
if (-not $Yes) {
    $Confirm = Read-Host "Do you want to set '$AbsTargetDir' as SNAP Root and download models ($($TargetLangs -join ', '))? (y/N)"
    if ($Confirm -notmatch "^[yY]([eE][sS])?$") {
        Write-Host "[CANCELLED] Initialization aborted by user." -ForegroundColor Red
        exit 0
    }
}

$ModelsDir = Join-Path $AbsTargetDir "models"
Write-Host ""
Write-Host "[1/2] Downloading SNAP models & lexicons ($($TargetLangs -join ', ')) into: $ModelsDir" -ForegroundColor Green

# 4. Standard Git Clone / Sparse Checkout / Main Archive Download
$GitCmd = Get-Command "git.exe" -ErrorAction SilentlyContinue

if ($GitCmd) {
    Write-Host " -> 'git' detected. Initializing models from Hugging Face repository..." -ForegroundColor Gray
    if (Test-Path $ModelsDir) {
        Remove-Item $ModelsDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    
    if ($IsAllLangs) {
        git clone --depth 1 $HF_REPO_URL $ModelsDir
    } else {
        Write-Host " -> Pinpoint downloading selected language(s) [$($TargetLangs -join ', ')] via Git sparse-checkout..." -ForegroundColor Cyan
        git clone --depth 1 --no-checkout $HF_REPO_URL $ModelsDir
        if (Test-Path $ModelsDir) {
            Push-Location $ModelsDir
            try {
                git sparse-checkout init --cone
                git sparse-checkout set $TargetLangs
                git checkout
            } catch {
                Write-Host " [WARNING] Sparse checkout warning/fallback. Ensuring pinpoint language pruning..." -ForegroundColor Yellow
                git checkout
            } finally {
                Pop-Location
            }
            Prune-UnselectedLanguages -ModelsPath $ModelsDir -KeepLangs $TargetLangs
        }
    }
} else {
    Write-Host " -> 'git' not found. Falling back to direct Hugging Face main archive download..." -ForegroundColor Gray
    $TempZip = Join-Path $env:TEMP "snap_models_$([Guid]::NewGuid().ToString('N')).zip"
    try {
        $CurlCmd = Get-Command "curl.exe" -ErrorAction SilentlyContinue
        if ($CurlCmd) {
            curl.exe -L --progress-bar $HF_ARCHIVE_URL -o $TempZip
        } else {
            $ProgressPreference = 'SilentlyContinue'
            Invoke-WebRequest -Uri $HF_ARCHIVE_URL -OutFile $TempZip
        }
        
        if (-not (Test-Path $ModelsDir)) {
            New-Item -ItemType Directory -Path $ModelsDir -Force | Out-Null
        }
        Expand-Archive -Path $TempZip -DestinationPath $ModelsDir -Force
        
        # Flatten nested directory if created by Hugging Face archive
        $NestedDir = Join-Path $ModelsDir "snap-weights-main"
        if (Test-Path $NestedDir) {
            Get-ChildItem -Path $NestedDir | Move-Item -Destination $ModelsDir -Force
            Remove-Item $NestedDir -Recurse -Force -ErrorAction SilentlyContinue
        }

        # Prune unselected language directories
        Prune-UnselectedLanguages -ModelsPath $ModelsDir -KeepLangs $TargetLangs
    } finally {
        if (Test-Path $TempZip) {
            Remove-Item $TempZip -Force -ErrorAction SilentlyContinue
        }
    }
}

# 5. Environment Variable Setup (SNAP_HOME)
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
} else {
    Write-Host " -> Skipped setting environment variable." -ForegroundColor Gray
}

Write-Host ""
Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "  Initialization Complete!" -ForegroundColor Green
Write-Host "  Selected models ($($TargetLangs -join ', ')) ready in: $ModelsDir" -ForegroundColor Green
Write-Host "==================================================================" -ForegroundColor Cyan
