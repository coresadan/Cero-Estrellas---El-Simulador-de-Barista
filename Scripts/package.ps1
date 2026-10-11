param([string]$EngineRoot = 'F:\Games\UE_5.8', [switch]$ReuseCook)
$ErrorActionPreference = 'Stop'
$ceRoot = Split-Path -Parent $PSScriptRoot
$ceProject = Join-Path $ceRoot 'CeroEstrellas.uproject'
$ceCookArg = if ($ReuseCook) { '-skipcook' } else { '-cook' }
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$ceProject" -noP4 -platform=Win64 -clientconfig=Development -build $ceCookArg -stage -pak -iostore -archive "-archivedirectory=$ceRoot\Release" -utf8output
if ($LASTEXITCODE -ne 0) { throw "Packaging failed: $LASTEXITCODE" }


