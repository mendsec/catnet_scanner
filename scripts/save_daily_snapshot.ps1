# Cleanup old workspace folder if present (runs on logon)
$oldPath = "C:\Users\fabio\Documents\trae_projects\windows_apps_tests"
if (Test-Path $oldPath) {
  try {
    Remove-Item -Path $oldPath -Recurse -Force -ErrorAction Stop
    Write-Host "Old workspace removed: $oldPath"
  } catch {
    try {
      $bak = "$oldPath.bak"
      if (Test-Path $bak) { Remove-Item -Path $bak -Recurse -Force -ErrorAction SilentlyContinue }
      Rename-Item -Path $oldPath -NewName (Split-Path $bak -Leaf) -ErrorAction Stop
      Write-Host "Old workspace renamed to: $bak"
    } catch {
      Write-Warning "Could not cleanup old workspace: $($_.Exception.Message)"
    }
  }
}
$projectRoot = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $projectRoot 'docs\chat_logs'
New-Item -ItemType Directory -Path $logDir -Force | Out-Null

$today = Get-Date
$dateStr = $today.ToString('yyyy-MM-dd')
$ts = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$dailyPath = Join-Path $logDir "$dateStr-daily-snapshot.md"

Push-Location $projectRoot
try {
  $branchStatus = (git status -s -b | Out-String)
  $remote = (git remote -v | Out-String)
  $lastCommitsToday = (git log --since "$dateStr 00:00" --pretty="%h %ad %an <%ae> %s" --date=iso | Out-String)
  if ([string]::IsNullOrWhiteSpace($lastCommitsToday)) {
    $lastCommitsToday = (git log -n 10 --pretty="%h %ad %an <%ae> %s" --date=iso | Out-String)
  }
  $userName = (git config user.name)
  $userEmail = (git config user.email)
  $credHelper = (git config --global credential.helper)
} finally {
  Pop-Location
}

$lines = @()
$lines += "# Daily Snapshot  $dateStr"
$lines += ""
$lines += "## Timestamp"
$lines += $ts
$lines += ""
$lines += "## Branch/Status"
$lines += '```'
$lines += $branchStatus.TrimEnd()
$lines += '```'
$lines += ""
$lines += "## Remoto"
$lines += '```'
$lines += $remote.TrimEnd()
$lines += '```'
$lines += ""
$lines += "## Commits (hoje ou últimos 10)"
$lines += '```'
$lines += $lastCommitsToday.TrimEnd()
$lines += '```'
$lines += ""
$lines += "## Git Config (resumo)"
$lines += "- user.name: $userName"
$lines += "- user.email: $userEmail"
$lines += "- credential.helper: $credHelper"
$lines += ""
$lines += "## Observações"
$lines += "- Logs de chat locais em docs/chat_logs (ignorados pelo Git)."
$lines += "- Use scripts/save_chat_note.ps1 para adicionar notas rápidas."

$md = ($lines -join "`n")
Set-Content -Path $dailyPath -Value $md -Encoding UTF8
Write-Host "Snapshot diário salvo em $dailyPath"

# Append to daily chat log
$chatLogPath = Join-Path $logDir ($dateStr + '-chat.md')
if (-not (Test-Path $chatLogPath)) {
  Set-Content -Path $chatLogPath -Value "# Chat Log" -Encoding UTF8
}
Add-Content -Path $chatLogPath -Value ("`n`n## Daily Snapshot (" + $ts + ")")
Add-Content -Path $chatLogPath -Value ("- Arquivo: " + $dailyPath)

