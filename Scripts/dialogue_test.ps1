param([switch]$Packaged,[switch]$Offscreen,[string]$EvidenceName='DialogoEditor',[string]$EngineRoot='F:\Games\UE_5.8',[ValidateRange(640,3840)][int]$ResX=1280,[ValidateRange(360,2160)][int]$ResY=720)
$ErrorActionPreference='Stop'
$ceRoot=Split-Path -Parent $PSScriptRoot
$ceEvidence=Join-Path $ceRoot ('Evidence/'+$EvidenceName)
New-Item -ItemType Directory -Path $ceEvidence -Force | Out-Null
$ceLog=Join-Path $ceEvidence 'runtime.log'
$ceResult=Join-Path $ceEvidence 'dialogue_result.json'
$ceBefore=if(Test-Path $ceResult){(Get-Item $ceResult).LastWriteTimeUtc}else{[DateTime]::MinValue}
$ceArgs=@('-windowed',"-ResX=$ResX","-ResY=$ResY",'-CeroDialogueTest','-unattended','-NoSplash',('"-CeroEvidenceDir='+$ceEvidence+'"'),('"-CeroTestWav='+(Join-Path $ceRoot 'SourceAudio/Voice/dialogue_test.wav')+'"'),('"-abslog='+$ceLog+'"'))
if($Packaged){$ceExe=Join-Path $ceRoot 'Release/Windows/CeroEstrellas.exe'}else{
    $ceExe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
    $ceArgs=@(('"'+(Join-Path $ceRoot 'CeroEstrellas.uproject')+'"'),'-game')+$ceArgs
}
if($Offscreen){$ceArgs += @('-RenderOffScreen','-ForceRes')}
$ceProcess=Start-Process -FilePath $ceExe -ArgumentList $ceArgs -WorkingDirectory $ceRoot -WindowStyle Hidden -PassThru -Wait
if($ceProcess.ExitCode -ne 0){throw 'Ha fallado la prueba de dialogo. Revisa runtime.log.'}
if(!(Test-Path $ceResult) -or (Get-Item $ceResult).LastWriteTimeUtc -le $ceBefore){throw 'No hay informe actualizado.'}
$ceReport=Get-Content $ceResult -Raw | ConvertFrom-Json
if(!$ceReport.passed){throw 'Hay comprobaciones de dialogo fallidas.'}
if(Select-String -LiteralPath $ceLog -Pattern 'LogMaterial: Error:|missing usage flag|Failed to compile Material|Fatal error:|Failed to find object .StaticMesh|Cafe mesh missing:' -Quiet){throw 'Error de renderizado o runtime.'}
$ceReport
