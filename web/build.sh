#!/usr/bin/env bash
# Build the motes WebAssembly module with Emscripten. Requires `em++` on PATH.
# Output: web/motes.mjs (+ motes.wasm), an ES module exporting a `createMotes()` factory.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/.." && pwd)"

# Output lands next to the wrapper (index.mjs) so the package publishes as one flat directory.
# -sDYNAMIC_EXECUTION=0: without it, embind generates its call invokers with `new Function(...)`,
# which a strict Content-Security-Policy (no 'unsafe-eval') blocks in the browser. The flag makes
# embind use a CSP-safe codegen path, so the module runs under a script-src that allows only the
# narrow 'wasm-unsafe-eval' (as digitalbluebird.com does) and not the broad 'unsafe-eval'.
em++ -std=c++17 -O3 -I "$root/include" "$here/binding.cpp" \
  -lembind \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sEXPORT_NAME=createMotes \
  -sEXPORTED_FUNCTIONS=_malloc,_free \
  -sEXPORTED_RUNTIME_METHODS=HEAPU8 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sDYNAMIC_EXECUTION=0 \
  -sENVIRONMENT=web,node \
  -o "$here/motes.mjs"

echo "built:"
ls -lh "$here/motes.mjs" "$here/motes.wasm"
