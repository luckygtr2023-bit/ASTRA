# Project Astra Cosmos — Milestone Plan

| Field | Value |
|---|---|
| **Version** | 1.0 |
| **Date** | 2026-09-28 |
| **Status** | Baseline (Phase 0 in execution) |
| **Companions** | GDD v1.0 · TDD v1.0 (T-001…T-014) · Art Bible v1.1 · Audio Design v1.3 |
| **Owner** | Sole developer (solo-dev pipeline: design doc → implement → test → verify) |

---

## 1. Purpose and living-document rules

This document is the **canonical schedule** for Project Astra Cosmos. It is a
living document: any change to a phase boundary, exit criterion, or duration is
recorded in §8 (Change Log) with a reason, and the companion docs (GDD §13,
TDD version tags, Audio Design §9) are updated in the same change set.

**Phase mapping (read this first):** this plan uses **MP Phase n** (n = 0…7).
The GDD (§13) uses 1-based **Phase n+1**. The mapping is exact:

> **MP Phase n = GDD Phase n+1** (MP0 = GDD Phase 1 Pre-production … MP7 = GDD Phase 8 QA & Launch)

The TDD version tags follow the same offset: `v0.2.0` = MP Phase 1 exit,
`v0.3.0` = MP Phase 2 exit, …, `v1.0.0` = MP Phase 7 exit.

---

## 2. Summary table

| MP Phase | Weeks | Duration | Goal (one line) | Gate (exit criterion, condensed) |
|---|---|---|---|---|
| **0 — Pre-production** | 1–2 | 2 wks | Docs + environment locked; proven render path | Docs signed off (GDD/TDD/AB/Audio/MilestonePlan); **Hello-Vulkan triangle on MinGW**, headless probe clean |
| **1 — Engine foundation** | 3–6 | 4 wks | Renderer, memory, threading, chunk streaming | Triangle at **60 FPS**; job system >70% CPU utilization; **100 chunks/s @ 60 FPS**; 1M stars in **1 draw call** |
| **2 — Exploration** | 7–10 | 4 wks | Flight, planet LOD, terrain, landing, scanning | Fly / orbit / land / walk / scan all work; **>30 FPS stable** |
| **3 — Survival & economy** | 11–14 | 4 wks | Mining laser, asteroid belts, inventory, stations | Full **mine → sell → craft → upgrade** loop; 20 recipes; balanced economy |
| **4 — Story, aliens, the Phantom** | 15–18 | 4 wks | Dialogue engine, Act 1, Watcher/Harvester AI, 4-phase Phantom | Playable Act 1; **5% Phantom spawn verified** |
| **5 — Visual polish & black holes** | 19–20 | 2 wks | Interstellar BH shader, volumetric nebulae, post pipeline, adaptive presets | **Black hole at 30+ FPS on Vega 8** |
| **6 — Multiplayer & map** | 21–22 | 2 wks | Discovery sync, 3-layer Star Atlas, mini-map, waypoints | **Two instances share discoveries** |
| **7 — QA & launch** | 23–24 | 2 wks | v1.0 portable package | Zero criticals; **RAM <8 GB**; clean Windows launch via .start.bat |

Total: **24 weeks**.

---

## 3. Phase detail

### MP Phase 0 — Pre-production (Weeks 1–2)

**Goal.** The whole project is papered, the toolchain is proven end-to-end, and
the last sandbox verification is a working Hello-Vulkan. No gameplay code.

| # | Deliverable |
|---|---|
| 0.1 | GDD v1.0 (signed), TDD v1.0 (signed), Art Bible v1.1, Audio Design v1.3, this Milestone Plan v1.0 |
| 0.2 | Toolchain validated & logged: x86_64-w64-mingw32-g++, CMake, Ninja, glslang, Python 3 + pip, Git, Vulkan headers |
| 0.3 | Repository skeleton: exact Phase 0 tree (src/, shaders/, cmake/, docs/, scripts/, assets/, data/, libs/, .start.bat, build.sh) |
| 0.4 | Hello-Vulkan: `--headless` device probe (clean exit 0) + Win32 1080p window, rotating colored triangle, vsync, ESC |
| 0.5 | `build.sh` one-shot pipeline; `PHASE0_EXIT_REPORT.md` with gate line |

**Exit criteria.**

| Criterion | Status |
|---|---|
| All five docs present, versioned, cross-referenced | [x] |
| Toolchain command log with exit codes (every method tried) | [x] |
| Hello-Vulkan **cross-compiles to Windows** (astra.exe) | [ ] — see PHASE0_EXIT_REPORT (sandbox network allowlist blocked the MinGW cross compiler; fix options recorded) |
| Headless probe output logged (or "PENDING USER-SIDE TEST") | [x] native smoke + Windows pending |
| Triangle at 60 FPS on Vega 8 (user-side check) | [ ] PENDING user |

**Key risks.** Toolchain availability in the sandbox (mitigation: recorded
fallbacks + user-side fix options in the exit report); scope creep into
gameplay (rule: no gameplay code in MP0).

---

### MP Phase 1 — Engine foundation (Weeks 3–6)

