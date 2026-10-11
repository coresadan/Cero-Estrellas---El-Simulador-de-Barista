param([string]$EngineRoot = 'F:\Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$ceRoot = Split-Path -Parent $PSScriptRoot
$ceProject = Join-Path $ceRoot 'CeroEstrellas.uproject'
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') CeroEstrellasEditor Win64 Development $ceProject -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Editor build failed: $LASTEXITCODE" }


