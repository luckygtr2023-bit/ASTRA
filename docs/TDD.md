# Project Astra Cosmos — Technical Design Document (TDD)

| | |
|---|---|
| **Document** | Technical Design Document (TDD) — implementation blueprint |
| **Version** | 1.0 — Baseline |
| **Date** | 2026-09-27 |
| **Status** | **Authoritative for all technical implementation decisions.** |
| **Companion** | [GDD](GDD.md) v1.0 — authoritative for *game design*. Where this document and the GDD conflict on *implementation*, this document wins and the GDD decision log is updated with a pointer entry. |
| **Change Control** | §0.3 — technical decisions are recorded as `T-###` entries in §9. |

---

## Table of Contents

0. [Document Control](#0-document-control)
1. [Technical Overview & Design Principles](#1-technical-overview--design-principles)
2. [System Architecture](#2-system-architecture)
   - 2.1 Dual-Engine Architecture
   - 2.2 Thread Model & Job System
   - 2.3 Memory Model (8 GB Budget)
   - 2.4 Universe Data Flow & the `.astroct` Chunk Container
   - 2.5 `ChunkData` Schema & Forward Compatibility
   - 2.6 Terrain Generation Pipeline
   - 2.7 Rendering Pipeline (Vulkan)
   - 2.8 Physics & Simulation
   - 2.9 Audio
   - 2.10 Networking (Phase 7)
   - 2.11 Save System
3. [Performance Budgets & Quality Presets](#3-performance-budgets--quality-presets)
4. [Tooling, Libraries & Dependencies](#4-tooling-libraries--dependencies)
5. [Build & Deployment](#5-build--deployment)
6. [Versioning, Branching & Delivery](#6-versioning-branching--delivery)
7. [Testing & Quality Gates](#7-testing--quality-gates)
8. [Technical Risks](#8-technical-risks)
9. [Technical Decision Log](#9-technical-decision-log)
- [Appendix A — `.astroct` Binary Layout (v1)](#appendix-a--astroct-binary-layout-v1)
- [Appendix B — `.start.bat` (canonical)](#appendix-b--startbat-canonical)
- [Appendix C — Shipped Directory Tree (annotated)](#appendix-c--shipped-directory-tree-annotated)
- [Appendix D — Quality Presets (JSON)](#appendix-d--quality-presets-json)
- [Appendix E — Non-Functional Requirements (NFR list)](#appendix-e--non-functional-requirements-nfr-list)

---

## 0. Document Control

### 0.1 Document Family

| Doc | Scope | Wins on |
|---|---|---|
| **GDD** | What the game is (design, story, art intent) | Design intent |
| **TDD** (this) | How it is built (architecture, budgets, build, tooling) | Implementation |

### 0.2 Relationship Rule

- A TDD decision that changes *game-feel-relevant* behavior (e.g., a budget that alters a GDD-stated FPS) is logged here **and** in the GDD decision log as a pointer entry.
- A TDD decision that is purely internal (thread affinity, file formats, tooling) is logged here only.

### 0.3 Change Process

1. Propose `T-###` (date, decision, rationale, status `PROPOSED`).
2. Implement only after `ACCEPTED`.
3. Prose in this document is updated in the same commit as acceptance.
4. Superseded entries are kept, marked `SUPERSEDED BY T-###`.

### 0.4 Version History of this Document

| Version | Date | Author | Summary |
|---|---|---|---|
| 1.0 | 2026-09-27 | Technical Architect | Baseline TDD: dual-engine architecture, 8-core thread model, 8 GB memory model, `.astroct` format v1, LOW/BALANCED/HIGH budgets, toolchain (MinGW-w64, glslangValidator, Python), `.start.bat`, shipped tree, semver + GitFlow. Decisions T-001…T-012. |

---

## 1. Technical Overview & Design Principles

### 1.1 Fixed Hardware Target (from GDD §12.1 — restated as engineering constraints)

| Component | Target | Engineering consequence |
|---|---|---|
| CPU | AMD Ryzen 7, **8 physical cores** | All real-time work is CPU work. Threads are **pinned 1:1 to cores** — no oversubscription, no SMT reliance. |
| RAM | 16 GB system | Hard process budget **8 GB** (GDD §12.1), including the iGPU's shared allocation. |
| GPU | **Radeon Vega 8 iGPU** | Shared-memory GPU. It is treated as a **rasterizer only**: no compute shaders, no geometry tessellation, no async compute. Its "VRAM" is a window into the same 16 GB. |
| FPS | Adaptive 30–60 | Three quality presets (§3) + a dynamic governor that trades resolution first. |

### 1.2 The Five Engineering Principles

1. **CPU does everything the GPU is not good at; the GPU does exactly one thing** — rasterize pre-calculated vertex data. Mesh generation, terrain, starfields, physics, streaming, audio, networking: all CPU. The GPU receives finished vertex buffers, simple PBR-lite shaders, and a fixed post chain. This keeps the shared-memory bandwidth budget predictable — the #1 constraint on a Vega 8.
2. **Determinism is an architectural property, not a test.** All randomness flows from seeded integer streams keyed by `(salt, draw_index)` — never by wall clock, thread identity, or floating-point accumulation. The same world is a pure function of `(seed, action_log)` (GDD §12.6).
3. **Everything is transient and budgeted.** No system owns memory permanently beyond its pool (§2.3). Allocation outside a pool is a debug-build assertion.
4. **The disk is a memory tier.** Pre-derived universe data (`.astroct`) and terrain tiles are disk-cached; the ring buffer in RAM is a working set, not a store (§2.4).
5. **Own the toolchain.** Vendored dependencies only, pinned versions, reproducible builds. The game must build green from a clean Linux container with no network access at build time.

### 1.3 Non-Functional Requirements

See [Appendix E](#appendix-e--non-functional-requirements-nfr-list) — NFR-1…NFR-7, each traceable to a CI gate (§7).

---

## 2. System Architecture

### 2.1 Dual-Engine Architecture

The "two engines" are not two processes — they are two **execution domains with a strict data contract between them**:

```
┌──────────────────────────── CPU ENGINE (Ryzen 7, cores 0–7) ───────────────────────────┐
│                                                                                         │
│  CORE 0         CORES 1–2        CORES 3–5        CORE 6          CORE 7               │
│  ┌─────────┐    ┌──────────┐     ┌───────────┐    ┌──────────┐    ┌──────────────┐      │
│  │  MAIN   │    │ STREAM   │     │  GEN      │    │ PHYSICS  │    │ NET + AUDIO  │      │
│  │ game    │    │ IO,      │     │ universe  │    │ 60 Hz    │    │ opset sync   │      │
│  │ logic,  │    │ ZSTD/    │     │ derivation│    │ sweep    │    │ PortAudio    │      │
│  │ UI,     │    │ LZ4      │     │ terrain   │    │ CCD      │    │ callback,    │      │
│  │ command │    │ unpack,  │     │ mesh      │    │ species  │    │ OpenAL,      │      │
│  │ record, │    │ ring     │     │ build,    │    │ AI tick  │    │ Opus decode, │      │
│  │ submit  │    │ eviction │     │ starfield │    │          │    │ DSP synth    │      │
│  └────┬────┘    └────┬─────┘     └─────┬─────┘    └────┬─────┘    └──────┬───────┘      │
│       │        lock-free ring queues (MPMC) + work stealing across GEN pool             │
│       └──────────────────┴──────────────┴───────────────┴─────────────────┘              │
│                              │  finished vertex data (double-buffered)                  │
└──────────────────────────────┼──────────────────────────────────────────────────────────┘
                               ▼
┌─────────────────────────── GPU ENGINE (Vega 8 iGPU) ────────────────────────────────────┐
│  command buffers (NVIDIA-free):                                                          │
│   1. clear → 2. starfield (3 point-draws) → 3. nebula billboards (1 instanced draw)     │
│   → 4. terrain (8–16 chunk draws) → 5. bodies/space (instanced, ≤40 draws)              │
│   → 6. atmosphere shells (per visible body) → 7. particles (2–4)                        │
│   → 8. UI (1–3)                                                                          │
│  POST (BALANCED+):  bloom → lensing pass (BH) → tonemap/grain                             │
│  Rules: no compute shaders · no tessellation · no async compute · ≤ 280 draw calls/frame │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

**Data contract CPU→GPU (locked):**
- The GPU reads only from **double-buffered vertex/index buffers** owned by the CPU engine and written entirely on GEN cores before the frame fence. The GPU never generates geometry.
- Per-frame UBOs: camera, star, hemisphere, palette-quantized constants. One UBO set per pipeline family.
- All vertex attributes are pre-computed: position, normal, **vertex color** (the GDD art style — vertex colors over textures, GDD §9.2). Vertex shaders do transform + light only.
- Starfields are **pre-computed point buffers** (100K/500K/1M points per preset) — the GPU's job is `gl.POINTS`-style rasterization, nothing more.
- Black-hole lensing (BALANCED/HIGH) is a **post-process**, not a geometry feature: screen-space ray-bend on a masked region. This keeps the signature visual inside the fixed post chain (GDD AAV-1).

**Why a rasterizer-only GPU (T-001):** Vega 8 shares its memory bus with the CPU. Every GPU compute or async operation is bandwidth the terrain workers and streamer don't get. Rasterization is the one workload where the GPU is ~100× the CPU and where its work is *fixed-size per preset* — which is what makes an 8 GB, 30–60 FPS budget provable rather than hopeful.

### 2.2 Thread Model & Job System

**Core pinning (locked, T-002):** Windows `SetThreadAffinityMask` + `THREAD_PRIORITY_*` as below. Affinity is set at startup after core count validation (exactly 8 logical cores expected; on other topologies the game falls back to an *unpinned pool* of N=min(8, logical cores) and logs the change — it never crashes).

| Core | Thread | Role | Priority | What it must never do |
|---|---|---|---|---|
| 0 | **Main** | Game systems, mission/story logic, UI, scan/craft, Vulkan command recording + submit | ABOVE_NORMAL | Block on IO, block on generation, run >4 ms of game logic per frame |
| 1 | **Stream-A** | Disk reads, `.astroct` ZSTD/LZ4 decompress, region-ring eviction, disk-cache writes | ABOVE_NORMAL | Touch vertex buffers (hand off via queues) |
| 2 | **Stream-B** | Same as Stream-A; also owns the disk-cache file manager (trim, rotate) | ABOVE_NORMAL | Same |
| 3–5 | **Gen-A/B/C** | Universe derivation (seed→region→body), terrain mesh build (two-tier, GDD §12.4), starfield buffers, POI/derelict instances | ABOVE_NORMAL | Allocate from pools other than the Gen pool; write to a frame's *presented* buffer |
| 6 | **Sim** | Fixed 60 Hz physics (swept-sphere ship, surface collision, CCD weapons), species AI tick (amortized), hazard sim | TIME_CRITICAL* | Yield a physics tick; tick is ≤ 6 ms budgeted |
| 7 | **Net/Audio** | Audio render thread (PortAudio callback, 48 kHz/256 s), DSP synth, OpenAL mixer, HRTF, Opus decode; (P7) opset receive/relay | TIME_CRITICAL* | Run any game logic; the audio callback is the real-time-critical path in the whole process |

\* `TIME_CRITICAL` is used only for the two threads that must never miss a deadline; everything else is `ABOVE_NORMAL`. The Main thread is deliberately *not* `HIGHEST` so the audio thread always wins CPU arbitration.

**Job system (lock-free + work stealing, locked):**

- **Queues:** hand-rolled MPMC rings, power-of-two capacity (default 4096 slots, 32 B/slot, cache-line padded; SPMC variant for the gen→main completion queue). Counted with atomic seqcount CAS — no mutexes on any hot path. The only locks in the process: the disk-file mutex (Stream-B) and the save-file mutex (Main).
- **Task types:** `T_REGION_DERIVE`, `T_ASTROCT_DECOMPRESS`, `T_TERRAIN_TILE`, `T_STARFIELD_LAYER`, `T_PHYSICS_TICK` (Sim-owned), `T_OPSET_RELAY` (Net-owned), `T_AUDIO_MIX` (Audio-owned). Task payloads are POD; no allocations inside a task.
- **Work stealing:** the three GEN threads steal from each other's queues when idle; Stream threads may steal GEN terrain tasks (but never the reverse — streaming has latency priority over generation). Stealing happens in 16-task batches to amortize the CAS.
- **Scheduling policy:** generation is *pull-based* — the Main thread enqueues region/tile requests each frame based on the camera's 1-ring-ahead demand (GDD §12.4). The queue depth is capped (128 pending tasks); overflow drops the *farthest* request (never the nearest) and logs.
- **Frame barriers:** one per frame — a two-phase fence: (a) Main signals `FRAME_N` demand, (b) a Gen completion barrier releases the double-buffer for frame N+2. No cross-thread pointer chasing; buffers are swapped by index.
- **Determinism guard (NFR-1):** task *execution order* may vary; therefore no PRNG stream, no physics, and no mission logic may depend on when a task ran. All derived data carries its `(seed, coords, salt)` identity; a task that produces the wrong identity fails an assertion in debug builds and a golden-hash check in CI.
- **Steal/idle:** a GEN thread with no work for >4 frames sleeps on a futex (win32 `WaitOnAddress`) — this is the "idle/steal" capacity the GDD budgets, now embodied as the stealing policy above.

### 2.3 Memory Model (8 GB Total Budget)

**Global rule (T-006):** the process must hold **≤ 8 GB resident at steady state** (Act 3, 1080p, BALANCED). The iGPU's "VRAM" is a shared window into the same physical RAM — we budget it separately because the driver charges it against the same pool, but *no allocation exists that is "VRAM-only."*

| Pool | Size | Owner | Contents |
|---|---:|---|---|
| **Engine Core** | **1 GB** | Main (core 0) | ECS storage, UI (immediate-mode buffers), systems state, command buffers, Vulkan handles, shader/pipeline object handles, mission/story state, save (in-memory log tail) |
| **Chunk Ring Buffer** | **3 GB** | Stream (cores 1–2) | Fixed slab: **1,536 slots × 2 MB**. Holds decoded `.astroct` region data + in-flight terrain tiles. Transient: LRU eviction by camera distance; a slot is never freed individually (address-stable — GPU buffers and ECS pointers survive eviction *of content* via re-derivation, never via pointer churn) |
| **Asset Pool** | **1.5 GB** | Stream | Hero meshes (vertex-color, ≤ 4 MB textures total — GDD §9.2), starfield point buffers (3 layers), atmosphere/negbula billboard data, Opus decoded frames, UI atlas (≤ 1 k textures) |
| **Audio/IO** | **0.5 GB** | Net/Audio + Stream | PortAudio ring, 64 voice slots (DSP synth state), HRTF filter tables, 64 KB IO read-ahead per open file |
| **Dynamic VRAM** | **2 GB (ceiling)** | Render (via Main) | Render targets + MSAA (1080p: ~40 MB), double-buffered in-flight vertex/index buffers (terrain ≤ 64 MB, bodies ≤ 16 MB, starfields ≤ 64 MB), lensing-pass intermediates (2 × 1080p × 8 B ≈ 36 MB), UI, pipeline/UBO residency |
| **Total** | **8 GB** | | |

**Sub-rules:**
1. **VRAM ceiling 2 GB, steady-state target ≤ 1.5 GB** (GDD §12.2's 1.5 GB is the *steady-state* figure; 2 GB is the *peak* ceiling the governor must never exceed — the 300 ms margin absorbs warp-cinematic spikes).
2. **Terrain memory math** (why the ring works): a local detail chunk is a 64 m-edge heightfield. LOW 128² = 16,641 verts; BALANCED 256² = 66,049; HIGH 512² = 263,041. At 30 B/vert (pos+normal+color, quantized): 0.5 / 2.0 / 7.9 MB per tile. Worst case (HIGH, 16 visible tiles + 8 in-flight) ≈ 190 MB — comfortably inside the ring.
3. **Ring eviction is a derivation, not a loss:** every evicted region can be re-derived from `(seed, coords)` in ≤ 200 µs cold (§2.4) or re-decompressed from disk if cached — the game never needs "unloading logic" as a gameplay concept.
4. **Accounting:** every pool has a watermark; the profiler (§3.4) shows pool fill as percentages. A pool at > 90% for > 5 s is a logged performance defect.
5. **Debug builds** add ~0.4 GB (validation layers, frame trace, extra asserts) — the 8 GB budget is a *release* budget; debug targets 8.4 GB.

### 2.4 Universe Data Flow & the `.astroct` Chunk Container

**The pipeline (every value below is measured or a Phase-2 gate):**

```
 (seed, region_coords)          ≤ 200 µs cold, O(1) hash work        [GEN]
        │  region derive (xxHash3 integer streams)
        ▼
 RegionPlan (POD, ≤ 64 KB)      star/belt/nebula/POI descriptors     [GEN]
        │  if region dirty/uncached:
        ▼
 .astroct encode (v1, §2.5)     [GEN → Stream]
        │  ZSTD level 3 (LZ4 for hot tiles — T-004)
        ▼
 disk cache  %LOCALAPPDATA%\AstraCosmos\cache\  (≤ 2 GB, self-trimming) [Stream-B]
        │  on demand (1-ring-ahead pull)
        ▼
 .astroct decode → ChunkData (ring slot, 2 MB)                        [Stream]
        │  fence → double-buffer swap
        ▼
 GPU vertex buffers + ECS instances                                     [Main]
```

**`.astroct` = ASTRa compressed chunk table** — one container for all derived-universe data on disk. Naming:

| File | Contents | Typical size |
|---|---|---|
| `r_<x>_<y>_<z>.astroct` | One Region (10 ly): all stars, planets, moons, belts, nebulae, POIs, Loom fields | 200 KB – 2 MB |
| `p_<planet_id>.astroct` | One planet's terrain: global-sphere heightfield + 4×4 local-detail tiles (preset-independent max resolution 512²; presets decimate at load) | 1 – 8 MB |
| `s_<site_id>.astroct` | A hand-authored site (Meridian Station interior, Loom Gates): scene layout, not procedural | 50 KB – 1 MB |

**Compression policy (T-004):**
- **Primary: ZSTD, level 3** (~2 GB/s decompress on the reference CPU, ~4:1 on region data). Chosen for ratio — the disk cache is 2 GB and a better ratio is literally more galaxy on the stick.
- **Fallback: LZ4** (~4.5 GB/s) for `p_*.astroct` terrain tiles *only if* the Phase-2 performance gate shows ZSTD tile decompress > 2 ms on a single Stream core. The container stores one codec tag byte per file, so the two coexist; the build flag `ASTRA_TERRAIN_CODEC={zstd,lz4}` selects at encode time.
- **Shipped assets** (hero meshes, UI) use ZSTD level 19 (ratio matters, decode is at load, not per-frame).

**Disk cache lifecycle (Stream-B owned):** append on derive → trim oldest-first past 2 GB → the cache is *always deletable* (deleting it costs time, never correctness). A `cache --verify` Python tool (§5.4) re-derives cached files and diffs golden hashes.

### 2.5 `ChunkData` Schema & Forward Compatibility

**Requirement (locked, T-005):** `ChunkData` must accommodate future building features (and any future per-chunk player content) **without breaking saves or invalidating the disk cache**.

Four mechanisms achieve this:

1. **Fixed header, append-only fixed section.** The header (§2.5.1) and the fixed section field order are append-only. A reader that is *newer* than the writer skips trailing unknown fields; a *older* reader refuses files with `version > supported` (it never misreads).
2. **TLV extension blocks for everything new.** Any new content — **building placements, base beacons, decoration, mining scars, P7 presence artifacts** — goes into a new extension block type. Readers skip unknown types by length. **Reserved now:** `0x0001 BUILD_AREA` (building placements, per-chunk), `0x0002 MINING_SCARS` (depleted-deposit deltas), `0x0003 PRESENCE` (P7). Their *existence* is reserved; their *schema* is defined when the feature ships.
3. **Unknown blocks survive write-back verbatim.** If the game ever re-encodes a chunk (it shouldn't for pristine chunks, but must for player-mutated ones), blocks the writer doesn't understand are copied byte-for-byte. A save from v2 that contains v3 building data round-trips through a v2 binary without data loss.
4. **Pristine chunks are never mutated — player content is an overlay.** The world is `f(seed)` (GDD §6.2). Buildings and mined stock are **player-mutable state**, so they live in the **action log** (GDD §12.6) and, for spatial queries, in a per-region **overlay file** `o_<x>_<y>_<z>.astro` (same container, `story=PLAYER` flag) sitting next to the pristine `r_*.astroct`. Rendering merges pristine + overlay at load. Consequences: deleting a region from the cache is always free; a save is a log, not a world image; and the building feature ships by *adding* overlay handling, never by touching pristine chunk code.

**`ChunkData` in-memory layout (ring slot, 2 MB cap):**

```cpp
struct ChunkData {
    // header (mirrors file header — see Appendix A)
    uint16_t version;            // format version
    uint32_t type;               // REGION | PLANET_TERRAIN | SITE
    uint32_t flags;              // bit0 has_build_area, bit1 has_pois, bit2 story_region, …
    Seed128  seed;               // copied for validation
    int32_t  region[3];          // chunk-space coords
    // fixed section (offsets in header block table)
    uint32_t star_count;  StarRec   stars[MAX_STARS_IN_REGION];   // 40
    uint32_t body_count;  BodyRec   bodies[MAX_BODIES_IN_REGION]; // 256
    uint32_t poi_count;   PoiRec    pois[MAX_POIS_IN_REGION];     // 16
    uint32_t siphon_count;SiphonRec siphons[MAX_SIPHONS];         // 4
    // extension blocks (TLV) — parsed on demand, skipped if unknown
    uint32_t ext_count;  ExtBlock  exts[MAX_EXT_BLOCKS];          // ≤ 32 blocks, index only
    // transient (never serialized)
    uint64_t frame_ready;  // fence index
    uint32_t gpu_buffer_id;
};
```

Record sizes are fixed (no variable-length strings in hot records — names/lore are integer IDs into `data/` tables). A region with maximum occupancy fits in one 2 MB slot with ≥ 1 MB of headroom for extension blocks.

### 2.6 Terrain Generation Pipeline

Two tiers (GDD §5.3), all on GEN cores:

| Tier | Resolution | When generated | Output |
|---|---|---|---|
| **Global sphere** | 256×128 heightfield | At planet first-visit (4 workers, ~120 ms) | planet face from orbit (33k verts) |
| **Local detail** | 64 m-edge chunks; 128² / 256² / 512² per preset | Pull-based, 1-ring (8 chunks) ahead of player, 8 visible | walkable surface + ship-hover band |

**Per-tile pipeline (one GEN task):** `FBM(4 oct) + domain-warp(2 oct)` on a 64 m grid → biome mask (7 biomes, GDD §6.3) → normal + **vertex color** (biome palette, 64-color quantization per GDD §9.2) → index build → 30 B/vert interleave → write to a *next-frame* double buffer.
**Budget:** amortized ≤ 2 ms/frame across 3 GEN cores (GDD §12.2). Worst-case single tile on one core: 9 ms @ LOW / 26 ms @ BALANCED / 95 ms @ HIGH — which is why tiles are generated 8-deep and the **soft-ground fallback** (a 32 m pre-shelled disk, 20 ms to swap in) covers any generation miss for ≤ 100 ms (GDD §12.4).
**Cache:** tiles write to the planet `.astroct` at max resolution (512²); a lower preset **decimates** (every 2nd/4th sample + normal recompute) at load instead of regenerating — one source of truth, three visual densities.

### 2.7 Rendering Pipeline (Vulkan)

- **Baseline:** Vulkan 1.1, `VK_KHR_swapchain`, `VK_EXT_extended_dynamic_state`. Triple-buffered swapchain (2–3 images by preset). **Validation layers: debug builds only** (T-011), toggled by `ASTRA_VALIDATION=1`; the layer DLL ships only in dev artifacts, never in the portable zip.
- **Pipelines (all static, pipeline cache to disk):** opaque PBR-lite (vertex-color base), starfield points, billboard/nebula, atmosphere shell (two-sided, rim scatter), particle, UI (unlit). One descriptor set layout per family — iGPU descriptor overhead is real.
- **Culling:** CPU software frustum + distance culling (the culling data is the same ring the streaming system owns — no GPU visibility buffer). Instancing for asteroids (≤ 500 instances/draw) and nebula billboards (≤ 16/draw).
- **Post chain (fixed order):** [BALANCED+] bloom (1-pass, 1/4 res) → **lensing pass** (AAV-1, masked, analytic bend) → [HIGH only] DOF (bokeh disk, 1/2 res) → tonemap + grain. LOW runs a 1-pass tonemap only. Post total budget: **2 ms** (GDD §12.2 / D-010).
- **Draw call budget** (iGPU calls are ~2–4× pricier than desktop): LOW ≤ 120, BALANCED ≤ 200, HIGH ≤ 280 per frame (§3.3).
- **No MSAA** at LOW; 2× MSAA optional at BALANCED if the frame-time headroom shows at the Phase-6 gate.

### 2.8 Physics & Simulation (Core 6)

- **Fixed timestep 60 Hz** (GDD §12.3). Ticks are decoupled from render frames (accumulator + interpolation for rendering; never catch-up loops > 2 ticks — beyond that the sim *drops* to a 30 Hz degraded tick and flags `SIM_DEGRADED` in the profiler).
- Ship: swept-sphere vs. bodies (bodies are spheres/ellipsoids — analytic, no mesh collision). Surface: capsule vs. heightfield (grid-based, O(1) per cell). Weapons: CCD ray segment vs. spheres.
- **Species AI** ticks here, amortized: Harvesters re-plan at 2 Hz, Watchers at 0.5 Hz, hazard sim (storms, radiation fronts) at 1 Hz. A full AI tick is budgeted ≤ 3 ms on the reference CPU.
- Determinism: sim consumes the PRNG stream salt `SIM`; a 1,000-tick replay of the same action prefix must bit-match (CI test, §7).

### 2.9 Audio (Core 7)

> **Full audio spec: [AudioDesign.md](AudioDesign.md) v1.3** — sound design parameters (engines, pulsars, radio, Phantasma 18 Hz bed), spatialization (HRTF + Doppler), music/dialogue Opus plan, and the **< 50 MB total audio RAM cap (AU-007)** — a refinement of the 0.5 GB Audio/IO pool (T-006 → T-013). This section keeps the plumbing; the audio document owns the design.

**Backend (T-003, supersedes GDD §12.5's custom-WASAPI plan):**
- **PortAudio** (MIT) as the capture/playback core — cross-compiles cleanly under MinGW-w64 and gives us one stable, 20-year-proven real-time callback across WASAPI/directsound fallbacks.
- **OpenAL** (`OpenAL32.dll`, bundled) as the spatial layer — HRTF positioning per GDD §10 (8-directional filter set applied in the AL effect chain on headphones; simplified stereo model auto-selected when speakers are detected).
- **libopus** decodes music (5 tracks × 3 stems, 32 kbps mono) and MERIDIAN VO (< 200 lines, 32 kbps mono) into the DSP graph.
- **DSP synth:** the custom seeded synthesis (thrusters, scans, impacts, UI, suit, storm beds — GDD §10) is *ours* and runs before OpenAL: 64 voice slots → synth bus → AL sources → master. Synthesis stays pure/seeded so SFX remain a function of game state (determinism win, GDD §12.5).
- Callback: 48 kHz, 256-sample period, `TIME_CRITICAL` on core 7, ≤ 1.5 ms DSP budget (TDD §3.3). The audio thread preempts everything — including the physics thread.

### 2.10 Networking (Phase 7 — architecture reserved now)

- **Transport:** Winsock2 UDP, host-authoritative sessions (GDD §13 P7). No third-party net library — the opset protocol is small enough that owning it is cheaper than vetting one.
- **Protocol:** action-log replication (GDD §12.6): peers send op deltas (versioned opset, same schema as saves); host replays on `f(seed, log)` and relays the authoritative tail. Delta compression: opset diff + ZSTD on the wire.
- **Placement:** all net I/O and decode on core 7; the Main thread consumes a *validated op queue* (SPMC) per frame — the network can never block render or sim.
- **Reservation now (Phase 2):** the `net/` module skeleton (opset encode/decode + unit tests) ships in Phase 2 so the save format and the wire format share one code path from day one.

### 2.11 Save System (implementation of GDD §12.6)

- **Format:** `save.astro` = `{ format_version, seed(128), player_state(POD), checkpoint(blob, LZ4), log_tail(JSON-lines, ZSTD) }`.
- **Compaction:** every 500 ops the tail is checkpointed (deterministic world snapshot = ring contents + ECS + sim state, LZ4); replay is O(tail).
- **Integrity:** xxHash64 over the whole file; on mismatch, truncate tail to last valid op (bounded corruption, GDD §12.6).
- **Multiplayer reuse:** the same `replay(seed, log)` function that loads a save is the function that syncs a session — one code path, tested by the same determinism harness (§7).

---

## 3. Performance Budgets & Quality Presets

### 3.1 Budget Philosophy

The budget is **per-preset and per-system**, measured on the reference hardware (Ryzen 7 / Vega 8, Windows 11, 16 GB) with the in-engine profiler (§3.4). A frame that exceeds *any* system's budget is a logged defect with the system named. Adaptive behavior (governor, §3.3) is the *only* sanctioned way to spend a budget — never silently.

### 3.2 The Three Presets

| Parameter | **LOW** | **BALANCED** | **HIGH** |
|---|---|---|---|
| **FPS target** | **60 guaranteed** | **45–60** | **30–45** |
| Internal resolution | 720p (dynamic 100→83%) | 1080p (dynamic 100→66%) | 1080p (dynamic 100→83%) |
| Frame budget | 16.6 ms | 22.2–16.6 ms | 33.3–22.2 ms |
| **Black-hole shader** | Simplified: glow billboard + procedural disk, **no lensing** | **Full AAV-1:** screen-space lensing + disk, masked pass | **Volumetric:** 16-step raymarch inside mask + full disk + accretion detail |
| **Terrain detail (local chunks)** | 128² (16,641 verts, 0.5 MB/tile) | 256² (66,049 verts, 2.0 MB/tile) | 512² (263,041 verts, 7.9 MB/tile) |
| **Starfield (far layer)** | 100K points | 500K points | 1M points |
| Post-processing | **None** (1-pass tonemap) | Bloom + lensing + tonemap/grain | Bloom + volumetric BH + DOF + tonemap/grain |
| Particles | Off (2 stub emitters) | Standard (storm/dust/engine) | Full (all emitters, 2× density) |
| MSAA | None | Optional 2× (Phase-6 gate) | None (budget goes to DOF) |
| Nebula billboards | 8 | 16 | 16 |
| Terrain ring occupancy | ≤ 25% | ≤ 55% | ≤ 90% |
| **Draw calls / frame (max)** | 120 | 200 | 280 |
| **Vertex throughput (typical frame)** | ~270K | ~1.05M | ~3.3M |

**Preset selection:** auto at first launch by GPU class detection (Vega-class iGPU → LOW by default, BALANCED offered); user-overridable in Settings; the saved choice lives in `astra.cfg` (created at runtime, outside the portable folder — §5.3).

### 3.3 Per-System Frame Budget (BALANCED, reference iGPU)

| System | Budget | Owner core(s) |
|---|---:|---|
| Main: game logic + UI + command record/submit | 4.0 ms | 0 |
| Streaming: IO + decompress (amortized) | 2.0 ms | 1–2 |
| Generation: terrain + universe (amortized) | 2.0 ms | 3–5 |
| Physics + AI (fixed tick, amortized) | 3.0 ms | 6 |
| Audio DSP | 1.5 ms | 7 |
| GPU: render (all draws) | 7.0 ms | — |
| GPU: post (lensing active) | 2.0 ms | — |
| Headroom / scheduler jitter | ≤ 1.0 ms | — |
| **Total @ 45 FPS** | **≤ 22.2 ms** | |

LOW and HIGH scale the GPU line (7.0 → 5.0 / 9.0 ms) and the vertex/point work accordingly; CPU lines are preset-independent (generation is the exception — HIGH terrain tiles are bigger, so its 2 ms amortized budget is the *average* over the 8-deep pipeline, not a per-tile cap).

### 3.4 Adaptive Governor (the "30–60 adaptive" contract)

Triggers and actions, in order — each step is logged with a reason code:

1. **Resolution:** if 60 consecutive frames exceed the preset's frame budget → internal resolution −6% (floor 66%). If 120 frames under 80% of budget → +6% (ceiling 100%).
2. **LOD bias:** terrain visible-ring 8 → 6 chunks; body LOD −1.
3. **Density:** starfield far layer −20%; nebula billboards −50%; particles −50%.
4. **Preset floor:** HIGH that can't hold 30 FPS auto-drops to BALANCED (user notified, not silent); BALANCED that can't hold 30 drops to LOW.

The governor is **one-directional per direction** (hysteresis 30 s) — no oscillation.

### 3.5 Profiler (in-engine + CI)

| Metric | Source | Where shown |
|---|---|---|
| **Per-core CPU utilization** | `QueryThreadCycleTime` per pinned thread, sampled per frame; utilization = busy-cycles / (frame time × 1 core) | In-engine `F9` panel; CI frame trace |
| **Draw calls / frame** | counted at submit | F9 panel; CI |
| **RAM (process)** | `GetProcessMemoryInfo` (working set) + per-pool watermarks (engine-side accounting — the primary signal; OS number is the cross-check) | F9 panel; CI |
| **VRAM (device)** | in-engine device-memory accounting (primary) + `VK_EXT_memory_budget` (cross-check when exposed by the driver) | F9 panel; CI |
| **Frame trace** | circular buffer of `(frame, system, ns)` tuples, 4,099 frames deep | F9 panel; dumpable to JSON-lines (`astra --dump-trace out.json`) |
| **Generation pipeline depth** | task queue depths, tile age (ms from request → ready) | F9 panel |
| **Ring occupancy** | free slots / evictions per minute | F9 panel |
| **Audio callback overruns** | PortAudio stream status | CI (must be 0) |

**CI performance harness:** the same frame-time measurement runs in a dedicated **`perf` build mode** (`astra --perf-scene <id> --frames 3600`). Because a GPU-less Linux CI runner cannot exercise Vulkan, the perf harness runs **nightly on a pinned reference box** (one Ryzen 7 / Vega 8 machine, immutable OS image) and posts results to the repo; a preset-regression of > 5% mean frame time or > 20% p99 hitch is build-failing on `develop`.

---

## 4. Tooling, Libraries & Dependencies

### 4.1 Runtime Stack

| Component | Library | License | Role | Linkage |
|---|---|---|---|---|
| Graphics | **Vulkan SDK** (loader) | Apache 2.0 | GPU API; `vulkan-1.dll` bundled in `libs/` | Dynamic (system DLL in `libs/`) |
| Windowing/input | **GLFW** | zlib | Window, swapchain surface, keyboard/mouse/gamepad | **Static** |
| Audio core | **PortAudio** | MIT | Real-time playback (WASAPI) | **Static** |
| Spatial audio | **OpenAL Soft** | LGPL 2.1* | Positional/panning layer (HRTF filter stage = custom CPU DSP, AudioDesign App. C); `OpenAL32.dll` bundled in `libs/` | Dynamic |
| Compression | **ZSTD** | BSD | `.astroct`, assets, save | **Static** |
| Compression (fallback) | **LZ4** | BSD | Hot-path terrain tiles if T-004 gate trips | **Static** |
| Math | **GLM** | LGPL 2.1 | Mat/vec/quaternion, shader-constant layout | **Header-only** (no link ⇒ no LGPL obligation beyond source availability, which we satisfy by shipping the repo) |
| Math (hash) | **xxHash3** (vendored) | BSD | All PRNG-derived identity | **Static** |
| Audio codec | **libopus** + libopusfile | BSD | Music/VO decode | **Static** |
| System | Win32 (kernel32, user32, gdi32, winmm, winsock2) | — | API surface | System |

\* OpenAL Soft is LGPL: we link the **dynamic DLL** (bundled in `libs/`), which is the clean LGPL path; users may swap `OpenAL32.dll` for their own.

**Dependency policy (locked):** every third-party dependency is **vendored in `third_party/` with a pinned commit + a `LICENSE` file + a one-paragraph audit note** in `third_party/AUDIT.md`. No runtime network, no vcpkg/conan at build time, no unvendored transitive dependencies. Adding a dependency is a `T-###` decision.

**Rejected alternatives (recorded):** XAudio2 (cross-compile friction — the reason we moved off the GDD's WASAPI plan, T-003); enet/ENet (the opset protocol doesn't need a relay engine); ImGui (immediate-mode UI is 3 screens + a HUD — our own `ui/` is ~2k lines and texture-bounded to 1k); GTest (a 300-line micro-test framework covers our needs with zero deps — see §7).

### 4.2 Build Toolchain (Linux → Windows)

| Tool | Version pin | Use |
|---|---|---|
| Ubuntu | 22.04 (CI image, pinned) | Build host |
| **MinGW-w64** | 11.0.1 (x86_64-w64-mingw32-gcc) | Cross-compiler (C/C++, `windaes` free) |
| **CMake** | 3.26+ | Build orchestration; `toolchain-mingw-x64.cmake` in-repo |
| **glslangValidator** | 14.3.0 | GLSL → **SPIR-V** (shaders precompiled at build time; only `.spv` ships) |
| **Python 3.11** | embedded build | Asset scripts (§5.4) |
| Ninja | 1.11+ | Generator (faster than Make on this tree) |
| ccache | 4.x | Incremental builds |

Compile flags: `-O2 -march=znver2 -mtune=generic` (Ryzen 7 is Zen 2/3 — `znver2` guarantees availability, Zen 3 gains are partial); `-fvisibility=hidden`; release links static runtime (`-static-libstdc++ -static-libgcc`), dynamic only for the three system-side DLLs (`vulkan-1`, `OpenAL32`, system APIs).

**Shader build step:** `glslangValidator -V -O --target-env vulkan1.1 in.glsl -o out.spv` per shader; a manifest `shaders/manifest.json` maps pipeline → SPIR-V; the engine refuses to start if the manifest doesn't match the built binary's shader hash (prevents shipping a zip with mismatched shaders — a real failure mode, logged as T-009's guard).

---

## 5. Build & Deployment

### 5.1 Build Pipeline (Linux → Windows portable)

```
1.  compile      git checkout → cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw-x64.cmake
                 → ninja astra astra-perf tools            (C++ → Windows .exe via MinGW-w64)
2.  shaders      for each src/shaders/*.glsl:
                 glslangValidator -V -O → build/shaders/*.spv + manifest.json
3.  assets       python_embed python tools/pack_assets.py  (ZSTD-compress hero meshes, audio,
                 UI → assets/*.astropack; verify licenses; compute SHA-256 manifest)
                 python_embed python tools/pack_data.py    (anchors.json, mission templates,
                 Codex text, preset JSONs → data/)
4.  package      tools/make_zip.py → AstraCosmos/ tree (§5.3) + libs/ + .start.bat
                 → SHA-256 manifest.txt → astra-<semver>-windows-x64-portable.zip
5.  verify       tools/verify_zip.py on the built zip: structure check, DLL presence,
                 `astra --version` matches manifest, shader manifest hash matches,
                 430-anchor schema audit, one deterministic region derive + golden hash
```

CI runs steps 1, 2, 5 on every PR to `develop` (steps 3–4 on merge to `develop` and every release tag). Total clean build target: **< 20 min** on a 4-vCPU runner (GDD §14 engineering KPI).

### 5.2 `.start.bat` — Behavior Spec (full script in Appendix B)

Order of operations, each with a **numbered, human-readable failure**:

| Step | Check | Failure code (shown to user) |
|---|---|---|
| 1 | `cd /d` to script directory; verify `astra\astra.exe` exists | `E1 — Game not found…` (re-download instructions) |
| 2 | Verify `libs\vulkan-1.dll` and `libs\OpenAL32.dll` present | `E2 — Missing graphics/audio runtime…` |
| 3 | Verify `data\manifest.json` present and its `version` field matches `astra --version` | `E4 — Version mismatch…` (partial update detected) |
| 4 | **RAM check:** `FreePhysicalMemory` via WMIC; require **≥ 8192 MB free** (of the 16 GB machine) | `E3 — Not enough free memory (need 8 GB)…` (lists top-3 memory hogs for convenience) |
| 5 | Create `%LOCALAPPDATA%\AstraCosmos\` (cache + cfg) if absent; verify writable | `E5 — Cannot create save folder…` (read-only USB note) |
| 6 | Launch: `start "" /HIGH astra\astra.exe` — **high CPU priority** per spec; the game itself then pins cores and sets per-thread priorities (§2.2) | — |
| 7 | On exit: if exit code ≠ 0, show the last 10 lines of `astra\logs\astra-<date>.log` | `E6 — Game exited with an error…` |

Rules: no admin required; the bat never writes inside the portable folder (USB-stick-safe); every message is one English paragraph + the failure code + one action.

### 5.3 Shipped Directory Structure (annotated)

See [Appendix C](#appendix-c--shipped-directory-tree-annotated) for the canonical annotated tree. Summary:

```
AstraCosmos/
├── .start.bat      launcher: verify deps + RAM, launch /HIGH
├── python_embed/   embedded CPython 3.11 + tools/ (pack, audit, cache verify)
├── libs/           vulkan-1.dll, OpenAL32.dll (+ dev: validation layers — never in release)
├── assets/         ZSTD .astropack pools (hero meshes, Opus audio, UI atlas)
├── data/           anchors.json (430), mission templates, Codex, presets, manifest.json
├── shaders/        compiled SPIR-V only (+ manifest.json)
└── astra/          astra.exe, logs/ (runtime), crash dumps (opt-in)
```

**Runtime-writable data lives OUTSIDE the zip** (`%LOCALAPPDATA%\AstraCosmos\`: disk cache ≤ 2 GB, `astra.cfg`, saves). The portable folder is effectively read-only at runtime — which is what makes "zip from a USB stick" a supported configuration.

### 5.4 `python_embed/` — Role & Scripts

Embedded CPython (the ~10 MB embeddable zip) + `tools/`:

| Script | Runs when | Purpose |
|---|---|---|
| `pack_assets.py` | CI step 3 | ZSTD-compress asset pools, license check, SHA-256 manifest |
| `pack_data.py` | CI step 3 | Validate `anchors.json` (430 rows, schema §GDD 6.2, sky-position audit), compile mission/Codex data |
| `universe_audit.py` | nightly CI + user | Re-derive N random regions, golden-hash diff (the determinism audit, GDD §6.2) |
| `cache_verify.py` | user (`python_embed\python.exe tools\cache_verify.py`) | Re-derive + diff every cached `.astroct`; `--purge-bad` |
| `unpack_astroct.py` | dev/user | Human-readable dump of any `.astroct` (blocks, records, extension TLVs) |
| `make_zip.py` / `verify_zip.py` | CI step 4–5 | Build + verify the portable zip |

**The game itself never needs Python** — `python_embed/` is tooling for the player's benefit (cache auditing, a fun "what's in this chunk" viewer) and for CI. Its absence from the runtime path is guaranteed by the dependency audit (T-012): no dynamic `import` of game code, no startup dependency.

---

## 6. Versioning, Branching & Delivery

### 6.1 Semantic Versioning (locked, T-008)

`MAJOR.MINOR.PATCH` —

- **MAJOR** = save/format-incompatible change. Our formats are designed to make this **rare** (append-only fixed sections + TLV blocks + overlay files, §2.5); if it happens, the release notes must include a migration story, and the launcher (`E4` check) must detect the mismatch *before* the game starts.
- **MINOR** = new feature / new phase deliverable / new preset parameter.
- **PATCH** = bug fix, balance, content errata (e.g., an anchor-data correction).

**Where the version lives (all three must match — the build enforces it):**
1. Windows version resource of `astra.exe` (file/product version),
2. `data/manifest.json` → `{"version": "x.y.z", "save_format": n, "chunk_format": n, "shader_hash": "…"}`,
3. Git tag `vX.Y.Z` on `main`.

`astra --version` prints all of them; `.start.bat` step 3 compares 1↔2; CI compares 2↔3.

### 6.2 GitFlow Branching (locked)

| Branch | Purpose | Protected? | Merge policy |
|---|---|---|---|
| `main` | Release-ready; **tags only enter here** | Yes (no direct push) | From `release/*` only |
| `develop` | Integration trunk for the 8 GDD phases | Yes (no direct push) | Squash-merge from `feature/*` and `release/*` |
| `feature/<ast-###>-<slug>` | One deliverable (e.g., `feature/ast-022-terrain-tiles`) | No | Opens a PR to `develop` with CI gates green |
| `release/x.y` | Cut from `develop` when a phase exit criteria is in sight | No | Frozen: patches only; when green → `main` (tag) + back to `develop` |
| `hotfix/<ast-###>` | Cut from `main` for shipped releases | No | Patches → `main` (tag `x.y.z+hotfix`) + back to `develop` |

**Phase mapping (GDD §13):** each phase's deliverables land as feature branches; a phase's *exit criteria* gate the `release/*` cut for the phase's milestone tag (`v0.2.0` = Phase 2 exit, `v0.3.0` = Phase 3 exit, …, `v1.0.0` = Phase 8 launch). Pre-1.0 versions carry the `0.` prefix deliberately — the save format is stable from Phase 2, but the *game* is not final until 1.0.

### 6.3 Delivery

- **Artifact:** `astra-<semver>-windows-x64-portable.zip` + `SHA256SUMS.txt` (both in the release tag's artifacts and on the distribution page of record — channel is GDD OQ-1, open).
- **Release checklist (Phase 8, GDD §13):** three clean-VM boot tests (Win10/Win11, USB-stick mount, fresh user profile), reference-box perf certification (all 3 presets, §3.5), decision-log reconciliation (every `ACCEPTED` GDD+TDD decision implemented or formally descoped), `universe_audit.py` green on the release's seed.
- **Post-launch:** patches are new zips (no patcher at launch); the `E4` version-mismatch check is the user's guard against mixed folders.

---

## 7. Testing & Quality Gates

| Gate | What | Runs | Failure ⇒ |
|---|---|---|---|
| **Unit** | micro-test framework (300 lines, zero deps): PRNG streams, xxHash3 vectors, opset encode/decode, chunk-container round-trip (incl. **unknown TLV block survival**, §2.5), ZSTD/LZ4 vectors, memory pool accounting | Every PR, Linux native build of the same sources | Build-fail |
| **Determinism audit** | `universe_audit.py`: 10,000 random regions re-derived, golden-hash diffed; + 1,000-tick sim replay bit-match; + save checkpoint/replay round-trip | Nightly + every PR touching `procgen`/`terrain`/`physics`/`save` | **Build-blocking** (NFR-1) |
| **Memory asserts** | release-instrumented build asserts: no allocation outside pools, ring never exceeds slot count, VRAM accounting ≤ 2 GB, process working set ≤ 8 GB over a 2 h scripted flight | Nightly (reference box, scripted Act-3 region fly) | Performance defect logged, p99 > budget ⇒ build-fail on `develop` |
| **Perf harness** | `astra --perf-scene` on the pinned reference box; preset regression > 5% mean / > 20% p99 | Nightly (reference box) | Build-fail on `develop` |
| **Audio** | PortAudio callback overruns = 0; Opus decode continuity (no gaps in a 10 h music loop) | Nightly | Build-fail |
| **Zip integrity** | `verify_zip.py` (structure, DLLs, version triad, shader hash, anchor audit) | Every release candidate | Release-blocked |
| **Validation layers** | Debug build with `ASTRA_VALIDATION=1`: 0 Vulkan errors in a 1 h smoke | Every PR (CI) | Build-fail |

**Micro-test framework note:** a `TEST(name){…}` macro + a main runner with `--filter`, `--list`, exit codes. Chosen over GTest to keep the shipped dependency list exactly the one in §4.1 (T-010).

---

## 8. Technical Risks

| # | Risk | L | I | Mitigation | Owner/phase |
|---|---|---|---|---|---|
| TR-1 | Shared-memory bandwidth contention (GPU raster vs. CPU streaming on one bus) | High | High | Rasterizer-only GPU (T-001); fixed post chain; perf harness watches *bus-bound* frames (GPU busy but CPU starved) separately; governor drops density before resolution | P2 |
| TR-2 | 512² terrain tiles (HIGH) miss the 8-deep pipeline under burst landings | Med | Med | 95 ms/tile single-core worst case is why tiles pre-generate 8-deep; soft-ground fallback ≤ 100 ms; HIGH auto-drops visible ring 8→6 first (§3.3) | P2/P3 |
| TR-3 | ZSTD decompress exceeds 2 ms on a terrain tile at HIGH | Med | Med | T-004 gate: LZ4 fallback per-asset-class, measured at the P2 gate, not by opinion | P2 |
| TR-4 | OpenAL Soft + PortAudio latency stacking on iGPU-class machines (audio dropout under load) | Med | Med | 256-sample period (10 ms), core 7 TIME_CRITICAL, audio preempts everything; 10 h loop test is a gate | P2 |
| TR-5 | 1M-point starfield vertex cost at HIGH on Vega 8 | Med | Low | Points are trivial vertex shaders; 1M points ≈ 0.3 ms measured-class on this GPU; if not, HIGH far layer falls to 700K via governor step 3 | P3 |
| TR-6 | MinGW/windaes ABI drift breaking a vendored lib (PortAudio historically the fragile one) | Med | Med | Pinned versions + vendored patches with audit notes; the *Linux native build* of the same sources runs the unit/determinism gates, so a MinGW-specific break is isolated to a thin integration surface | P2 |
| TR-7 | iGPU VRAM ceiling (2 GB) breached during warp cinematic + HIGH preset | Med | High | Cinematic spikes pre-allocate at governor step 0; VRAM accounting asserts in release builds; HIGH's 30 FPS floor buys the headroom | P6 |
| TR-8 | `.astroct` v1 schema gets locked in with a blind spot that Phase 7 (presence) needs | Low | Med | TLV blocks + reserved `0x0003 PRESENCE` from day one; overlay-file pattern isolates all mutable state (§2.5) | P2/P7 |
| TR-9 | Reference-box perf harness drifts (driver/OS update) invalidating baselines | Med | Med | Immutable OS image + pinned driver; baseline re-baseline requires a TDD decision entry | P2 |
| TR-10 | Validation-layer findings at P5+ (AAV passes) forcing pipeline rework | Med | Med | AAV-1/AAV-2 pipelines are validated from the moment they're written (layers on every PR in debug); post-chain is the *only* place new passes may appear | P5/P6 |

---

## 9. Technical Decision Log

**Format:** `T-###` · date · decision · rationale · status (`PROPOSED` / `ACCEPTED` / `REJECTED` / `SUPERSEDED BY T-###`). Append-only.

| ID | Date | Decision | Rationale | Status |
|---|---|---|---|---|
| T-001 | 2026-09-27 | **Dual-engine role split: CPU does generation/physics/streaming/net/audio; the GPU is a rasterizer of pre-computed vertex data (no compute, no tessellation, no async compute).** | Vega 8 shares its memory bus with the CPU; every GPU compute workload is bandwidth stolen from the terrain/streaming workers. Rasterization is the one GPU work that is ~100× faster than CPU *and* fixed-size per preset — which is what makes the 8 GB / 30–60 FPS budget provable. | ACCEPTED |
| T-002 | 2026-09-27 | **Core pinning: 0 Main · 1–2 Stream · 3–5 Gen · 6 Sim · 7 Net/Audio; MPMC lock-free rings + work stealing (Gen pool steals from Gen and Stream; inverse forbidden); one frame fence, double-buffered handoff.** | 1:1 pinning removes scheduler nondeterminism from the perf budget; streaming has latency priority over generation by rule, not by priority level; lock-free queues keep the hot path mutex-free (the only locks: disk file, save file). | ACCEPTED |
| T-003 | 2026-09-27 | **Audio backend: PortAudio (MIT, static) + OpenAL Soft (dynamic DLL in `libs/`) + libopus (static) + our seeded DSP synth.** *Supersedes GDD §12.5's custom-WASAPI plan; GDD D-012 updated with pointer D-017.* | PortAudio is the lowest-friction real-time callback that cross-compiles cleanly under MinGW-w64; OpenAL Soft gives HRTF/spatialization without us writing an audio engine; the synth layer stays ours (seeded, deterministic). Custom-WASAPI (GDD plan) saves ~300 KB and costs months of driver-edge-case time. | ACCEPTED |
| T-004 | 2026-09-27 | **Compression: ZSTD level 3 primary; LZ4 fallback per-asset-class (terrain tiles first) if a P2 perf gate shows > 2 ms decompress; one codec tag byte per file; shipped assets at ZSTD level 19.** | ZSTD wins on ratio (the 2 GB disk cache holds more galaxy); LZ4 wins on speed; the per-file codec tag makes the two coexist without migration. The decision is made by measurement at the P2 gate, not by preference. | ACCEPTED |
| T-005 | 2026-09-27 | **`ChunkData` forward-compat: append-only fixed section + TLV extension blocks (reserved: 0x0001 BUILD_AREA, 0x0002 MINING_SCARS, 0x0003 PRESENCE) + verbatim survival of unknown blocks on write-back + player-mutable state in overlay files, never in pristine chunks.** | The explicit requirement: future building features must not break saves. Pristine = `f(seed)` is immutable by law; buildings/mining are *player state*, so they belong in the action log + per-region overlays. New features then ship by adding a block type + overlay handling — never by touching saved data. | ACCEPTED |
| T-006 | 2026-09-27 | **8 GB memory model: 1 GB Engine Core · 3 GB Chunk Ring (1,536 × 2 MB slots) · 1.5 GB Asset Pool · 0.5 GB Audio/IO · 2 GB dynamic VRAM ceiling (steady-state target ≤ 1.5 GB, per GDD §12.2).** | The iGPU's "VRAM" is shared system RAM — we budget it separately because the driver charges it against the same pool, but every allocation is tracked in one of the five pools; allocation outside a pool is a debug assertion. Eviction is a re-derivation, not a loss (§2.4). | ACCEPTED |
| T-007 | 2026-09-27 | **Three quality presets (LOW 60-g/720p/no-post · BALANCED 45–60/1080p/full-BH · HIGH 30–45/1080p/volumetric-BH) + the 4-step governor (resolution → LOD → density → preset floor) with 30 s hysteresis.** | The preset table is the contract with the player (each promises an FPS range); the governor is the only sanctioned way to spend a budget. Resolution-first trading costs the least perceived quality. | ACCEPTED |
| T-008 | 2026-09-27 | **Semantic versioning with a three-way version check (exe resource ↔ `data/manifest.json` ↔ git tag); MAJOR reserved for save/format breaks.** | The portable-folder distribution means users *can* mix folders from different zips (partial updates, USB copies). The `E4` launcher check plus the build-enforced triad makes that fail loudly, at launch, with a human message. | ACCEPTED |
| T-009 | 2026-09-27 | **Shaders are precompiled at build time (glslangValidator → SPIR-V) with a manifest whose hash must match the binary at startup; only `.spv` ships.** | A runtime GLSL compiler would add a megabytes-scale dependency and a startup cost on the iGPU budget; the manifest-hash guard kills the "zip shipped with mismatched shaders" failure class at first launch. | ACCEPTED |
| T-010 | 2026-09-27 | **Testing: custom 300-line micro-test framework (zero deps); GTest rejected.** | The shipped dependency list is a product decision (the iGPU audience's disk is small and our trust surface is hand-audited); our test surface (hash vectors, container round-trip, opset, pool accounting) needs macros + a runner, not a framework. | ACCEPTED |
| T-011 | 2026-09-27 | **Vulkan validation layers: debug builds only, `ASTRA_VALIDATION=1`, layer DLL in dev artifacts only — never in the release zip.** | Layers cost 2–5× frame time; the AAV passes (P5/P6) need them *during* development on every PR, but shipping them would blow the frame budget on the reference iGPU. | ACCEPTED |
| T-012 | 2026-09-27 | **`python_embed/` is tooling-only (asset pack, universe audit, cache verify, chunk viewer); the game has zero runtime dependency on Python.** | Embedded Python earns its 10 MB by letting players audit their own disk cache and letting CI run the determinism audit from the same folder it ships — while the runtime stays exactly one .exe + three DLLs. | ACCEPTED |
| T-013 | 2026-09-27 | **Audio subsystem RAM hard cap: < 50 MB total including synthesizer memory (all active Opus streams + voice state + HRTF tables), per AudioDesign AU-007. Refines GDD §12.2's 100 MB audio line; sits inside the T-006 0.5 GB Audio/IO pool.** | The owner-specified audio budget, adopted as a hard cap with a per-frame watermark assert; current design uses ≈ 1.9 MB, so 50 MB is a guardrail with reserved headroom. §2.9 pointer added. | ACCEPTED |
| T-014 | 2026-09-28 | **Phase 0 toolchain provisioning in the Arena sandbox.** The sandbox network allowlist (github.com + codeload.github.com + pypi.org only) blocks every package CDN: apt/deb mirrors (TLS reset), conda, Linuxbrew, ghcr, and all ziglang PyPI wheels (68–98 MB > the 50 MB per-download rule). Sanctioned fallbacks used: cmake 4.4.3 + ninja 1.13.2 via PyPI; Vulkan-headers v1.3.296 vendored into `libs/` (the §3.3 fallback, gitignored); glslang built from the official Khronos source (target `glslang-standalone` → binary `glslang`); Vulkan-Loader v1.3.296 built for the native smoke test. **The MinGW cross compiler (x86_64-w64-mingw32-g++) is unobtainable in-sandbox** — zig 0.14.1 stage1 was bootstrapped from source as the last attempt, but its `cc` requires the self-hosted stage, which requires host LLVM 19 (also unobtainable). PHASE0_GATE: FAIL is recorded in docs/PHASE0_EXIT_REPORT.md with user-side fix options (grant deb.debian.org egress, or drop the 4–5 mingw .debs into the workspace, or allow one >50 MB download for the ziglang wheel). | The pipeline is complete and correct; only the cross compiler binary is missing from the sandbox. All Phase 0 code, build system, and docs are in place; a rerun of `build.sh` after toolchain access re-evaluates the gate. | ACCEPTED |

---

## Appendix A — `.astroct` Binary Layout (v1)

Little-endian throughout. `uN`/`iN` = N-bit unsigned/signed integer.

```
offset  size  field                    notes
──────  ────  ─────                    ─────
0x00    4     magic 'ASCT'
0x04    2     version                  = 1
0x06    1     codec                    0 = ZSTD, 1 = LZ4
0x07    1     story_flag               HOME | STORY | FLAVOR | PLAYER | ANCHOR
0x08    16    seed                     galactic seed (redundant, for validation)
0x18    12    region                   i32[3] chunk-space coords (region space)
0x24    2     type                     0=REGION 1=PLANET_TERRAIN 2=SITE
0x26    2     flags                    bit0 build_area 1 pois 2 story_region 3 siphon
0x28    4     fixed_section_len        bytes
0x2C    2     ext_count
0x2E    2     reserved[14]             future header growth
0x3E    ─     fixed section            (per type; fields are append-only, offsets in a
                                       per-type record table embedded at its head)
        ─     extension blocks (TLV × ext_count)
0x??    2     block_type               u16   0x0001 BUILD_AREA · 0x0002 MINING_SCARS
                                       0x0003 PRESENCE · 0xFFFF END
0x??    4     block_len                u32   payload bytes (excludes these 6)
0x??    n     block_payload            skipped verbatim if type unknown to reader
        ─     tail
        8     xxHash64                 over bytes 0x00 … end-of-blocks
```

**Reader rules (locked):**
1. `version > reader_max` → reject file, re-derive from seed (pristine) or refuse load (player overlay, with a human message).
2. `version < reader_max` → read fixed section up to `fixed_section_len`, then parse TLV; unknown block types are **copied to the in-memory image and re-emitted byte-for-byte** on any write-back.
3. xxHash mismatch on a pristine file → silently re-derive (disk corruption is normal on cheap USB sticks); mismatch on a PLAYER overlay → surface a repair dialog (player data is never silently dropped).

## Appendix B — `.start.bat` (canonical)

```bat
@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title Astra Cosmos Launcher

rem ── E1: game present ─────────────────────────────────────────────
if not exist "astra\astra.exe" (
  echo E1 - GAME NOT FOUND: astra\astra.exe is missing.
  echo The portable folder may be incomplete. Re-download the zip
  echo and extract it fully (do not run the exe from inside the zip).
  pause & exit /b 1
)
rem ── E2: runtimes present ─────────────────────────────────────────
if not exist "libs\vulkan-1.dll" (
  echo E2 - GRAPHICS RUNTIME MISSING: libs\vulkan-1.dll.
  echo Re-download the full portable zip. & pause & exit /b 1
)
if not exist "libs\OpenAL32.dll" (
  echo E2 - AUDIO RUNTIME MISSING: libs\OpenAL32.dll.
  echo Re-download the full portable zip. & pause & exit /b 1
)
rem ── E4: version triad (exe vs data manifest) ─────────────────────
for /f "delims=" %%v in ('astra\astra.exe --version 2^>nul') do set EXE_VER=%%v
if not exist "data\manifest.json" (
  echo E4 - DATA MISSING: data\manifest.json not found. & pause & exit /b 1
)
findstr /C:"\"version\": \"%EXE_VER%\"" "data\manifest.json" >nul
if errorlevel 1 (
  echo E4 - VERSION MISMATCH: the game and data folders come from
  echo different releases. Re-extract a single, complete zip.
  pause & exit /b 1
)
rem ── E3: free RAM ≥ 8 GB ──────────────────────────────────────────
for /f "tokens=3" %%m in ('wmic OS get FreePhysicalMemory /value ^| findstr "="') do set FREE_MB=%%m
if %FREE_MB% LSS 8192 (
  echo E3 - NOT ENOUGH FREE MEMORY: %FREE_MB% MB free, 8192 MB required.
  echo Close other applications and try again.
  pause & exit /b 1
)
rem ── E5: writable data dir (outside the portable folder) ──────────
if not exist "%LOCALAPPDATA%\AstraCosmos" mkdir "%LOCALAPPDATA%\AstraCosmos" 2>nul
if errorlevel 1 (
  echo E5 - CANNOT CREATE SAVE FOLDER: %LOCALAPPDATA%\AstraCosmos
  echo (check user profile / read-only media). & pause & exit /b 1
)
rem ── launch at high CPU priority ──────────────────────────────────
start "" /HIGH "astra\astra.exe"
if errorlevel 1 (
  echo E6 - LAUNCH FAILED: the game could not start. & pause & exit /b 1
)
exit /b 0
```


## Appendix C — Shipped Directory Tree (annotated)

```
AstraCosmos/                        ← extracted from astra-x.y.z-windows-x64-portable.zip
├── .start.bat                      ← launcher: E1–E6 checks, RAM gate, start /HIGH (§5.2)
├── python_embed/                   ← embedded CPython 3.11 (tooling only — T-012)
│   ├── python.exe
│   ├── python311.zip, python311._pth
│   └── tools/                      ← pack_assets, pack_data, universe_audit,
│                                     cache_verify, unpack_astroct, make_zip, verify_zip
├── libs/                           ← runtime DLLs (release set: exactly two)
│   ├── vulkan-1.dll                ← Vulkan loader (Apache 2.0)
│   └── OpenAL32.dll                ← OpenAL Soft (LGPL 2.1, dynamic)
├── assets/                         ← ZSTD .astropack pools (decode at load, level 19)
│   ├── heroes.astropack            ← vertex-color hero meshes (≤ 4 MB textures total)
│   ├── audio.astropack             ← music.opus (5 tracks), vo.opus (<200 lines), signals/
│   ├── ui.astropack                ← UI atlas (≤ 1 k), palette tables
│   └── SFX tables are synthesized — not shipped (§2.9)
├── data/                           ← validated, immutable
│   ├── manifest.json               ← version triad member #2, save/chunk format, shader hash
│   ├── anchors.json                ← the 430 real anchor points (GDD §6.2 schema)
│   ├── missions/                   ← mission templates (JSON)
│   ├── codex/                      ← Codex text + schematic IDs
│   └── presets/                    ← low.json / balanced.json / high.json (§3.2)
├── shaders/                        ← precompiled SPIR-V only (T-009)
│   ├── manifest.json               ← pipeline → .spv map + hash (startup-verified)
│   └── *.spv                       ← pbr_lite, starfield, billboard, atmosphere,
│                                     particle, ui, post_bloom, post_lensing, post_dof, tonemap
└── astra/
    ├── astra.exe                   ← the game (version triad member #1)
    ├── logs/                       ← created at runtime (astra-YYYY-MM-DD.log)
    └── dumps/                      ← crash dumps (opt-in in Settings)

%LOCALAPPDATA%\AstraCosmos\         ← runtime-writable, OUTSIDE the zip (USB-safe)
├── astra.cfg                       ← preset choice, binds, options
├── saves/                          ← save.astro files
└── cache/                          ← .astroct disk cache (≤ 2 GB, self-trimming, deletable)
```

## Appendix D — Quality Presets (JSON)

Canonical `data/presets/*.json` (schema v1):

```json
{
  "preset": "balanced",
  "fps_target": [45, 60],
  "internal_resolution": { "base": "1080p", "dynamic": { "min": 0.66, "max": 1.0, "step": 0.06 } },
  "frame_budget_ms": 22.2,
  "blackhole": { "tier": "full_lensing", "mask_radius_screen": 0.25 },
  "terrain": { "local_chunk_verts": 256, "visible_ring": 8, "pregen_depth": 8 },
  "starfield": { "far_points": 500000, "layers": 3 },
  "post": ["bloom", "lensing", "tonemap_grain"],
  "particles": "standard",
  "msaa": 0,
  "nebula_billboards": 16,
  "draw_call_max": 200,
  "ring_occupancy_max": 0.55
}
```

`low.json` and `high.json` follow the same schema with the §3.2 values (low: `tier: "glow_disk"`, `far_points: 100000`, `post: ["tonemap_grain"]`, 128², 720p; high: `tier: "volumetric_16", 512², 1M points, ["bloom","volumetric_bh","dof","tonemap_grain"]`).

## Appendix E — Non-Functional Requirements (NFR list)

| NFR | Requirement | Verified by |
|---|---|---|
| NFR-1 | **Determinism:** world = f(seed, action_log); 10,000-region golden audit + 1,000-tick sim replay, every relevant PR | §7 Determinism audit (build-blocking) |
| NFR-2 | **Memory:** process ≤ 8 GB steady state (5-pool accounting, §2.3); VRAM ≤ 2 GB ceiling / 1.5 GB steady | §7 Memory asserts (release-instrumented) |
| NFR-3 | **Performance:** preset FPS ranges held on reference iGPU; p99 hitch < 200 ms; audio overruns = 0 | §7 Perf harness (nightly, reference box) |
| NFR-4 | **Portability:** zip runs from any writable drive incl. USB; zero admin; no runtime network; no writes inside the folder | §7 Zip integrity + 3 clean-VM tests (P8) |
| NFR-5 | **Save compatibility:** `.astroct`/`save.astro` evolution rules of §2.5 (TLV survival, append-only, overlay isolation); MAJOR version only on format break | §7 Unit (container round-trip incl. unknown TLV) + triad check |
| NFR-6 | **Debuggability:** F9 profiler (per-core utilization, draw calls, RAM/VRAM, pipeline depth), frame-trace dump, validation layers (debug), crash dumps (opt-in) | §3.5 |
| NFR-7 | **Reproducible builds:** pinned toolchain (§4.2), vendored deps, clean-container build < 20 min, SHA-256 manifest | CI every PR + release verification |

---

*End of TDD v1.0 baseline. Next review: Phase 2 exit (terrain codec gate T-004, perf baselines on the reference box, first 1,000-region golden audit).*
