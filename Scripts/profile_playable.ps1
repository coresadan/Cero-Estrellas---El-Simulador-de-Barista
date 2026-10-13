param([string]$EvidenceName='Entrega06Playable', [int]$TimeoutSeconds=900, [switch]$OnlyEvidence)
$ErrorActionPreference='Stop'
$ceRoot=Split-Path -Parent $PSScriptRoot
$ceRelease=[IO.Path]::GetFullPath((Join-Path $ceRoot 'Release\Windows'))
$ceEvidence=Join-Path $ceRoot ('Evidence\'+$EvidenceName)
[IO.Directory]::CreateDirectory($ceEvidence) | Out-Null
$ceDeadline=(Get-Date).AddSeconds($TimeoutSeconds)
$ceSamples=[Collections.Generic.List[object]]::new()
$ceSeen=$false
$ceOtherGameSeen=$false
while((Get-Date) -lt $ceDeadline) {
    $ceGames=@(Get-Process -Name CeroEstrellas -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($ceRelease,[StringComparison]::OrdinalIgnoreCase) })
    if($OnlyEvidence) {
        $ceSelectedIds=@(Get-CimInstance Win32_Process -Filter "Name = 'CeroEstrellas.exe'" | Where-Object { $_.CommandLine -and $_.CommandLine.Contains($EvidenceName) } | ForEach-Object {$_.ProcessId})
        if(@($ceGames | Where-Object { $_.Id -notin $ceSelectedIds }).Count -gt 0) { $ceOtherGameSeen=$true }
        $ceGames=@($ceGames | Where-Object { $_.Id -in $ceSelectedIds })
    }
    if($ceGames.Count -gt 0) {
        $ceSeen=$true
        $ceValues=(& nvidia-smi --query-gpu=memory.used,memory.total,utilization.gpu --format=csv,noheader,nounits).Split(',').Trim()
        $ceMemory=($ceGames | Measure-Object -Property WorkingSet64 -Sum).Sum/1MB
        $ceSamples.Add([pscustomobject]@{ utc=(Get-Date).ToUniversalTime().ToString('o'); total_desktop_gpu_mib=[int]$ceValues[0]; gpu_capacity_mib=[int]$ceValues[1]; gpu_utilization_percent=[int]$ceValues[2]; game_working_set_mib=[Math]::Round($ceMemory,1) })
    } elseif($ceSeen) { break }
    Start-Sleep -Seconds 2
}
if(!$ceSeen) { throw 'No project packaged game observed.' }
$ceSamples | Export-Csv -LiteralPath (Join-Path $ceEvidence 'resources.csv') -NoTypeInformation -Encoding UTF8
[pscustomobject]@{ samples=$ceSamples.Count; other_game_observed=$ceOtherGameSeen; only_named_evidence_processes=[bool]$OnlyEvidence; total_desktop_gpu_peak_mib=($ceSamples | Measure-Object total_desktop_gpu_mib -Maximum).Maximum; gpu_capacity_mib=$ceSamples[0].gpu_capacity_mib; game_working_set_peak_mib=($ceSamples | Measure-Object game_working_set_mib -Maximum).Maximum; note='GPU usage includes desktop and other apps; sampled every two seconds, not guaranteed to capture every transient peak.' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ceEvidence 'resources_summary.json') -Encoding UTF8
