#!/usr/bin/env bash
# ===========================================================================
# Project Astra Cosmos — Phase 0 build pipeline (Linux -> Windows x64)
#
#   [1/4] shaders  -> SPIR-V          (glslangValidator, into build/shaders/)
#   [2/4] cmake configure (MinGW-w64 cross toolchain, Ninja)
#   [3/4] build     (Ninja)
#   [4/4] artifacts (size + sha256 of astra.exe and every .spv)
#
# Any failure at any step aborts with a non-zero exit code (set -euo pipefail).
# ===========================================================================
set -euo pipefail
cd "$(dirname "$0")"
export PATH="$HOME/.local/bin:$PATH"   # sandbox: pip-installed cmake/ninja

echo "== [1/4] shaders -> SPIR-V (glslangValidator) =="
mkdir -p build/shaders
for f in shaders/triangle.vert shaders/triangle.frag; do
  name="$(basename "$f")"
  glslangValidator -V -o "build/shaders/${name}.spv" "$f"
done

echo "== [2/4] configure (MinGW-w64 cross) =="
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake

echo "== [3/4] build (Ninja) =="
cmake --build build

echo "== [4/4] artifacts =="
ls -l build/output/astra.exe build/output/shaders/
sha256sum build/output/astra.exe build/output/shaders/*.spv
echo "BUILD OK"
