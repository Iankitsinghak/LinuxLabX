LinuxLabX Packaging Guide

Local build (CMake first):

mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel

macOS packaging:

1. Ensure Qt is installed and macdeployqt is on PATH.
2. Run macdeployqt on the app bundle:
   macdeployqt build/LinuxLabX.app
3. Create DMG:
   bash packaging/macos/create-dmg.sh build/LinuxLabX.app dist/LinuxLabX-mac.dmg

Windows packaging:

1. Ensure windeployqt and makensis are available.
2. Run:
   powershell -ExecutionPolicy Bypass -File packaging/windows/package.ps1 -BuildDir build -DistDir dist -Config Release -AppVersion 1.0.0

Linux packaging:

1. Build app in Release mode with CMake.
2. Run:
   bash packaging/linux/build-appimage.sh build dist

Signing placeholders:

- macOS signing identity:
  export APPLE_CODESIGN_IDENTITY="Developer ID Application: TEAM (TEAMID)"

- macOS notarization profile:
  export APPLE_NOTARY_PROFILE="notary-profile-name"

- Windows signing cert thumbprint:
  setx WINDOWS_SIGN_CERT_SHA1 "YOUR_CERT_THUMBPRINT"

Release artifacts expected:

- dist/LinuxLabX-mac.dmg
- dist/LinuxLabX-win.exe
- dist/LinuxLabX-linux.AppImage
