#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
cmake --build build -j
exec ./build/networking_chat
