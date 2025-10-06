Param(
  [ValidateSet('MSVC','MinGW')]
  [string]$Compiler = 'MSVC',
  [switch]$UseGacUI,
  [string]$GacUIInclude,
  [string]$GacUILibs,
  [string]$GacGenPath,
  [string]$GacResource = 'src\\resources.xml',
  [ValidateSet('build','chat-note','daily-snapshot','chat-clean')]
  [string]$Task = 'build',
  [string]$Text,
  [string]$LogName
)

$ErrorActionPreference = 'Stop'

# Project utility tasks (chat logs)
if ($Task -eq 'chat-note') {
  if (-not $Text) { Write-Error "Provide -Text for the chat note." }
  & (Join-Path $PSScriptRoot 'scripts/save_chat_note.ps1') -Text $Text -LogName $LogName
  return
}
elseif ($Task -eq 'daily-snapshot') {
  & (Join-Path $PSScriptRoot 'scripts/save_daily_snapshot.ps1')
  return
}
elseif ($Task -eq 'chat-clean') {
  $projectRoot = $PSScriptRoot
  $logDir = Join-Path $projectRoot 'docs\chat_logs'
  if (-not (Test-Path $logDir)) { Write-Host "Directory does not exist: $logDir"; return }
  Write-Host "Cleaning unnecessary files in $logDir ..."
  $kept = @()
  $removed = @()
  $attachmentExts = @('.png','.jpg','.jpeg','.gif','.svg','.pdf','.txt')
  Get-ChildItem -Path $logDir -Force -Recurse | ForEach-Object {
    $item = $_
    if ($item.PSIsContainer) {
      # remove common irrelevant directories
      if ($item.Name -match '^(vsdbg|node_modules)$') {
        try { Remove-Item -Path $item.FullName -Recurse -Force -ErrorAction Stop; $removed += $item.FullName } catch {}
      }
      return
    }
    $name = $item.Name
    $keep = $false
    if ($name -match '(^|-)chat\.md$') { $keep = $true }
    elseif ($name -match 'daily-snapshot\.md$') { $keep = $true }
    elseif ($name -match '^README\.md$') { $keep = $true }
    else {
      $ext = ([System.IO.Path]::GetExtension($name)).ToLower()
      if ($attachmentExts -contains $ext) { $keep = $true }
    }
    # Remove .json, .js, .html, and other non-md artifacts
    if ($keep) { $kept += $item.FullName }
    else {
      try { Remove-Item -Path $item.FullName -Force -ErrorAction Stop; $removed += $item.FullName } catch {}
    }
  }
  Write-Host "Kept: $($kept.Count) | Removed: $($removed.Count)"
  if ($removed.Count -gt 0) { Write-Host "Removed:"; $removed | ForEach-Object { Write-Host $_ } }
  return
}

New-Item -ItemType Directory -Force -Path "bin" | Out-Null

if ($Task -ne 'build') { return }

