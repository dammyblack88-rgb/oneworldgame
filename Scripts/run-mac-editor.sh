#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE_ROOT="${UE_ROOT:-/Users/Shared/UE_5.8}"
EDITOR="$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app"
PROJECT="$ROOT/ParadiseHeat/ParadiseHeat.uproject"
[[ -d "$EDITOR" && -f "$PROJECT" ]] || { echo 'Set UE_ROOT to your Unreal Engine 5.8 installation.' >&2; exit 2; }
open -n "$EDITOR" --args "$PROJECT" "$@"
