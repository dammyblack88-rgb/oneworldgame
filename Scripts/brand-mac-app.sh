#!/bin/bash
set -euo pipefail
APP="$1"
DEST="$(dirname "$APP")/Paradise Heat.app"
if [[ "$APP" != "$DEST" ]]; then
  [[ ! -e "$DEST" ]] || { echo "Destination already exists: $DEST" >&2; exit 1; }
  mv "$APP" "$DEST"
fi
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cp "$ROOT/ParadiseHeat/Build/Mac/Application.icns" "$DEST/Contents/Resources/Application.icns"
python3 - "$DEST/Contents/Info.plist" <<'PY'
import plistlib, sys
path = sys.argv[1]
with open(path, 'rb') as f:
    data = plistlib.load(f)
data.update(CFBundleName='Paradise Heat', CFBundleDisplayName='Paradise Heat', CFBundleIconFile='Application.icns')
data.pop('CFBundleIconName', None)
with open(path, 'wb') as f:
    plistlib.dump(data, f)
PY
codesign --force --sign - --preserve-metadata=entitlements "$DEST"
codesign --verify --deep --strict "$DEST"
echo "Branded production app: $DEST"
