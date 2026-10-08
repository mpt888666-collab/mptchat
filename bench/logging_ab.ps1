# A/B experiment: cost of synchronous server-side logging.
# Runs the same heartbeat benchmark twice against the same binary, changing only
# where the server's stdout goes: a log file vs NUL.
#
# Usage:
#   powershell -File bench/logging_ab.ps1 -ServerDir "C:\path\to\ChatServer\cmake-build-release\Release"
param(
    [string]$Python = "python",
    [string]$StressScript = (Join-Path $PSScriptRoot "chat_stress.py"),
    [Parameter(Mandatory = $true)][string]$ServerDir,
    [string]$LogDir = (Join-Path $env:TEMP "chatdemo-logs"),
    [int]$Duration = 30
)
$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$exe = Join-Path $ServerDir "ChatServer.exe"
if (-not (Test-Path $exe)) { throw "ChatServer.exe not found in $ServerDir" }
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

function Start-ChatServer([string]$StdoutTarget) {
    Get-Process -Name ChatServer -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "cmd.exe"
    $psi.Arguments = "/c `"`"$exe`" > $StdoutTarget 2>&1`""
    $psi.WorkingDirectory = $ServerDir
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    [System.Diagnostics.Process]::Start($psi) | Out-Null
    Start-Sleep -Seconds 3
}

function Measure-Heartbeat([string]$label) {
    $c = Get-Process -Name ChatServer -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $c) { Write-Host "ChatServer is not running for $label"; return }
    $cpu0 = $c.TotalProcessorTime.TotalSeconds
    $t0 = Get-Date
    $txt = & $Python $StressScript --mode heartbeat --duration $Duration 2>&1 | Out-String
    $wall = ((Get-Date) - $t0).TotalSeconds
    $c2 = Get-Process -Id $c.Id
    $cpu1 = $c2.TotalProcessorTime.TotalSeconds
    Write-Host "### $label"
    Write-Host ($txt.TrimEnd())
    Write-Host ("  chat_cpu={0:N2}s avg_cpu={1:N1}% (of one core)  wall={2:N2}s  rss={3:N1}MB" -f ($cpu1 - $cpu0), (($cpu1 - $cpu0) / $wall * 100), $wall, ($c2.WorkingSet64 / 1MB))
}

$logFile = Join-Path $LogDir "ChatServer.out.log"
Start-ChatServer "`"$logFile`""
Measure-Heartbeat "A: stdout -> log file"

Start-ChatServer "NUL"
Measure-Heartbeat "B: stdout -> NUL"

Start-ChatServer "`"$logFile`""
Write-Host "restored ChatServer (stdout -> $logFile)"