#!/usr/bin/env bash
# Build the motes WebAssembly module with Emscripten. Requires `emcc` on PATH.
# Output: web/build/motes.mjs (+ motes.wasm), an ES module exporting a `createMotes()` factory.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"
mkdir -p "$here/build"

em++ -std=c++17 -O3 -I "$root/include" "$here/binding.cpp" \
  -lembind \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sEXPORT_NAME=createMotes \
  -sEXPORTED_FUNCTIONS=_malloc,_free \
  -sEXPORTED_RUNTIME_METHODS=HEAPU8 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sENVIRONMENT=web,node \
  -o "$here/build/motes.mjs"

echo "built: $here/build/motes.mjs (+ motes.wasm)"
ls -lh "$here/build/"
