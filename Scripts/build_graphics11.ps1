param([string]$EngineRoot='F:\Games\UE_5.8',[switch]$SkipImport)
$ErrorActionPreference='Stop'
$ceRoot=Split-Path -Parent $PSScriptRoot
$ceProject=Join-Path $ceRoot 'CeroEstrellas.uproject'
$ceEditor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if(!$SkipImport) {
    $ceScript=(Join-Path $PSScriptRoot 'import_all11.py').Replace('\','/')
    & $ceEditor $ceProject -run=pythonscript "-script=$ceScript" -unattended -nop4 -nosplash -UTF8Output "-abslog=$ceRoot/Evidence/entrega11_import_runtime.log"
    if($LASTEXITCODE -ne 0){throw 'Graphics import failed.'}
}
& $ceEditor $ceProject -run=ResavePackages -buildlighting -buildreflectioncaptures -allowcommandletrendering -map=Cafeteria -quality=Production -projectonly -unattended -nop4 -nosplash -UTF8Output "-abslog=$ceRoot/Evidence/entrega11_lightmass_runtime.log"
if($LASTEXITCODE -ne 0){throw 'Lightmass build failed.'}
if(!(Test-Path -LiteralPath (Join-Path $ceRoot 'Content/Maps/Cafeteria_BuiltData.uasset'))){throw 'Lightmass did not produce saved map build data.'}
$ceFinalize=(Join-Path $PSScriptRoot 'finalize_cafe11.py').Replace('\','/')
& $ceEditor $ceProject -run=pythonscript "-script=$ceFinalize" -unattended -nop4 -nosplash -UTF8Output "-abslog=$ceRoot/Evidence/entrega11_scene_audit_runtime.log"
if($LASTEXITCODE -ne 0){throw 'Saved scene or shader usage audit failed.'}
