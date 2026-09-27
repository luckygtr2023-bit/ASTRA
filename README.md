# ASTRA

**Project Astra Cosmos** — story-driven space exploration, 1:1-scale procedural Milky Way, C++17 + Vulkan, iGPU-first (Ryzen 7 / Vega 8, <8 GB RAM, adaptive 30–60 FPS).

## Phase 0 status: toolchain validated

Phase 0 delivered the repository skeleton, the docs family (GDD/TDD/Art
Bible/Audio/Milestone Plan), the build system, and the Hello-Vulkan source
(`astra.exe` + `shaders/`), compiled warning-free and smoke-tested headless
(clean exit 0 against a from-source Vulkan loader). The one blocked item: the
MinGW-w64 cross compiler could not be installed in the build sandbox (network
allowlist) — see `docs/PHASE0_EXIT_REPORT.md` (gate line at the bottom) for
the exact failure and the three user-side fix options. Once `x86_64-w64-mingw32-g++`
is available, `./build.sh` produces the Windows artifacts; then double-click
`.start.bat` on the laptop — you should see a rotating colored triangle at
vsynced 60 FPS on the Vega 8.

## Documents

| Doc | Role |
|---|---|
| [GDD](docs/GDD.md) | Game Design Document — design source of truth & living decision log (§17) |
| [TDD](docs/TDD.md) | Technical Design Document — implementation blueprint & technical decision log (§9) |
| [ArtBible](docs/ArtBible.md) | Art Bible — visual style, color system, asset & reference authority (§11) |
| [AudioDesign](docs/AudioDesign.md) | Audio Design Document — sound design & audio implementation authority (§9) |
| [MilestonePlan](docs/MilestonePlan.md) | Milestone Plan — canonical schedule (MP Phase n = GDD Phase n+1) |

Conflict rule: GDD wins on *what exists in the game*, TDD wins on *how it is computed*, Art Bible on *how it looks*, Audio Design on *how it sounds*. Decisions are cross-logged between documents.

## Build

**Windows target (the shipped artifact):**

```bash
./build.sh
# -> build/output/astra.exe + build/output/shaders/*.spv
```

`build.sh` compiles the shaders to SPIR-V (glslangValidator), configures CMake
with the MinGW-w64 cross toolchain (`cmake/mingw-w64-x86_64.cmake`, Ninja),
builds, and prints the size + sha256 of every artifact. Any failure aborts with
a non-zero exit code.

**Headless probe (any platform with a Vulkan runtime):**

```bash
./build/output/astra.exe --headless   # Windows, or the native smoke build
```

Prints every physical device (name, type, driver) + queue families, selects the
first integrated/discrete GPU, exits 0.

## Repository layout

```
astra/
├── .start.bat            # double-click launcher (Windows)
├── build.sh              # one-shot build pipeline
├── CMakeLists.txt
├── cmake/                # cross toolchain files
├── docs/                 # GDD, TDD, Art Bible, Audio Design, Milestone Plan, exit reports
├── src/                  # main.cpp, vk_loader.h (runtime Vulkan loader)
├── shaders/              # GLSL sources
├── scripts/              # tooling (Phase 1+)
├── assets/               # content (Phase 1+)
├── data/                 # .astroct portable data (Phase 1+)
└── libs/                 # vendored headers (Vulkan-Headers)
```
