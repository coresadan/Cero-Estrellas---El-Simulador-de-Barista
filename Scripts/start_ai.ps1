$ErrorActionPreference='Stop'
$ceRoot=Split-Path -Parent $PSScriptRoot
$ceTools=Join-Path $ceRoot 'Tools/LocalAI'
$ceOllama=Join-Path $ceTools 'Ollama/ollama.exe'
$cePython=Join-Path $ceTools 'Python/Scripts/python.exe'
if (!(Test-Path $ceOllama) -or !(Test-Path $cePython)) { throw 'Ejecuta primero Preparar_IA.bat.' }
New-Item -ItemType Directory -Path (Join-Path $ceRoot 'Saved/LocalAI') -Force | Out-Null
$ceEmptyInput=Join-Path $ceRoot 'Saved/LocalAI/empty_input.txt'
if(!(Test-Path -LiteralPath $ceEmptyInput)) { [IO.File]::WriteAllText($ceEmptyInput,'') }
$env:OLLAMA_NO_CLOUD='1'
$env:OLLAMA_HOST='127.0.0.1:11434'
$env:OLLAMA_MODELS=Join-Path $ceTools 'Models/Ollama'
$env:OLLAMA_NUM_PARALLEL='1'
$env:OLLAMA_MAX_LOADED_MODELS='1'
$env:CUDA_VISIBLE_DEVICES='-1'
try { $null=Invoke-RestMethod http://127.0.0.1:11434/api/tags -TimeoutSec 2 } catch {
    Start-Process -FilePath $ceOllama -ArgumentList 'serve' -WorkingDirectory $ceTools -WindowStyle Hidden -RedirectStandardInput $ceEmptyInput -RedirectStandardOutput (Join-Path $ceRoot 'Saved/LocalAI/ollama.log') -RedirectStandardError (Join-Path $ceRoot 'Saved/LocalAI/ollama_error.log') | Out-Null
}
try { $null=Invoke-RestMethod http://127.0.0.1:11435/health -TimeoutSec 2 } catch {
    Start-Process -FilePath $cePython -ArgumentList ('"'+(Join-Path $PSScriptRoot 'local_voice.py')+'"') -WorkingDirectory $ceTools -WindowStyle Hidden -RedirectStandardInput $ceEmptyInput -RedirectStandardOutput (Join-Path $ceRoot 'Saved/LocalAI/voice.log') -RedirectStandardError (Join-Path $ceRoot 'Saved/LocalAI/voice_error.log') | Out-Null
}
Start-Process -FilePath $cePython -ArgumentList ('"'+(Join-Path $PSScriptRoot 'warm_dialogue.py')+'"') -WorkingDirectory $ceTools -WindowStyle Hidden -RedirectStandardInput $ceEmptyInput -RedirectStandardOutput (Join-Path $ceRoot 'Saved/LocalAI/warm.log') -RedirectStandardError (Join-Path $ceRoot 'Saved/LocalAI/warm_error.log') | Out-Null
Write-Host 'Preparando el turno...'
