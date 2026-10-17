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
$ceObservedProcesses=@{}
while((Get-Date) -lt $ceDeadline) {
    # Classify from one snapshot: the launcher can exit between Get-Process
    # and CIM queries, which otherwise falsely reports an unrelated game.
    $ceRecords=@(Get-CimInstance Win32_Process -Filter "Name = 'CeroEstrellas.exe'" | Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith($ceRelease,[StringComparison]::OrdinalIgnoreCase) })
    if($OnlyEvidence) {
        foreach($ceRecord in $ceRecords) {
            $ceMatches=$ceRecord.CommandLine -and $ceRecord.CommandLine.Contains($EvidenceName)
            $ceObservedProcesses[[string]$ceRecord.ProcessId]=[pscustomobject]@{ pid=$ceRecord.ProcessId; executable=$ceRecord.ExecutablePath; matches_evidence=[bool]$ceMatches }
            if(!$ceMatches) { $ceOtherGameSeen=$true }
        }
        $ceRecords=@($ceRecords | Where-Object { $_.CommandLine -and $_.CommandLine.Contains($EvidenceName) })
    }
    $ceGames=@(foreach($ceRecord in $ceRecords) { Get-Process -Id $ceRecord.ProcessId -ErrorAction SilentlyContinue })
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
$ceObservedProcesses.Values | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $ceEvidence 'observed_processes.json') -Encoding UTF8
[pscustomobject]@{ samples=$ceSamples.Count; other_game_observed=$ceOtherGameSeen; only_named_evidence_processes=[bool]$OnlyEvidence; total_desktop_gpu_peak_mib=($ceSamples | Measure-Object total_desktop_gpu_mib -Maximum).Maximum; gpu_capacity_mib=$ceSamples[0].gpu_capacity_mib; game_working_set_peak_mib=($ceSamples | Measure-Object game_working_set_mib -Maximum).Maximum; note='GPU usage includes desktop and other apps; sampled every two seconds, not guaranteed to capture every transient peak.' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ceEvidence 'resources_summary.json') -Encoding UTF8
