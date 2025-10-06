Param(
  [ValidateSet('MSVC','MinGW')]
  [string]$Compiler = 'MSVC',
  [switch]$UseGacUI,
  [string]$GacUIInclude,
  [string]$GacUILibs,
  [string]$GacGenPath,
  [string]$GacResource = 'src\\resources.xml'
)

$ErrorActionPreference = 'Stop'

New-Item -ItemType Directory -Force -Path "bin" | Out-Null

if ($Compiler -eq 'MSVC') {
  Write-Host "Compilando com MSVC (cl)"
  $files = @(Get-ChildItem -Recurse -Filter *.c | ForEach-Object { $_.FullName })
  $includes = ''
  $defines = '/D _CRT_SECURE_NO_WARNINGS /D _WINSOCK_DEPRECATED_NO_WARNINGS'
  $cxxflags = ''
  $linkopts = ''
  $out = 'bin\\net_tui_scanner.exe'
  $libs = 'Ws2_32.lib Iphlpapi.lib'

  if ($UseGacUI) {
    Write-Host "GacUI habilitado"
    # Preferir parâmetros; se não fornecidos, usar variáveis de ambiente
    $incPath = $GacUIInclude
    $libPath = $GacUILibs
    if (-not $incPath) { $incPath = $env:GACUI_INCLUDE }
    if (-not $libPath) { $libPath = $env:GACUI_LIBS }
    if (-not $incPath -or -not $libPath) {
      Write-Error "Forneça -GacUIInclude e -GacUILibs, ou defina GACUI_INCLUDE/GACUI_LIBS."
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
      Write-Host "Aviso: Vlpp.lib não encontrado em '$libPath'. Prosseguindo apenas com GacUI.lib." -ForegroundColor Yellow
    }
    $cxxflags = '/std:c++20 /EHsc /Zc:__cplusplus /permissive- /utf-8 /MD'
    $out = 'bin\\net_gui_scanner.exe'
    $linkopts = '/link /SUBSYSTEM:WINDOWS'

    # Integração opcional com GacGen.exe para gerar código de recursos
    if ($GacGenPath -and (Test-Path $GacGenPath) -and (Test-Path $GacResource)) {
      $genOutDir = Join-Path $PWD 'src/generated'
      New-Item -ItemType Directory -Force -Path $genOutDir | Out-Null
      Write-Host "Gerando código de recursos com GacGen..."
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
    Write-Host "Arquivos:"; Write-Host ($files -join ', ')
    Write-Host "Includes: $includes"
    Write-Host "Libs: $libs"
    Write-Host $clCmd
    Invoke-Expression $clCmd
  } else {
    # Tentar localizar VsDevCmd via vswhere
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
    if (-not (Test-Path $vswhere)) {
      Write-Error "MSVC 'cl' não encontrado e 'vswhere.exe' ausente. Abra o 'Developer Command Prompt for VS' e execute este script."
    }
    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Workload.VCTools -property installationPath
    if (-not $installPath) {
      Write-Error "Visual Studio Build Tools não encontrados pelo vswhere. Instale o workload de C++."
    }
    $vsDevCmd = Join-Path $installPath "Common7/Tools/VsDevCmd.bat"
    if (-not (Test-Path $vsDevCmd)) {
      Write-Error "VsDevCmd.bat não encontrado em '$vsDevCmd'. Verifique a instalação do Build Tools."
    }
    $fullCmd = "`"$vsDevCmd`" -arch=amd64 -host_arch=amd64 && $clCmd"
    Write-Host "Arquivos:"; Write-Host ($files -join ', ')
    Write-Host "Includes: $includes"
    Write-Host "Libs: $libs"
    Write-Host "Ativando ambiente VS e compilando..."
    Write-Host $fullCmd
    & cmd /c $fullCmd
  }
}
elseif ($Compiler -eq 'MinGW') {
  Write-Host "Compilando com MinGW (gcc)"
  if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Error "MinGW 'gcc' não encontrado. Instale MinGW e garanta que 'gcc' está no PATH."
  }
  $files = @(Get-ChildItem -Recurse -Filter *.c | ForEach-Object { $_.FullName })
  $out = 'bin\\net_tui_scanner.exe'
  $libs = '-lws2_32 -liphlpapi'
  $cmd = "gcc -O2 -Wall -D _CRT_SECURE_NO_WARNINGS -D _WINSOCK_DEPRECATED_NO_WARNINGS -o $out $($files -join ' ') $libs"
  Write-Host $cmd
  & cmd /c $cmd
}

Write-Host "Build finalizado. Executável em $out"