if ($Compiler -eq 'MSVC') {
  Write-Host "Building with MSVC (cl)"
  $files = @(Get-ChildItem -Recurse -Filter *.c | ForEach-Object { $_.FullName })
  $includes = ''
  $defines = '/D _CRT_SECURE_NO_WARNINGS /D _WINSOCK_DEPRECATED_NO_WARNINGS'
  $cxxflags = ''
  $linkopts = ''
  $out = 'bin\\catnet_tui_scanner.exe'
  $libs = 'Ws2_32.lib Iphlpapi.lib'

  if ($UseGacUI) {
    Write-Host "GacUI enabled"
    # Preferir parâmetros; se não fornecidos, usar variáveis de ambiente
    $incPath = $GacUIInclude
    $libPath = $GacUILibs
    if (-not $incPath) { $incPath = $env:GACUI_INCLUDE }
    if (-not $libPath) { $libPath = $env:GACUI_LIBS }
    if (-not $incPath -or -not $libPath) {
      Write-Error "Provide -GacUIInclude and -GacUILibs, or set GACUI_INCLUDE/GACUI_LIBS."
    }
    # Troca o arquivo de entrada principal para C++ GacUI
    $files = $files | Where-Object { $_ -notmatch "\\src\\main.c$" }
    $files += Join-Path $PWD "src/main_gacui.cpp"
    $includes += " /I `"$incPath`""
    $libs += " `"$libPath\\GacUI.lib`" Comctl32.lib UxTheme.lib User32.lib Gdi32.lib Comdlg32.lib Ole32.lib Dwmapi.lib Shlwapi.lib Shell32.lib Kernel32.lib Advapi32.lib"
    $vlpp = Join-Path $libPath 'Vlpp.lib'
    if (Test-Path $vlpp) {
      $libs += " `"$vlpp`""
    } else {
      Write-Host "Warning: Vlpp.lib not found in '$libPath'. Proceeding with GacUI.lib only." -ForegroundColor Yellow
    }
    $cxxflags = '/std:c++20 /EHsc /Zc:__cplusplus /permissive- /utf-8 /MD'
    $out = 'bin\\catnet_gui_scanner.exe'
    $linkopts = '/link /SUBSYSTEM:WINDOWS'

    # Integração opcional com GacGen.exe para gerar código de recursos
    if ($GacGenPath -and (Test-Path $GacGenPath) -and (Test-Path $GacResource)) {
      $genOutDir = Join-Path $PWD 'src/generated'
      New-Item -ItemType Directory -Force -Path $genOutDir | Out-Null
      Write-Host "Generating resource code with GacGen..."
      $genCmd = "`"$GacGenPath`" -res `"$GacResource`" -name AppRes -cpp `"$genOutDir`""
      Write-Host $genCmd
      & cmd /c $genCmd
      $genFiles = @(Get-ChildItem -Recurse -Path $genOutDir -Filter *.cpp | ForEach-Object { $_.FullName })
      if ($genFiles.Count -gt 0) {
        $files += $genFiles
      }
    }
  }

  $clExists = (Get-Command cl -ErrorAction SilentlyContinue) -ne $null
  $clCmd = "cl /nologo /W3 /O2 $defines $cxxflags /Fe$out $($files -join ' ') $includes $libs $linkopts"

  if ($clExists) {
    Write-Host "Files:"; Write-Host ($files -join ', ')
    Write-Host "Includes: $includes"
    Write-Host "Libs: $libs"
    Write-Host $clCmd
    Invoke-Expression $clCmd
  } else {
    # Tentar localizar VsDevCmd via vswhere
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
    if (-not (Test-Path $vswhere)) {
      Write-Error "MSVC 'cl' not found and 'vswhere.exe' is missing. Open the 'Developer Command Prompt for VS' and run this script."
    }
    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Workload.VCTools -property installationPath
    if (-not $installPath) {
      Write-Error "Visual Studio Build Tools not found by vswhere. Please install the C++ workload."
    }
    $vsDevCmd = Join-Path $installPath "Common7/Tools/VsDevCmd.bat"
    if (-not (Test-Path $vsDevCmd)) {
      Write-Error "VsDevCmd.bat not found at '$vsDevCmd'. Check your Build Tools installation."
    }
    $fullCmd = "`"$vsDevCmd`" -arch=amd64 -host_arch=amd64 && $clCmd"
    Write-Host "Files:"; Write-Host ($files -join ', ')
    Write-Host "Includes: $includes"
    Write-Host "Libs: $libs"
    Write-Host "Activating VS environment and building..."
    Write-Host $fullCmd
    & cmd /c $fullCmd
  }
}
elseif ($Compiler -eq 'MinGW') {
  Write-Host "Building with MinGW (gcc)"
  if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Error "MinGW 'gcc' not found. Install MinGW and ensure 'gcc' is on PATH."
  }
  $files = @(Get-ChildItem -Recurse -Filter *.c | ForEach-Object { $_.FullName })
  $out = 'bin\\catnet_tui_scanner.exe'
  $libs = '-lws2_32 -liphlpapi'
  $cmd = "gcc -O2 -Wall -D _CRT_SECURE_NO_WARNINGS -D _WINSOCK_DEPRECATED_NO_WARNINGS -o $out $($files -join ' ') $libs"
  Write-Host $cmd
  & cmd /c $cmd
}

Write-Host "Build finished. Executable at $out"