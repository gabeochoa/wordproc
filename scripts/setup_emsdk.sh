#!/usr/bin/env bash
# setup_emsdk.sh — Install and activate the Emscripten SDK for WebAssembly builds.
#
# Usage:
#   ./scripts/setup_emsdk.sh          # install to ./emsdk/
#   ./scripts/setup_emsdk.sh /opt/emsdk  # install to custom path
#
# After running, source the environment before building:
#   source <install-dir>/emsdk_env.sh
#   make web

set -euo pipefail

INSTALL_DIR="${1:-$(pwd)/emsdk}"

if [ -x "$INSTALL_DIR/emsdk" ]; then
    echo "emsdk already installed at $INSTALL_DIR"
    echo "Updating to latest..."
    cd "$INSTALL_DIR"
    git pull
else
    echo "Cloning emsdk to $INSTALL_DIR..."
    git clone https://github.com/emscripten-core/emsdk.git "$INSTALL_DIR"
    cd "$INSTALL_DIR"
fi

echo "Installing latest Emscripten toolchain..."
./emsdk install latest

echo "Activating latest Emscripten toolchain..."
./emsdk activate latest

echo ""
echo "Done. To use em++ in your current shell, run:"
echo ""
echo "  source $INSTALL_DIR/emsdk_env.sh"
echo ""
