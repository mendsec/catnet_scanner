# Requires: GitHub CLI (gh) v2.0+ and Git
# Usage examples:
#   powershell -ExecutionPolicy Bypass -File sync_gh.ps1
#   powershell -ExecutionPolicy Bypass -File sync_gh.ps1 -CreateRelease -Tag v0.1.0 -ReleaseName "CatNet Scanner v0.1.0" -UploadBinaries

param(
    [switch] $CreateRelease,
    [string] $Tag = "",
    [string] $ReleaseName = "",
    [switch] $UploadBinaries
)

$ErrorActionPreference = "Stop"

function Ensure-GhInstalled {
    $ghPath = "C:\Program Files\GitHub CLI\gh.exe"
    if (-not (Test-Path $ghPath)) {
        Write-Host "GitHub CLI não encontrado. Instalando via winget..." -ForegroundColor Yellow
        winget install --id GitHub.cli -e --source winget | Out-Host
        if (-not (Test-Path $ghPath)) {
            throw "Falha ao instalar GitHub CLI via winget."
        }
    }
    return $ghPath
}

function Ensure-GhAuth($gh) {
    try {
        & $gh auth status -h github.com | Out-Null
    } catch {
        Write-Host "Autenticando no GitHub via navegador..." -ForegroundColor Yellow
        & $gh auth login -h github.com -s repo,workflow,read:org -w -p https | Out-Host
        & $gh auth setup-git | Out-Host
    }
}

function Ensure-Remote {
    $remoteUrl = (git remote get-url origin 2>$null)
    if (-not $remoteUrl) {
        throw "Remote 'origin' não está configurado. Configure com: git remote add origin https://github.com/<user>/<repo>.git"
    }
    Write-Host "Remote origin: $remoteUrl" -ForegroundColor Cyan
}

function Push-Main {
    Write-Host "Empurrando branch atual para origin..." -ForegroundColor Green
    git push -u origin $(git branch --show-current) | Out-Host
}

function Create-Release($gh) {
    if (-not $CreateRelease) { return }
    if (-not $Tag) { throw "Informe -Tag para criar release." }
    $name = $ReleaseName
    if (-not $name) { $name = $Tag }
    Write-Host "Criando release $Tag..." -ForegroundColor Green
    & $gh release create $Tag -t $name -n "Release $name" | Out-Host
    if ($UploadBinaries) {
        $binDir = Join-Path $PSScriptRoot "bin"
        if (Test-Path $binDir) {
            $files = Get-ChildItem $binDir -File
            foreach ($f in $files) {
                Write-Host "Anexando binário: $($f.FullName)" -ForegroundColor Cyan
                & $gh release upload $Tag $f.FullName --clobber | Out-Host
            }
        } else {
            Write-Host "Diretório de binários não encontrado: $binDir" -ForegroundColor Yellow
        }
    }
}

try {
    $gh = Ensure-GhInstalled
    Ensure-GhAuth $gh
    Ensure-Remote
    Push-Main
    Create-Release $gh
    Write-Host "Sincronização via GitHub CLI concluída." -ForegroundColor Green
} catch {
    Write-Error $_
    exit 1
}