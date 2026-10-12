param([string]$EngineRoot = 'F:\Games\UE_5.8', [switch]$ReuseCook, [switch]$IterativeCook)
$ErrorActionPreference = 'Stop'
$ceRoot = Split-Path -Parent $PSScriptRoot
$ceProject = Join-Path $ceRoot 'CeroEstrellas.uproject'
$ceCookArg = if ($ReuseCook) { '-skipcook' } else { '-cook' }
if ($ReuseCook -and $IterativeCook) { throw 'ReuseCook skips cooking; use IterativeCook on its own to update changed assets.' }
$ceCookExtras=@()
if ($IterativeCook) { $ceCookExtras += '-iterativecooking' }
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$ceProject" -noP4 -platform=Win64 -clientconfig=Development -build $ceCookArg @ceCookExtras -stage -pak -iostore -archive "-archivedirectory=$ceRoot\Release" -utf8output
if ($LASTEXITCODE -ne 0) { throw "Packaging failed: $LASTEXITCODE" }


