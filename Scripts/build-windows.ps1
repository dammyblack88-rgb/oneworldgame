param(
    [Parameter(Mandatory=$true)][string]$UERoot,
    [string]$ProjectPath,
    [string]$OutputDir
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
if ([System.Environment]::OSVersion.Platform -ne [System.PlatformID]::Win32NT) { throw 'Native Windows build host required.' }
if (-not $ProjectPath) { $ProjectPath = Join-Path $Root 'ParadiseHeat/ParadiseHeat.uproject' }
if (-not $OutputDir) { $OutputDir = Join-Path $Root 'artifacts/windows' }
$UAT = Join-Path $UERoot 'Engine/Build/BatchFiles/RunUAT.bat'
if (-not (Test-Path -LiteralPath $UAT -PathType Leaf)) { throw "Unreal Automation Tool missing: $UAT" }
if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf)) { throw "Unreal project missing: $ProjectPath" }
$LogDir = Join-Path $Root 'artifacts/logs'
New-Item -ItemType Directory -Force -Path $OutputDir, $LogDir | Out-Null
$Log = Join-Path $LogDir ("windows-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
& $UAT BuildCookRun "-project=$ProjectPath" -noP4 -platform=Win64 -clientconfig=Development -build -cook -allmaps -stage -pak -package -archive "-archivedirectory=$OutputDir" -utf8output 2>&1 | Tee-Object -FilePath $Log
if ($LASTEXITCODE -ne 0) { throw "UAT failed with exit code $LASTEXITCODE. See $Log" }
Write-Output "UAT completed. Inspect package under $OutputDir; startup/gameplay remain unverified. Log: $Log"
