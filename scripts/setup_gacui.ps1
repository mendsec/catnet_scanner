param(
  [string]$InstallDir
)

$ErrorActionPreference = 'Stop'

if (-not $InstallDir -or [string]::IsNullOrWhiteSpace($InstallDir)) {
  $InstallDir = Join-Path (Split-Path -Parent $PSScriptRoot) 'third_party\GacUI'
}

function Get-MSBuildPath {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
  if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found. Please install Visual Studio Build Tools." }
  $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
  if (-not $installPath) { throw "Visual Studio Build Tools not found by vswhere. Install the C++ workload." }
  $msbuild = Join-Path $installPath 'MSBuild/Current/Bin/MSBuild.exe'
  if (-not (Test-Path $msbuild)) { throw "MSBuild.exe not found at '$msbuild'." }
  return $msbuild
}

Write-Host "Setting up GacUI (Release repo) in $InstallDir"
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

Push-Location $InstallDir
try {
  if (-not (Test-Path (Join-Path $InstallDir '.git'))) {
    Write-Host "Cloning vczh-libraries/Release..."
    git clone https://github.com/vczh-libraries/Release.git .
  } else {
    Write-Host "Repo already present. Fetching updates..."
    git fetch --all
    git pull --ff-only
  }

  $importPath = Join-Path $InstallDir 'Import'
  if (-not (Test-Path $importPath)) { throw "Import folder not found at $importPath" }

  # Build tools (GacGen.exe, CppMerge.exe, etc.)
  Write-Host "Building GacUI tools (Release|Win32)..."
  $msbuild = Get-MSBuildPath
  $toolsSln = Join-Path $InstallDir 'Tools/Executables/Executables.sln'
  & $msbuild $toolsSln /m /p:Configuration=Release /p:Platform=Win32
  Write-Host "Copying executables to Tools..."
  & powershell -ExecutionPolicy Bypass -File (Join-Path $InstallDir 'Tools/CopyExecutables.ps1')

  $gacGen = Join-Path $InstallDir 'Tools/GacGen.exe'
  if (Test-Path $gacGen) { Write-Host "GacGen ready at $gacGen" } else { Write-Host "Warning: GacGen.exe not found in Tools." -ForegroundColor Yellow }

  # Build libraries (Release|x64)
  Write-Host "Building GacUI libraries (Release|x64)..."
  $gacuiProj = Join-Path $InstallDir 'Tutorial/Lib/GacUI/GacUI.vcxproj'
  & $msbuild $gacuiProj /m /p:Configuration=Release /p:Platform=x64
  $gacuiCompleteProj = Join-Path $InstallDir 'Tutorial/Lib/GacUIComplete/GacUIComplete.vcxproj'
  & $msbuild $gacuiCompleteProj /m /p:Configuration=Release /p:Platform=x64

  # Locate and copy libs to Lib\Release
  $libReleasePath = Join-Path $InstallDir 'Lib/Release'
  New-Item -ItemType Directory -Force -Path $libReleasePath | Out-Null
  $foundLibs = @()
  Get-ChildItem -Path (Join-Path $InstallDir 'Tutorial') -Recurse -Filter '*.lib' | ForEach-Object {
    $name = $_.Name.ToLower()
    if ($name -eq 'gacui.lib' -or $name -eq 'vlpp.lib') {
      Copy-Item -Path $_.FullName -Destination (Join-Path $libReleasePath $_.Name) -Force
      $foundLibs += $_.Name
    }
  }
  if ($foundLibs.Count -eq 0) { throw "No libs found after build. Expected GacUI.lib (and optionally Vlpp.lib)." }
  Write-Host "Copied libs: $($foundLibs -join ', ') to $libReleasePath"

  # Set environment variables for GacUI paths
  Write-Host "Setting environment variables for GacUI paths (User scope)"
  [Environment]::SetEnvironmentVariable('GACUI_INCLUDE', $importPath, 'User')
  [Environment]::SetEnvironmentVariable('GACUI_LIBS', $libReleasePath, 'User')
  Write-Host "GACUI_INCLUDE = $importPath"
  Write-Host "GACUI_LIBS    = $libReleasePath"
}
finally {
  Pop-Location
}

Write-Host "GacUI setup completed. You can now build GUI with -UseGacUI."