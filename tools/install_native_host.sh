#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <chrome_or_edge_extension_id> [chrome|edge|both]" >&2
  exit 1
fi

EXTENSION_ID="$1"
BROWSER="${2:-both}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST_PATH="$ROOT_DIR/chem_native_host"
TEMPLATE="$ROOT_DIR/native-host/com.compiler_project.my_chem.json.in"

if [[ ! -x "$HOST_PATH" ]]; then
  echo "Native host executable not found: $HOST_PATH" >&2
  echo "Run: cmake --build $ROOT_DIR --target chem_native_host" >&2
  exit 1
fi

install_one() {
  local dir="$1"
  mkdir -p "$dir"
  sed \
    -e "s#@HOST_PATH@#$HOST_PATH#g" \
    -e "s#@EXTENSION_ID@#$EXTENSION_ID#g" \
    "$TEMPLATE" > "$dir/com.compiler_project.my_chem.json"
  echo "Installed native host manifest: $dir/com.compiler_project.my_chem.json"
}

case "$BROWSER" in
  chrome)
    install_one "$HOME/.config/google-chrome/NativeMessagingHosts"
    ;;
  edge)
    install_one "$HOME/.config/microsoft-edge/NativeMessagingHosts"
    ;;
  both)
    install_one "$HOME/.config/google-chrome/NativeMessagingHosts"
    install_one "$HOME/.config/microsoft-edge/NativeMessagingHosts"
    ;;
  *)
    echo "Unknown browser '$BROWSER'; expected chrome, edge, or both" >&2
    exit 1
    ;;
esac
