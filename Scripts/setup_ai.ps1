param([string]$EngineRoot='F:\Games\UE_5.8')
$ErrorActionPreference='Stop'
$ceRoot=Split-Path -Parent $PSScriptRoot
$ceTools=Join-Path $ceRoot 'Tools/LocalAI'
New-Item -ItemType Directory -Path $ceTools -Force | Out-Null
$ceOllama=Join-Path $ceTools 'Ollama/ollama.exe'
if (!(Test-Path $ceOllama)) {
    # Portable official distribution; inference is forced to CPU in the requests.
    $ceZip=Join-Path $ceTools 'ollama.zip'
    & curl.exe -fL --retry 3 --silent --show-error 'https://github.com/ollama/ollama/releases/download/v0.40.3/ollama-windows-amd64.zip' -o $ceZip
    if ($LASTEXITCODE -ne 0) { throw 'No se ha podido descargar Ollama.' }
    Expand-Archive -LiteralPath $ceZip -DestinationPath (Join-Path $ceTools 'Ollama') -Force
}
$cePython=Join-Path $ceTools 'Python/Scripts/python.exe'
if (!(Test-Path $cePython)) {
    & (Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe') -m venv (Join-Path $ceTools 'Python')
    if ($LASTEXITCODE -ne 0) { throw 'No se ha podido crear el entorno Python.' }
}
& $cePython -m pip install --disable-pip-version-check 'faster-whisper==1.2.1' 'piper-tts==1.4.2'
if ($LASTEXITCODE -ne 0) { throw 'No se han instalado Whisper/Piper.' }
& $cePython (Join-Path $PSScriptRoot 'local_voice.py') --download
if ($LASTEXITCODE -ne 0) { throw 'No se han descargado los modelos de voz.' }
& (Join-Path $PSScriptRoot 'start_ai.ps1')
$ceModelLine=Get-Content (Join-Path $ceRoot 'Config/DefaultGame.ini') | Where-Object { $_ -match '^Model=' } | Select-Object -First 1
$ceModel=$ceModelLine.Substring(6).Trim()
$cePull=Start-Process -FilePath $ceOllama -ArgumentList @('pull',$ceModel) -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $ceRoot 'Saved/LocalAI/model_setup.log') -RedirectStandardError (Join-Path $ceRoot 'Saved/LocalAI/model_setup_progress.log')
if ($cePull.ExitCode -ne 0) { throw 'No se ha descargado el modelo de conversacion.' }
& $cePython (Join-Path $PSScriptRoot 'warm_dialogue.py')
Write-Host 'IA local lista. Ollama + Whisper tiny + Piper, CPU.'
