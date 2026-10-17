param([string]$EngineRoot='F:\Games\UE_5.8',[switch]$SkipImport)
$ErrorActionPreference='Stop'
$ceRoot=Split-Path -Parent $PSScriptRoot
$ceProject=Join-Path $ceRoot 'CeroEstrellas.uproject'
$ceEditor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if(!$SkipImport) {
    $ceScript=(Join-Path $PSScriptRoot 'author_cafe12.py').Replace('\','/')
    & $ceEditor $ceProject -run=pythonscript "-script=$ceScript" -unattended -nop4 -nosplash -UTF8Output "-abslog=$ceRoot/Evidence/entrega12_import_runtime.log"
    if($LASTEXITCODE -ne 0){throw 'Graphics import failed.'}
}
if(!(Get-Process -Name SwarmAgent -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/DotNET/SwarmAgent.exe') -WorkingDirectory (Join-Path $EngineRoot 'Engine/Binaries/DotNET') -WindowStyle Hidden | Out-Null
}
$ceSwarmReady=$false
$ceSwarmDeadline=(Get-Date).AddSeconds(45)
while((Get-Date) -lt $ceSwarmDeadline -and !$ceSwarmReady) {
    $ceSocket=[Net.Sockets.TcpClient]::new()
    try { $ceSwarmReady=$ceSocket.ConnectAsync('127.0.0.1',8008).Wait(250) -and $ceSocket.Connected } catch {} finally { $ceSocket.Dispose() }
    if(!$ceSwarmReady) { Start-Sleep -Milliseconds 250 }
}
if(!$ceSwarmReady) { throw 'Swarm Agent is not listening on localhost:8008.' }
& $ceEditor $ceProject -run=ResavePackages -buildlighting -buildreflectioncaptures -allowcommandletrendering -map=Cafeteria -quality=Production -projectonly -unattended -nop4 -nosplash -UTF8Output "-abslog=$ceRoot/Evidence/entrega12_lightmass_runtime.log"
if($LASTEXITCODE -ne 0){throw 'Lightmass build failed.'}
if(!(Test-Path -LiteralPath (Join-Path $ceRoot 'Content/Maps/Cafeteria_BuiltData.uasset'))){throw 'Lightmass did not produce saved map build data.'}
$ceFinalize=(Join-Path $PSScriptRoot 'finalize_cafe12.py').Replace('\','/')
& $ceEditor $ceProject -run=pythonscript "-script=$ceFinalize" -unattended -nop4 -nosplash -UTF8Output "-abslog=$ceRoot/Evidence/entrega12_scene_audit_runtime.log"
if($LASTEXITCODE -ne 0){throw 'Saved scene or shader usage audit failed.'}
