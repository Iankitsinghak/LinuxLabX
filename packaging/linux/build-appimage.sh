#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
DIST_DIR="${2:-dist}"
APP_NAME="LinuxLabX"
APPDIR="$BUILD_DIR/AppDir"
DESKTOP_FILE="$APPDIR/usr/share/applications/${APP_NAME}.desktop"
ICON_DIR="$APPDIR/usr/share/icons/hicolor/256x256/apps"
ICON_FILE="$ICON_DIR/${APP_NAME}.png"
OUTPUT_APPIMAGE="$DIST_DIR/LinuxLabX-x86_64.AppImage"

mkdir -p "$DIST_DIR" "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" "$ICON_DIR"

if [[ -f "$BUILD_DIR/LinuxLabX" ]]; then
  cp "$BUILD_DIR/LinuxLabX" "$APPDIR/usr/bin/LinuxLabX"
elif [[ -f "$BUILD_DIR/Release/LinuxLabX" ]]; then
  cp "$BUILD_DIR/Release/LinuxLabX" "$APPDIR/usr/bin/LinuxLabX"
else
  echo "LinuxLabX binary not found in $BUILD_DIR"
  exit 1
fi

cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=LinuxLabX
GenericName=Linux Learning Terminal
Comment=Beginner-friendly virtual Linux terminal simulator
Exec=LinuxLabX
Icon=LinuxLabX
Categories=Education;Utility;
Terminal=false
StartupNotify=true
EOF

if [[ -f assets/icon.png ]]; then
  cp assets/icon.png "$ICON_FILE"
elif [[ -f /usr/share/pixmaps/debian-logo.png ]]; then
  cp /usr/share/pixmaps/debian-logo.png "$ICON_FILE"
else
  # Generate a tiny valid PNG placeholder icon.
  echo "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9s6R5e8AAAAASUVORK5CYII=" | base64 -d > "$ICON_FILE"
fi

if [[ ! -x linuxdeployqt.AppImage ]]; then
  wget -q https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage -O linuxdeployqt.AppImage
  chmod +x linuxdeployqt.AppImage
fi

./linuxdeployqt.AppImage "$DESKTOP_FILE" -bundle-non-qt-libs -appimage

APPIMAGE_FILE=$(ls ./*.AppImage | head -n 1)
mv "$APPIMAGE_FILE" "$OUTPUT_APPIMAGE"
chmod +x "$OUTPUT_APPIMAGE"

echo "Created $OUTPUT_APPIMAGE"
