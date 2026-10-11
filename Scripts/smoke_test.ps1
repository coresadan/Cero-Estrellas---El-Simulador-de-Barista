param([string]$EngineRoot = 'F:\Games\UE_5.8', [switch]$Packaged, [string]$EvidenceName = 'RealismoPlayable')
$ErrorActionPreference = 'Stop'
$ceRoot = Split-Path -Parent $PSScriptRoot
$ceEvidence = Join-Path $ceRoot ('Evidence\' + $EvidenceName)
[IO.Directory]::CreateDirectory($ceEvidence) | Out-Null
$ceReport = Join-Path $ceEvidence 'smoke_result.json'
$cePrevious = if (Test-Path -LiteralPath $ceReport) { (Get-Item -LiteralPath $ceReport).LastWriteTimeUtc } else { [datetime]::MinValue }
if ($Packaged) {
    $ceExe = Join-Path $ceRoot 'Release\Windows\CeroEstrellas.exe'
    $ceArguments = @('-windowed', '-ResX=1280', '-ResY=720', '-CeroSmokeTest', '-unattended', '-NoSplash', ('"-CeroEvidenceDir={0}"' -f $ceEvidence), ('"-abslog={0}\packaged_runtime.log"' -f $ceEvidence))
} else {
    $ceProject = Join-Path $ceRoot 'CeroEstrellas.uproject'
    $ceExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
    $ceArguments = @(('"{0}"' -f $ceProject), '-game', '-windowed', '-ResX=1280', '-ResY=720', '-CeroSmokeTest', '-unattended', '-NoSplash', ('"-CeroEvidenceDir={0}"' -f $ceEvidence), ('"-abslog={0}\editor_runtime.log"' -f $ceEvidence))
}
$ceProcess = Start-Process -FilePath $ceExe -ArgumentList $ceArguments -WorkingDirectory $ceRoot -WindowStyle Hidden -Wait -PassThru
if ($ceProcess.ExitCode -ne 0) { throw "Runtime smoke test exited with $($ceProcess.ExitCode)" }
if (-not (Test-Path -LiteralPath $ceReport)) { throw 'No smoke report produced.' }
if ((Get-Item -LiteralPath $ceReport).LastWriteTimeUtc -le $cePrevious) { throw 'Smoke report was not refreshed.' }
$ceResult = Get-Content -LiteralPath $ceReport -Raw | ConvertFrom-Json
if (-not $ceResult.passed) { throw 'Gameplay smoke checks failed. Inspect report and runtime log.' }
$ceRuntimeLog = Join-Path $ceEvidence ('{0}_runtime.log' -f $(if ($Packaged) { 'packaged' } else { 'editor' }))
if (Select-String -LiteralPath $ceRuntimeLog -Pattern 'LogMaterial: Error:|missing usage flag|Failed to compile Material|Fatal error:' -Quiet) {
    throw 'Runtime rendering or fatal errors detected. Inspect the runtime log.'
}
$ceResult

