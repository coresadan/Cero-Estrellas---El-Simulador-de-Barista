param([string]$EngineRoot = 'F:\Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$ceRoot = Split-Path -Parent $PSScriptRoot
$ceProject = Join-Path $ceRoot 'CeroEstrellas.uproject'
$ceScript = (Join-Path $ceRoot 'Scripts\build_content.py').Replace('\','/')
& (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') $ceProject -run=pythonscript "-script=$ceScript" -unattended -nop4 -nosplash -UTF8Output
if ($LASTEXITCODE -ne 0) { throw "Content generation failed: $LASTEXITCODE" }

