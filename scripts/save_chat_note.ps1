param(
  [Parameter(Mandatory=$true)][string]$Text,
  [string]$LogName
)

$projectRoot = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $projectRoot 'docs\chat_logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

if (-not $LogName -or [string]::IsNullOrWhiteSpace($LogName)) {
  $LogName = (Get-Date -Format 'yyyy-MM-dd') + '-chat.md'
}

$logPath = Join-Path $logDir $LogName
$timestamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$header = "`n`n### $timestamp"

if (-not (Test-Path $logPath)) {
  Set-Content -Path $logPath -Value "# Chat Log" -Encoding UTF8
}
Add-Content -Path $logPath -Value $header
Add-Content -Path $logPath -Value $Text
Write-Host "Nota salva em $logPath"
