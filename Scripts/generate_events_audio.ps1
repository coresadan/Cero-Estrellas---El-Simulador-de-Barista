$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Speech
$ceRoot=Split-Path -Parent $PSScriptRoot
$ceAudio=Join-Path $ceRoot 'SourceAudio/Boss'
$ceLines=Get-Content (Join-Path $ceAudio 'events07.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$ceVoice=[System.Speech.Synthesis.SpeechSynthesizer]::new()
try {
    $ceVoice.SelectVoice('Microsoft Helena Desktop'); $ceVoice.Rate=1
    for($ceIndex=0;$ceIndex -lt $ceLines.Count;$ceIndex++) {
        $ceVoice.SetOutputToWaveFile((Join-Path $ceAudio ('Boss_Event_'+($ceIndex+1)+'.wav')))
        $ceVoice.Speak($ceLines[$ceIndex]); $ceVoice.SetOutputToNull()
    }
} finally { $ceVoice.Dispose() }
Write-Output 'Tres llamadas de eventos grabadas.'
