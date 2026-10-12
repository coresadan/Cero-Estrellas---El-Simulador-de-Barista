$ceRoot=Split-Path -Parent $PSScriptRoot
$ceTools=[IO.Path]::GetFullPath((Join-Path $ceRoot 'Tools/LocalAI'))
$ceVoiceScript=[IO.Path]::GetFullPath((Join-Path $ceRoot 'Scripts/local_voice.py'))
$ceWarmScript=[IO.Path]::GetFullPath((Join-Path $ceRoot 'Scripts/warm_dialogue.py'))
Get-CimInstance Win32_Process | Where-Object {
    ($_.Name -eq 'python.exe' -and $_.CommandLine -and ($_.CommandLine.Replace('/','\').IndexOf($ceVoiceScript,[StringComparison]::OrdinalIgnoreCase) -ge 0 -or $_.CommandLine.Replace('/','\').IndexOf($ceWarmScript,[StringComparison]::OrdinalIgnoreCase) -ge 0)) -or
    ($_.ExecutablePath -and $_.ExecutablePath.StartsWith($ceTools+'\',[StringComparison]::OrdinalIgnoreCase) -and ($_.Name -like 'ollama*.exe' -or $_.Name -eq 'llama-server.exe'))
} | ForEach-Object { Stop-Process -Id $_.ProcessId -ErrorAction SilentlyContinue }
Write-Host 'Servicios de IA de este proyecto detenidos.'
