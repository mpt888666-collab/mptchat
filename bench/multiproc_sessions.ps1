# Multi-process concurrent session benchmark: N worker processes x M connections each.
# Splitting the load generator into processes avoids the Python GIL thundering herd
# that inflates p95 when hundreds of threads wake up at the same time.
#
# Usage:
#   powershell -File bench/multiproc_sessions.ps1 -Total 500 -PerProc 100 -Duration 60
param(
    [int]$Total = 500,
    [int]$PerProc = 100,
    [int]$Duration = 60,
    [int]$HbInterval = 5,
    [string]$Python = "python",
    [string]$StressScript = (Join-Path $PSScriptRoot "chat_stress.py"),
    [string]$RedisCli = "redis-cli"
)
$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$chat = Get-Process -Name ChatServer -ErrorAction SilentlyContinue | Select-Object -First 1
$cpu0 = $chat.TotalProcessorTime.TotalSeconds
$t0 = Get-Date

$groups = [math]::Floor($Total / $PerProc)
$jobs = @()
for ($k = 0; $k -lt $groups; $k++) {
    $start = $k * $PerProc + 1
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $Python
    $psi.Arguments = (($StressScript, "--mode", "sessions", "--clients", $PerProc, "--distinct-users", "$Total",
                       "--user-start", $start, "--duration", $Duration, "--hb-interval", $HbInterval) |
                      ForEach-Object { '"' + $_ + '"' }) -join ' '
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $p = [System.Diagnostics.Process]::Start($psi)
    $jobs += [pscustomobject]@{ Proc = $p; Out = $p.StandardOutput.ReadToEndAsync(); Err = $p.StandardError.ReadToEndAsync(); Index = $k }
}
Write-Host "launched $($jobs.Count) worker processes x $PerProc connections = $($jobs.Count * $PerProc)"
foreach ($j in $jobs) { $j.Proc.WaitForExit() }
$wall = ((Get-Date) - $t0).TotalSeconds
$cpu1 = (Get-Process -Id $chat.Id).TotalProcessorTime.TotalSeconds

$conn = 0; $alive = 0; $hb = 0; $drops = 0
$p50s = @(); $p95s = @(); $p99s = @(); $maxs = @()
foreach ($j in ($jobs | Sort-Object Index)) {
    $txt = $j.Out.Result
    Write-Host "--- worker $($j.Index) (user-start $($j.Index * $PerProc + 1)) ---"
    Write-Host $txt.TrimEnd()
    $e = $j.Err.Result.Trim()
    if ($e) { Write-Host "stderr: $e" }
    if ($txt -match 'handshake_ok=(\d+)') { $conn += [int]$Matches[1] }
    if ($txt -match 'alive_at_end=(\d+)') { $alive += [int]$Matches[1] }
    if ($txt -match 'hb_total=(\d+)')     { $hb += [int]$Matches[1] }
    if ($txt -match 'hb_err=(\d+)')       { $drops += [int]$Matches[1] }
    if ($txt -match 'rtt ms p50=([\d.]+) p95=([\d.]+) p99=([\d.]+) max=([\d.]+)') {
        $p50s += [double]$Matches[1]; $p95s += [double]$Matches[2]
        $p99s += [double]$Matches[3]; $maxs += [double]$Matches[4]
    }
}
function Avg($a) { if ($a.Count -eq 0) { 0 } else { ($a | Measure-Object -Average).Average } }
Write-Host ""
Write-Host "=== AGGREGATE (processes=$($jobs.Count) x $PerProc) ==="
Write-Host ("concurrent_connections={0} alive_at_end={1} heartbeats={2} hb_errors={3}" -f $conn, $alive, $hb, $drops)
Write-Host ("wall={0:N2}s chat_cpu={1:N2}s avg_cpu={2:N1}% chat_rss={3:N1}MB" -f $wall, ($cpu1 - $cpu0), (($cpu1 - $cpu0) / $wall * 100), ((Get-Process -Id $chat.Id).WorkingSet64 / 1MB))
Write-Host ("per-worker rtt (avg over workers) p50={0:N2}ms p95={1:N2}ms p99={2:N2}ms max={3:N2}ms" -f (Avg $p50s), (Avg $p95s), (Avg $p99s), (Avg $maxs))
$redis = & $RedisCli --scan --pattern 'uip_*'
Write-Host ("redis uip_keys={0}" -f (($redis | Measure-Object).Count))