**Goal.** The Astra runtime exists as a real engine: renderer, memory,
threading, chunk streaming — with the performance numbers that the whole game
is built on.

| # | Deliverable |
|---|---|
| 1.1 | Vulkan renderer v0.1 (swapchain, render pass, pipeline cache, command pool) — evolved from the MP0 skeleton |
| 1.2 | Memory: `VulkanAllocator` + `AstroctBufferPool` (8 GB GPU pool, `.astroct` CPU pre-cached pool), double-buffered frame resources |
| 1.3 | Job system: core pinning (audio core 7), CPU worker pool, >70% measured utilization |
| 1.4 | Chunk streaming: procedural chunk generation + async GPU upload; **100 chunks/s @ 60 FPS** |
| 1.5 | Star field: 1M stars, instanced, **1 draw call** |
| 1.6 | `.astroct` portable data format (header + chunk map + pre-cached ASTC pool) |
| 1.7 | TDD → v0.2.0 |

**Exit criteria.**

| Criterion | Target |
|---|---|
| Triangle (now: rotating + star field) | 60 FPS on Vega 8 |
| Job system CPU utilization | >70% |
| Chunk streaming | 100 chunks/s @ 60 FPS |
| Star field | 1M stars, 1 draw call |

**Key risks.** Streaming bandwidth on Vega 8 (mitigation: `.astroct` pre-caching
per TDD); pool fragmentation (mitigation: fixed-size pool blocks).

---

### MP Phase 2 — Exploration (Weeks 7–10)

**Goal.** You can fly a ship, orbit a planet, land, walk, and scan — the core
exploration verb set of the game.

| # | Deliverable |
|---|---|
| 2.1 | Flight model (sub-light + warp transition per GDD), ship camera |
| 2.2 | Planet LOD chain (interstellar → orbit → surface → ground) |
| 2.3 | Procedural terrain + atmosphere |
| 2.4 | Landing / takeoff, on-foot mode |
| 2.5 | Scanning system (planet/object scan → discovery data) |
| 2.6 | Audio: engine sounds, warp Shepard sweep (per Audio Design) |
| 2.7 | TDD → v0.3.0 |

**Exit criteria.**

| Criterion | Target |
|---|---|
| Fly / orbit / land / walk / scan | All functional, no hard locks |
| Stability | >30 FPS stable (no sustained dips below) |

**Key risks.** LOD pop-in (mitigation: hysteresis + distance budgeting);
landing-state machine complexity (mitigation: explicit state table in TDD).

---

### MP Phase 3 — Survival & economy (Weeks 11–14)

**Goal.** The survival loop: mine, refine, craft, trade, upgrade.

| # | Deliverable |
|---|---|
| 3.1 | Mining laser + asteroid belts (procedural, destructible) |
| 3.2 | Inventory system + crafting with **20 recipes** |
| 3.3 | Meters: hull, power, heat, O₂ (PD ping + brownout + glancing-hull damage per AU-003 — no shields) |
| 3.4 | Stations + trader AI (buy/sell with economy balancing) |
| 3.5 | TDD → v0.4.0 |

**Exit criteria.**

| Criterion | Target |
|---|---|
| Full loop | mine → sell → craft → upgrade, end to end |
| Economy | balanced (no runaway inflation; verified by test scenarios) |

**Key risks.** Economy balance (mitigation: parameterized economy table +
scenario scripts in scripts/); asteroid destruction performance (mitigation:
LOD + culling).

---

### MP Phase 4 — Story, aliens, the Phantom (Weeks 15–18)

**Goal.** The narrative spine: dialogue engine, Act 1, the alien factions, and
the Phantom.

| # | Deliverable |
|---|---|
| 4.1 | Dialogue engine (nodes, choices, conditions) |
| 4.2 | Act 1 (complete, playable) |
| 4.3 | Watcher AI + Harvester AI |
| 4.4 | The Phantom: 4-phase encounter, **5% spawn probability** |
| 4.5 | Audio: Phantasma protocol (2 s fade, 18 Hz bed, gaze-response cue) per Audio Design |
| 4.6 | TDD → v0.5.0 |

**Exit criteria.**

| Criterion | Target |
|---|---|
| Act 1 | Playable start → end |
| Phantom spawn | 5% rate verified (statistical test over N spawns) |

**Key risks.** Scope (Act 1 must be cut early, not late); Phantom audio
perceptual tuning (mitigation: the Audio Design §4.6 parameters are fixed and
testable).

---

### MP Phase 5 — Visual polish & black holes (Weeks 19–20)

**Goal.** The signature visuals land: interstellar black holes, volumetric
nebulae, full post pipeline, adaptive quality.

| # | Deliverable |
|---|---|
| 5.1 | Interstellar black hole shader (accretion disk, lensing) |
| 5.2 | Volumetric nebulae |
| 5.3 | Post-processing pipeline (per Art Bible grade) |
| 5.4 | Adaptive quality presets (Vega 8 tier + 20% headroom rule) |
| 5.5 | TDD → v0.6.0 |

**Exit criteria.**

