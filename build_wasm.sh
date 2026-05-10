#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EMSDK_ENV="$SCRIPT_DIR/tools/emsdk/emsdk_env.sh"

if [[ ! -f "$EMSDK_ENV" ]]; then
    echo "error: emsdk not found at tools/emsdk/"
    echo "  cd tools && git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    exit 1
fi

# shellcheck source=/dev/null
source "$EMSDK_ENV"

mkdir -p "$SCRIPT_DIR/CBKBindings/bin/wasm"

# Parallelism: default to all cores; override with JOBS=N. `make V=1` for verbose.
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

make -C "$SCRIPT_DIR/CBKBindings" -f Makefile.emscripten -j"$JOBS" "$@"
