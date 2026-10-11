$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Speech
$ceRoot = Split-Path -Parent $PSScriptRoot
$ceAudioDir = Join-Path $ceRoot 'SourceAudio\Boss'
New-Item -ItemType Directory -Path $ceAudioDir -Force | Out-Null
$ceLines = Get-Content (Join-Path $ceAudioDir 'lines.json') -Encoding UTF8 -Raw | ConvertFrom-Json
$ceVoice = New-Object System.Speech.Synthesis.SpeechSynthesizer
try {
    $ceVoice.SelectVoice('Microsoft Helena Desktop')
    $ceVoice.Rate = 1
    $ceVoice.Volume = 100
    for ($ceIndex = 0; $ceIndex -lt $ceLines.Count; $ceIndex++) {
        $ceVoice.SetOutputToWaveFile((Join-Path $ceAudioDir "Boss_$ceIndex.wav"))
        $ceVoice.Speak($ceLines[$ceIndex])
        $ceVoice.SetOutputToNull()
    }
} finally { $ceVoice.Dispose() }
Write-Output "Generated $($ceLines.Count) provisional Spanish boss recordings."