| Criterion | Target |
|---|---|
| Black hole scene | 30+ FPS on Vega 8 |
| Presets | adaptive switching, no visible hitch |

**Key risks.** BH shader cost (mitigation: LOD of the lensing; pre-baked disk
texture at distance).

---

### MP Phase 6 — Multiplayer & map (Weeks 21–22)

**Goal.** Shared discovery and the navigation layer.

| # | Deliverable |
|---|---|
| 6.1 | Multiplayer: discovery sync (two instances share discoveries) |
| 6.2 | Star Atlas: 3 layers (local system / region / galaxy) |
| 6.3 | Mini-map + waypoints |
| 6.4 | TDD → v0.7.0 |

**Exit criteria.**

| Criterion | Target |
|---|---|
| Multiplayer | Two local instances exchange discoveries reliably |
| Map | All three atlas layers navigable |

**Key risks.** Sync protocol complexity (mitigation: discovery packets are
small, idempotent, versioned).

---

### MP Phase 7 — QA & launch (Weeks 23–24)

**Goal.** v1.0: the portable package that runs on the target laptop.

| # | Deliverable |
|---|---|
| 7.1 | v1.0 portable build (astra.exe + shaders/ + .start.bat + README) |
| 7.2 | QA pass: zero critical defects |
| 7.3 | Performance re-certification: RAM <8 GB, FPS targets per MP2/MP5 gates |
| 7.4 | README + credits, final documentation sync |
| 7.5 | TDD → v1.0.0 |

**Exit criteria.**

| Criterion | Target |
|---|---|
| Criticals | 0 |
| RAM | <8 GB |
| Launch | Clean Windows launch via .start.bat (the Phase 0 triangle path, full game) |

**Key risks.** Late performance regressions (mitigation: per-phase perf gates
above; no new heavy features after MP6).

---

## 4. Dependencies and critical path

```
MP0 (docs + toolchain + Hello-Vulkan)
  └─ MP1 (engine foundation)  ← critical path start
       └─ MP2 (exploration) — needs renderer + streaming + LOD
            ├─ MP3 (survival/economy) — needs terrain/landing/scanning
            │    └─ MP4 (story/aliens/Phantom) — needs the full loop
            └─ MP5 (visual polish/BH) — needs post pipeline (MP1) + scenes (MP2)
                 └─ MP6 (multiplayer/map) — needs discovery data (MP2/MP3)
                      └─ MP7 (QA/launch)
```

- MP5 can overlap MP4 (different systems, same build).
- MP6 depends on MP3's discovery data being stable.
- Everything after MP1 depends on the MP1 performance gates holding — a
  failing MP1 gate is a stop-the-line event, not a slip.

## 5. Risk register

| # | Risk | Impact | Likelihood | Mitigation | Owner |
|---|---|---|---|---|---|
| R-01 | Sandbox toolchain blocks the Windows cross-build (hit in MP0) | Blocks MP0 gate | **Realized** | Fallbacks documented (vendored headers, PyPI cmake/ninja, source glslang); user-side fix options in PHASE0_EXIT_REPORT | dev |
| R-02 | Vega 8 misses a perf gate (60 FPS triangle / 30 FPS BH) | Phase exit delayed | Medium | `.astroct` pre-caching, adaptive presets, per-phase perf gates as stop-the-line | dev |
| R-03 | RAM pressure on 8 GB laptop | Stutter / crash | Medium | 8 GB pool budgets (TDD), Opus streaming (Audio), chunk streaming back-pressure | dev |
| R-04 | Solo-dev scope creep (esp. MP4 story) | 24-week plan slips | Medium | Fixed scope tables above; Act 1 cut list decided at MP3 exit | dev |
| R-05 | Vulkan driver quirks on AMD (Vega 8) | Rare crashes | Medium | Validation layers in all dev builds, crash dumps (dumps/), repro-first | dev |
| R-06 | Multiplayer complexity overrun (MP6) | Launch slips | Low-Medium | Discovery-only sync (no shared simulation); two-local-instance test harness | dev |

## 6. Assumptions

- Single developer, one workstation (sandbox + Windows laptop with Radeon Vega 8, <8 GB RAM budget).
- Windows 10/64-bit as the only ship target; MinGW-w64 cross-compilation as the only toolchain (TDD T-001).
- No paid services at launch (multiplayer is local/LAN-first; cloud sync is out of scope for v1.0).
- All content is procedural or Opus-streamed (Audio Design AU-001: synthesis over storage).

## 7. Governance

- Every phase exit produces a short **exit note** appended to this file's §8
  (date, gate result, deltas).
- A gate failure = iteration buffer: fix prompts are generated per the
  project's established loop (the user receives the exact failure + the
  iteration fix; the gate is re-evaluated).
- Version tags: docs bump on every decision; TDD minor versions on phase exits
  (see mapping in §1).

## 8. Change log

| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-09-28 | Baseline. 8-phase plan fixed (MP0–MP7 = GDD Phase 1–8). GDD §13 and TDD version tags follow the MP n = GDD Phase n+1 mapping. |
