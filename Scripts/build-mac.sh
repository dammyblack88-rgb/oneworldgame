#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${PROJECT_PATH:-$ROOT/ParadiseHeat/ParadiseHeat.uproject}"
: "${UE_ROOT:?Set UE_ROOT to the installed Unreal Engine root}"
UAT="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
[[ -f "$UAT" ]] || { echo "Unreal Automation Tool missing: $UAT" >&2; exit 2; }
[[ -f "$PROJECT" ]] || { echo "Unreal project missing: $PROJECT" >&2; exit 2; }
[[ "$(uname -s)" == Darwin ]] || { echo 'Native macOS build host required' >&2; exit 2; }
OUTPUT="${OUTPUT_DIR:-$ROOT/artifacts/mac}"
mkdir -p "$OUTPUT" "$ROOT/artifacts/logs"
LOG="$ROOT/artifacts/logs/mac-$(date +%Y%m%d-%H%M%S).log"
bash "$UAT" BuildCookRun -project="$PROJECT" -noP4 -platform=Mac -architecture=arm64 -clientconfig=Shipping -build -cook -map=/Game/Maps/CityDistrict10x_Day -stage -pak -package -archive -archivedirectory="$OUTPUT" -utf8output -unattended -nocompileeditor -NoUBA -UbtArgs="-MaxParallelActions=2 -NoUBA" 2>&1 | tee "$LOG"
bash "$ROOT/Scripts/brand-mac-app.sh" "$OUTPUT/ParadiseHeat-Mac-Shipping.app"
echo "UAT completed. Inspect package under $OUTPUT; runtime verification still required. Log: $LOG"
