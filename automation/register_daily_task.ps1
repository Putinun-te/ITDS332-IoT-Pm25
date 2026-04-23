param(
    [string]$TaskName = "pm2.5 forcast",
    [string]$StartTime = "00:10"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$runnerPath = (Resolve-Path (Join-Path $PSScriptRoot "run_forecast_notebook.ps1")).Path
$taskCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$runnerPath`""

schtasks /Create /F /SC DAILY /ST $StartTime /TN $TaskName /TR $taskCommand | Out-Null

Write-Host "Scheduled task created."
Write-Host "Task name: $TaskName"
Write-Host "Start time: $StartTime"
Write-Host "Run once now: schtasks /Run /TN `"$TaskName`""
Write-Host "Check result: schtasks /Query /TN `"$TaskName`" /V /FO LIST"
