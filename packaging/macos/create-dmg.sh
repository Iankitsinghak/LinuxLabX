#!/usr/bin/env bash
set -euo pipefail

APP_BUNDLE_PATH="${1:-build/LinuxLabX.app}"
OUTPUT_DMG="${2:-dist/LinuxLabX-mac.dmg}"
APP_NAME="LinuxLabX"
VOLUME_NAME="LinuxLabX Installer"
BACKGROUND_IMAGE="${DMG_BACKGROUND_IMAGE:-assets/dmg-background.png}"

mkdir -p "$(dirname "$OUTPUT_DMG")"

if [[ ! -d "$APP_BUNDLE_PATH" ]]; then
  echo "App bundle not found: $APP_BUNDLE_PATH"
  exit 1
fi

if [[ -n "${APPLE_CODESIGN_IDENTITY:-}" ]]; then
  echo "Signing app bundle with Developer ID"
  codesign --force --options runtime --deep --sign "$APPLE_CODESIGN_IDENTITY" "$APP_BUNDLE_PATH"
fi

STAGE_DIR="$(mktemp -d)"
cp -R "$APP_BUNDLE_PATH" "$STAGE_DIR/"
ln -s /Applications "$STAGE_DIR/Applications"

if command -v create-dmg >/dev/null 2>&1; then
  DMG_ARGS=(
    --volname "$VOLUME_NAME"
    --window-pos 120 120
    --window-size 920 560
    --icon-size 128
    --icon "$APP_NAME.app" 250 300
    --hide-extension "$APP_NAME.app"
    --app-drop-link 670 300
  )

  if [[ -f "$BACKGROUND_IMAGE" ]]; then
    DMG_ARGS+=(--background "$BACKGROUND_IMAGE")
  fi

  create-dmg \
    "${DMG_ARGS[@]}" \
    "$OUTPUT_DMG" \
    "$STAGE_DIR"
else
  hdiutil create \
    -volname "$VOLUME_NAME" \
    -srcfolder "$STAGE_DIR" \
    -ov \
    -format UDZO \
    "$OUTPUT_DMG"
fi

rm -rf "$STAGE_DIR"

if [[ -n "${APPLE_CODESIGN_IDENTITY:-}" ]]; then
  echo "Signing DMG"
  codesign --force --sign "$APPLE_CODESIGN_IDENTITY" "$OUTPUT_DMG"
fi

if [[ -n "${APPLE_NOTARY_PROFILE:-}" ]]; then
  echo "Submitting DMG for notarization"
  xcrun notarytool submit "$OUTPUT_DMG" --keychain-profile "$APPLE_NOTARY_PROFILE" --wait
  xcrun stapler staple "$OUTPUT_DMG"
fi

echo "Created $OUTPUT_DMG"
