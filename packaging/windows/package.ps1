param(
    [string]$BuildDir = "build",
    [string]$DistDir = "dist",
    [string]$Config = "Release",
    [string]$AppVersion = "1.0.0"
)

$ErrorActionPreference = "Stop"

$exePath = Join-Path $BuildDir "LinuxLabX.exe"
if (!(Test-Path $exePath)) {
    $exePath = Join-Path $BuildDir "$Config/LinuxLabX.exe"
}
if (!(Test-Path $exePath)) {
    throw "LinuxLabX.exe not found in $BuildDir"
}

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
$deployDir = Join-Path $BuildDir "deploy"
New-Item -ItemType Directory -Force -Path $deployDir | Out-Null
Copy-Item $exePath $deployDir -Force

$windeployqt = Get-Command windeployqt -ErrorAction SilentlyContinue
if (-not $windeployqt) {
    throw "windeployqt not found on PATH"
}

& $windeployqt.Source --release --compiler-runtime (Join-Path $deployDir "LinuxLabX.exe")

if ($env:WINDOWS_SIGN_CERT_SHA1) {
    Write-Host "Code signing executable"
    signtool sign /sha1 $env:WINDOWS_SIGN_CERT_SHA1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 (Join-Path $deployDir "LinuxLabX.exe")
}

$nsis = Get-Command makensis -ErrorAction SilentlyContinue
if (-not $nsis) {
    throw "makensis not found on PATH"
}

& $nsis.Source `
    "/DAPP_NAME=LinuxLabX" `
    "/DAPP_VERSION=$AppVersion" `
    "/DBUILD_DIR=$BuildDir" `
    "/DDIST_DIR=$DistDir" `
  "/DICON_PATH=assets\\icon.ico" `
    "packaging/windows/installer.nsi"

$setupInstaller = Join-Path $DistDir "LinuxLabX-Setup.exe"
$releaseInstaller = Join-Path $DistDir "LinuxLabX-win.exe"
if (Test-Path $setupInstaller) {
    Copy-Item $setupInstaller $releaseInstaller -Force
}

if ($env:WINDOWS_SIGN_CERT_SHA1 -and (Test-Path $setupInstaller)) {
    Write-Host "Code signing setup installer"
    signtool sign /sha1 $env:WINDOWS_SIGN_CERT_SHA1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 $setupInstaller
}

if ($env:WINDOWS_SIGN_CERT_SHA1 -and (Test-Path $releaseInstaller)) {
    Write-Host "Code signing release installer"
    signtool sign /sha1 $env:WINDOWS_SIGN_CERT_SHA1 /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 $releaseInstaller
}
