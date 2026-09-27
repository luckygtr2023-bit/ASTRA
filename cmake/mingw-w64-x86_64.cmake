# ===========================================================================
# MinGW-w64 cross toolchain file — Linux -> Windows x64 (Project Astra Cosmos)
#
# Usage:
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake
#
# Requires x86_64-w64-mingw32-g++ on PATH (Debian/Ubuntu:
#   sudo apt-get install g++-mingw-w64-x86-64).
#
# Target hardware: AMD Radeon Vega 8 (Zen 2 -> -march=znver1).
# ===========================================================================
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Cross-compile root (host tools are never discovered from the target root).
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Phase 0 release flags (TDD: Vega 8 tuned, fully static — the shipped folder
# must contain only astra.exe + shaders/ + .start.bat).
set(CMAKE_CXX_FLAGS_RELEASE
    "-O2 -march=znver1 -static-libgcc -static-libstdc++ -DVK_NO_PROTOTYPES -DNDEBUG -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE
    "-O2 -march=znver1 -static-libgcc -DVK_NO_PROTOTYPES -DNDEBUG -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
