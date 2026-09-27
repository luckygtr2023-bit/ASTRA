# Project Astra Cosmos — Phase 0 Exit Report

| Field | Value |
|---|---|
| **Version** | 1.2 (v1.0 initial · v1.1 filename/link audit · v1.2 deep code audit — all same day) |
| **Date** | 2026-09-28 |
| **Branch** | `arena/01a0df74-astra` (sandbox session branch; the brief's `develop`/`main` naming is a user-side rebase — the sandbox session is fixed to this branch) |
| **Sandbox** | Linux x86_64, 2 CPUs, ~3 GB RAM, Debian userland, **network allowlist: github.com, codeload.github.com, pypi.org only** |
| **Gate** | **PHASE0_GATE: FAIL** (see §8 — single cause: the MinGW cross compiler is unobtainable in-sandbox; everything else passed) |

---

## 1. Toolchain validation — every command, exit code, and the method that worked

The sandbox network allowlist (only `github.com`, `codeload.github.com`, `pypi.org`
reachable) shaped this whole section. Probes used
`curl -s -o /dev/null -w code=%{http_code}` (a `curl | head; echo $?` probe
**lies** — it captures `head`'s exit code; this was caught and corrected).

| # | Tool | Method tried | Result |
|---|---|---|---|
| 1 | apt (any .deb) | `apt-get update` (http) | **FAIL** — port 80 blocked |
| 1 | apt (any .deb) | sed sources to https + `apt-get update` (×3, incl. `-o Acquire::Retries=3`, 5 Debian mirrors: deb.debian.org, kernel.org, halifax, leaseweb, dotsrc) | **FAIL** — all `curl exit 35` (TLS "non-properly terminated" via e2b proxy). apt declared dead, not retried. |
| 1 | pre-seeded packages | `dpkg -l \| grep -E 'mingw\|wine\|llvm\|clang'`, `ls /var/cache/apt/archives/*.deb`, full-filesystem `find` for `*mingw*`/`*w64*` | **FAIL** — none present (count 0) |
| 2 | **cmake** | `pip3 install --user --break-system-packages cmake` | **PASS** — cmake 4.4.3 → `~/.local/bin` (exit 0) |
| 3 | **ninja** | `pip3 install --user --break-system-packages ninja` | **PASS** — ninja 1.13.2 → `~/.local/bin` (exit 0) |
| 4 | git, python3, pip3, g++ (host) | preinstalled in image | **PASS** — git 2.39.5, python 3.11.2, GCC 12.2.0 |
| 5 | **Vulkan headers** | `git clone --depth 1 --branch v1.3.296 https://github.com/KhronosGroup/Vulkan-Headers libs/Vulkan-Headers` | **PASS** — v1.3.296, commit `29f979e` (the TDD-sanctioned vendored fallback; gitignored) |
| 6 | **glslang** | PyPI — no glslang wheel exists. Then `git clone --depth 1 --branch vulkan-sdk-1.4.357.0 https://github.com/KhronosGroup/glslang /tmp/glslang` + `cmake -S /tmp/glslang -B /tmp/glslang-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_SPIRV=ON -DENABLE_HLSL=OFF -DENABLE_OPT=OFF -DENABLE_TESTS=OFF -DBUILD_SHARED_LIBS=OFF` (exit 0) + `cmake --build /tmp/glslang-build --target glslang-standalone` (exit 0) | **PASS** — binary `/tmp/glslang-build/StandAlone/glslang` (glslang 16.4.0), symlinked to `~/.local/bin/glslangValidator` and `~/.local/bin/glslang`. Note: the standalone CMake build names the binary `glslang` (target `glslang-standalone`), not `glslangValidator`; CMakeLists.txt and build.sh accept both names. |
| 7 | **Vulkan-Loader** (native smoke only) | `git clone --depth 1 --branch v1.3.296 https://github.com/KhronosGroup/Vulkan-Loader /tmp/Vulkan-Loader` + headers installed to `/tmp/vulkan-install` (exit 0) + `cmake -S /tmp/Vulkan-Loader -B /tmp/vloader-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/tmp/vulkan-install -DBUILD_WSI_XCB_SUPPORT=OFF -DBUILD_WSI_XLIB_SUPPORT=OFF -DBUILD_WSI_WAYLAND_SUPPORT=OFF` (exit 0) + `cmake --build /tmp/vloader-build` (exit 0) | **PASS** — `libvulkan.so.1.3.296`. WSI disabled (no X11 dev libs; headless smoke needs none). A `pkg-config` shim was required (CMake requires the binary; the shim answers `--version` and reports every module missing, so the optional WSI deps degrade cleanly). |
| 8 | **x86_64-w64-mingw32-g++** | apt (dead, #1) → every Debian/Ubuntu mirror raw curl (exit 35) → ghcr.io (Linuxbrew) blocked → conda.anaconda.org blocked → sourceware.org / ftp.gnu.org blocked → PyPI `ziglang` wheels 0.08.0–0.16.0 = **68–98 MB, all over the 50 MB per-download rule** (checked via pypi.org JSON) → **zig 0.14.1 from GitHub source**: clone OK (~35 MB), but (a) the 0.14.1 root has no `./zig` build script, (b) its CMake build requires host **LLVM 19** (`find_package(llvm 19)` unconditional — configure exit 1), (c) manual stage-1 bootstrap (gcc `stage1/wasm2c.c` → wasm2c → `zig1.wasm`→`zig1.c` (234 MB) → gcc → `/tmp/zig1`, exit 0) produced a working bootstrap whose `cc` subcommand is partial ("development environment bootstrap does not support feature …") — full `zig cc` needs the self-hosted stage, i.e. host LLVM again. | **FAIL** — no path to a Windows PE cross compiler inside the sandbox rules. Recorded as **TDD T-014** (decision log). |
| 9 | wine (to run astra.exe in-sandbox) | not installable (apt dead; no mirror allowed) | **FAIL** — the Windows binary can only be executed on the user's laptop. Headless verification on Windows is therefore **PENDING USER-SIDE TEST**. |

**Net toolchain status:** cmake ✓ · ninja ✓ · git ✓ · python3+pip ✓ ·
Vulkan headers ✓ (vendored) · glslang ✓ (source-built) ·
**x86_64-w64-mingw32-g++ ✗ (blocked)** · wine ✗ (blocked).

## 1a. v1.1 audit pass (2026-09-28, brief re-submitted)

The Phase 0 brief was re-submitted; the deliverable set was re-audited line by
line against the repository. Findings, fixes, and the full re-verification
(v1.1):

| # | Finding | Fix |
|---|---|---|
| F-1 | Doc filenames deviated from the brief's exact list (`docs/` must contain `GDD.md, TDD.md, ArtBible.md, AudioDesign.md, MilestonePlan.md`) | `git mv` to the exact names; **36 cross-references rewritten** across 7 files (path refs + body title mentions); 1 pre-existing broken link fixed (`art/refs/README.md` → `../../docs/ArtBible.md`). Link audit after: **17/17 markdown links resolve, 0 broken, 0 stale tokens** |
| F-2 | **Latent Windows compile failure:** `vkCreateSemaphore`/`vkDestroySemaphore` were called in the windowed path but never loaded in `vk_loader.h` (invisible to the Linux smoke build, which compiles that path out) | Both entry points added to the runtime loader — now **64/64** entry points; static check: every `vk*` call token in main.cpp (61 unique) resolves to a loaded global |
| F-3 | README status section lacked the brief's literal line | Restored heading `Phase 0 status: toolchain validated` (with the honest caveat paragraph below it) |
| F-4 | Between turns the sandbox reset `.git` to the initial commit and wiped `/tmp` + build artifacts (worktree sources survived intact) | Full toolchain re-provisioned from the same pinned sources and the entire verification chain re-run — results below |

**Re-verification log (v1.1, all exit codes):**

| Step | Command | Exit |
|---|---|---|
| pip cmake+ninja | `pip3 install --user --break-system-packages cmake ninja` | 0 |
| clone Vulkan-Headers v1.3.296 → `libs/` | `git clone --depth 1 --branch v1.3.296 …` | 0 |
| clone glslang vulkan-sdk-1.4.357.0 | `git clone --depth 1 --branch vulkan-sdk-1.4.357.0 …` | 0 |
| clone Vulkan-Loader v1.3.296 | `git clone --depth 1 --branch v1.3.296 …` | 0 |
| glslang configure / build `glslang-standalone` | `cmake … -DENABLE_OPT=OFF … && cmake --build … --target glslang-standalone` | 0 / 0 |
| headers install to `/tmp/vulkan-install` | `cmake -S libs/Vulkan-Headers … && cmake --install` | 0 / 0 |
| loader configure (WSI off) | first attempt exit 1 (pkg-config shim dir missing from PATH) → corrected PATH → exit **0** (retry 1 of 3) | 0 |
| loader build | `cmake --build /tmp/vloader-build` | 0 |
| native strict build | `cmake … -DCMAKE_CXX_FLAGS="-Wall -Wextra -O2" && cmake --build build-native` | 0 — **0 warnings, 0 errors** |
| headless smoke | `LD_LIBRARY_PATH=/tmp/vloader-build/loader ./build-native/output/astra --headless` | **0** (clean "no drivers in sandbox" report) |
| cross pipeline | `./build.sh` | **1** — step 1 shaders OK; step 2 configure fails identically to v1.0 (`x86_64-w64-mingw32-g++ … not found in the PATH`) — reproducible, environmental |

**Re-verification results (v1.1):** artifact SHA-256 hashes are **byte-identical to
§5** (deterministic build confirmed); fragment SPIR-V still 3 instructions;
code total 1389 lines (< 1500); static VK_NO_PROTOTYPES contract satisfied
(64/64 entry points, all call tokens resolved); validation layers still
guarded by `#ifndef NDEBUG` only.

## 1b. v1.2 deep audit (2026-09-28, brief re-submitted a third time)

The brief was re-submitted again; environment re-checked first (allowlist still
github/pypi only — no .debs dropped in, no mingw, blocker unchanged). The audit
then went one level deeper: **a full static review of the windowed-mode code
path — the only path never compiled in this sandbox** (the native smoke build
compiles it out via `#ifdef _WIN32`, so any error there would surface as a
crash or black screen on the user's laptop, not here).

**Findings — two real bugs, both fixed:**

| # | Bug (windowed path only) | Failure mode on the laptop | Fix |
|---|---|---|---|
| B-1 | Render pass was built with a hardcoded `B8G8R8A8_SRGB` while the swapchain picks the driver's actual format (and `VK_FORMAT_UNDEFINED` = "driver's choice" was never resolved) | **Black screen / garbage on first frame** if the AMD driver offers any other format | `create_swapchain` now returns the chosen format (resolving `VK_FORMAT_UNDEFINED` to `B8G8R8A8_SRGB`) and the render pass is built from it |
| B-2 | `destroy_windowed_resources()` called `vkDeviceWaitIdle`/`vkDestroyDevice`/`vkDestroySurfaceKHR` unconditionally, but every early-failure path reaches it with `g_device == VK_NULL_HANDLE` | **Crash on any early failure** (driver missing, surface rejected, …) | Teardown is now fully null-guarded (device scope, surface, instance — each destroyed only if created) |

**Reviewed and verified correct (no changes needed):** Win32 window setup
(`RegisterClassExW`/`CreateWindowExW`, 1920×1080 clamped to work area,
non-resizable by design); Win32 surface creation (hinstance/hwnd); device
selection preference (integrated → discrete → any surface-capable); graphics
queue-family discovery; swapchain parameters (FIFO = vsync, min+1 images
clamped to max, client-rect extent clamped to surface caps); render pass
attachment/dependency; pipeline state (dynamic viewport/scissor, push-constant
range 4 bytes = one float, vertex binding stride 20 B matching the shader
locations 0/1); command recording; submit/present/fence/semaphore invariants
on **every** failure branch of the frame loop (no deadlock, no stale fence);
destruction dependency order; instance extension set; validation layers
strictly `#ifndef NDEBUG`.

**Known Phase 0 simplifications (documented, correct on the target hardware —
single-family iGPU):** surface support is queried on queue family 0; the window
is non-resizable (resize support is Phase 1 scope); single graphics queue.

**v1.2 re-verification (sandbox re-wiped `.git`/`/tmp` again between turns —
full re-provision and re-run):** pip cmake+ninja 0 · 3× git clone 0 · glslang
config/build 0/0 · headers install 0/0 · loader config/build 0/0 · strict
build (`-Wall -Wextra -O2`) **0 — 0 warnings, 0 errors** · headless smoke
**exit 0** · `./build.sh` **exit 1** at the identical point (step 1 shaders OK;
step 2 configure: `x86_64-w64-mingw32-g++ … not found in the PATH`). SHA-256:
both .spv **byte-identical to §5**; the native `astra` binary is also
byte-identical — expected, because both fixes live inside the `#ifdef _WIN32`
path the native build excludes.

## 2. Repository tree (post-Phase 0, committed state)

```
astra/
├── .gitignore
├── .start.bat                     # CRLF; plain-English error if astra.exe missing; launches --cpu-priority=high
├── CMakeLists.txt
├── README.md
├── art/refs/README.md
├── assets/.gitkeep
├── build.sh                       # one-shot pipeline; non-zero exit on any failure
├── cmake/
│   └── mingw-w64-x86_64.cmake     # cross toolchain file (Release flags per TDD, -march=znver1, static libs)
├── data/.gitkeep
├── docs/
│   ├── ArtBible.md               # v1.1
│   ├── AudioDesign.md            # v1.3
│   ├── GDD.md                     # v1.0 (§13 now points at the Milestone Plan)
│   ├── MilestonePlan.md          # v1.0 — 8 phases, MP Phase n = GDD Phase n+1
│   └── TDD.md                     # v1.0 (T-001…T-014; T-014 = this report's toolchain finding)
├── libs/
│   ├── .gitkeep
│   └── Vulkan-Headers/            # v1.3.296 clone (gitignored, refetch documented)
├── scripts/.gitkeep
├── shaders/
│   ├── triangle.frag              # passthrough; 3 SPIR-V instructions (< 10 required)
│   └── triangle.vert              # vec2 pos + vec3 color in; rotation via ONE push-constant float
└── src/
    ├── main.cpp                   # headless probe + raw Win32 1080p window, vsync, ESC
    └── vk_loader.h                # VK_NO_PROTOTYPES runtime loader (LoadLibraryA/dlopen, 64 entry points)
```

Build outputs (gitignored): `build/output/` (cross) · `build-native/output/` (smoke)
with `astra(.exe)` + `shaders/triangle.{vert,frag}.spv`.

**Generated code: 1408 lines** (main.cpp 925, vk_loader.h 275, shaders 30,
CMakeLists 86, toolchain file 36, build.sh 34, .start.bat 22) — under the 1500 cap.
(+23 lines vs v1.0: semaphore entry points + the two v1.2 fixes, all in the windowed path.)

## 3. Compiler + linker log (native smoke build — the C++ exactly as shipped, strict flags)

Command:

```
$ cmake -S . -B build-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wextra -O2"
$ cmake --build build-native
```

Complete log (abridged to the compiler+linker lines; the configure section is a
standard CMake/Ninja host configure with no warnings):

```
[1/4] glslang: triangle.frag -> SPIR-V
/home/user/ASTRA/shaders/triangle.frag
[2/4] glslang: triangle.vert -> SPIR-V
/home/user/ASTRA/shaders/triangle.vert
[3/4] Building CXX object CMakeFiles/astra.dir/src/main.cpp.o
[4/4] Linking CXX executable output/astra; copying SPIR-V next to the executable
```

**Warnings: 0. Errors: 0.** (`-Wall -Wextra -O2`, g++ 12.2.0, C++17,
`VK_NO_PROTOTYPES` defined, dlopen path active.)

The **Windows Release** compile uses `-O2 -march=znver1 -static-libgcc
-static-libstdc++ -DVK_NO_PROTOTYPES -DNDEBUG -Wall -Wextra` (toolchain file
`cmake/mingw-w64-x86_64.cmake`, also set in CMakeLists for the target). The
Windows compile could not be executed here (no cross compiler — §8); the same
source compiled warning-free under identical warning flags on the host.

## 4. Cross-compile attempt — exact failure (evidence)

```
$ ./build.sh
== [1/4] shaders -> SPIR-V (glslangValidator) ==
shaders/triangle.vert
shaders/triangle.frag
== [2/4] configure (MinGW-w64 cross) ==
-- The CXX compiler identification is unknown
CMake Error at CMakeLists.txt:2 (project):
  The CMAKE_CXX_COMPILER:

    x86_64-w64-mingw32-g++

  is not a full path and was not found in the PATH.

  Tell CMake where to find the compiler by setting either the environment
  variable "CXX" or the CMake cache entry CMAKE_CXX_COMPILER to the full path
  to the compiler, or to the compiler name if it is in the PATH.

-- Configuring incomplete, errors occurred!
build.sh exit code: 1   (set -euo pipefail aborts on any failure, as specified)
```

Step 1 (shaders → SPIR-V) **passed**; the failure is at the compiler boundary,
exactly where the missing toolchain sits.

## 5. Artifact hashes and sizes

| Artifact | Size | SHA-256 |
|---|---|---|
| `build/shaders/triangle.vert.spv` | 1,608 B | `a8fc1bee109870ce521b7ccd854da8f399788b5a8294ec9c7177b766336e495b` |
| `build/shaders/triangle.frag.spv` | 496 B | `26dfa9e090dcfc735f4ee322e23d35db4ccd4d8c098f2d934122576dde528ab4` |
| `build-native/output/astra` (smoke build, Linux) | 21,336 B | `eec99ae21ab604d6e5781ad72acadd810fc0b865060935269a787f2ef431d6f2` |
| `build/output/astra.exe` | — | **NOT PRODUCED — cross compiler missing (§8)** |

The SPIR-V shipped next to the executable is byte-identical to the
`build/shaders/` copies (verified by hash in both output trees).

Shader contract verification (parsed from the SPIR-V words):
`triangle.frag` = **3 OpInstructions total** (limit: < 10). `triangle.vert`
reads `aPos` (loc 0, R32G32) + `aColor` (loc 1, R32G32B32) and rotates by the
single push-constant float.

## 6. Headless probe output

Native smoke run (built Vulkan-Loader 1.3.296, no ICD present — the sandbox has
no GPU driver, by design):

```
$ LD_LIBRARY_PATH=/tmp/vloader-build/loader ./build-native/output/astra --headless
WARNING: no Vulkan drivers found in this environment (expected in a sandbox without a GPU/ICD).
On the target laptop with the AMD driver, this probe prints every physical device.
exit code: 0
```

Loader trace (VK_LOADER_DEBUG=all) confirms the full runtime path works:
`Vulkan Loader Version 1.3.296` → layer/ICD manifest search in all standard
locations → `Found no drivers` (no ICD in sandbox) → app handles it cleanly and
exits 0. This is the strongest in-sandbox verification possible without a GPU:
dlopen → entry-point resolution → vkCreateInstance dispatch → manifest search
all execute correctly.

**Windows `--headless` output: PENDING USER-SIDE TEST** (no wine in sandbox;
the AMD ICD only exists on the laptop). On the laptop the probe must print the
Radeon Vega 8 device (integrated), its queue families, and exit 0.

## 7. Phase 0 checklist

- [x] **Docs** — the five signed-off documents under `docs/` exactly as the brief lists them: `GDD.md` v1.0, `TDD.md` v1.0 (+T-014), `ArtBible.md` v1.1, `AudioDesign.md` v1.3, `MilestonePlan.md` v1.0; all cross-referenced (17/17 links resolve), GDD §13 mapped (MP Phase n = GDD Phase n+1)
- [x] **Git** — repo initialized (session checkout), Phase 0 committed to `arena/01a0df74-astra`
- [ ] **Hello-Vulkan cross-compiles to Windows** — BLOCKED: `x86_64-w64-mingw32-g++` unobtainable in the sandbox (§1 #8, §4, T-014). All other pipeline steps (shaders → configure → build → hash) are in place and will run unmodified once the compiler exists.
- [ ] **Triangle at 60 FPS on Vega 8** — PENDING USER-SIDE TEST (requires the cross-built `astra.exe`)

## 8. Gate

**Why FAIL (single root cause):** the Phase 0 gate requires
"Hello-Vulkan cross-compiles to Windows". The MinGW-w64 cross compiler cannot
be installed in this sandbox: apt and every Debian/Ubuntu mirror are TLS-blocked
by the network allowlist, ghcr/conda/sourceware are blocked, all `ziglang` PyPI
wheels exceed the 50 MB per-download rule, and a from-source zig bootstrap
requires host LLVM 19, which is likewise unobtainable. Every other Phase 0
deliverable passed: docs, skeleton, toolchain (5 of 7 components), shaders,
build system, zero-warning native compile of the exact shipped source, and a
clean headless smoke run (exit 0) against a from-source Vulkan loader.

**First fix attempt (user-side, pick one):**

1. **Grant the sandbox egress to `deb.debian.org`** (the allowlist currently
   permits only github.com + codeload.github.com + pypi.org). Then:
   `sudo apt-get update && sudo apt-get install g++-mingw-w64-x86-64` →
   `./build.sh` → gate re-evaluated. (Fastest; ~2 minutes.)
2. **Drop the MinGW .debs into the workspace** — on any machine download
   `g++-mingw-w64-x86-64`, `gcc-mingw-w64-x86-64`, `mingw-w64-x86-64-dev`,
   `binutils-mingw-w64-x86-64` (+ deps) from a Debian mirror into
   `tools/debs/` in the repo; I install them with `dpkg -i` and rerun
   `build.sh`.
3. **Allow one exception to the 50 MB rule** for the `ziglang` PyPI wheel
   (~98 MB): `pip install ziglang` provides a full `zig cc`, and
   `cmake/mingw-w64-x86_64.cmake` can then target
   `zig cc -target x86_64-windows-gnu` (T-014 records this as the sanctioned
   fallback toolchain, keeping g++-mingw-w64 as the canonical one).

Once the compiler exists, `./build.sh` is the single command to re-open the
gate; the headless Windows probe and the 60 FPS triangle check then complete
the two remaining checklist rows.

---

PHASE0_GATE: FAIL

- MinGW-w64 cross compiler (x86_64-w64-mingw32-g++) is unobtainable in this sandbox (network allowlist + 50 MB rule); the Windows cross-build and the user-side triangle check therefore could not run. Every other Phase 0 criterion passed (docs, skeleton, 5/7 toolchain components, shaders, zero-warning build, clean headless smoke exit 0).
- v1.1 audit (2026-09-28, brief re-submitted): three deviations found and fixed (doc filenames aligned to the brief's exact list with all 36 cross-references rewritten and 17/17 links verified; missing semaphore entry points added — a latent Windows compile failure; README status line restored). Full re-verification re-ran every step with identical results (byte-identical artifact hashes). Gate outcome unchanged: the single blocker remains the cross compiler.
- v1.2 deep audit (2026-09-28, brief re-submitted a third time): the windowed-mode path — never compiled in-sandbox — was statically reviewed line by line; two real bugs found and fixed (render-pass/swapchain format mismatch → black screen; null-deref in early-failure teardown → crash). Code re-verified warning-free; gate outcome unchanged: the single blocker remains the cross compiler.
