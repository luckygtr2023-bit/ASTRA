# Project Astra Cosmos — Game Design Document

| | |
|---|---|
| **Document** | Game Design Document (GDD) & Living Decision Log |
| **Version** | 1.0 — Baseline |
| **Date** | 2026-09-26 |
| **Status** | **Authoritative.** All development decisions default to this document. |
| **Owner** | Lead Designer (luckygtr2023) |
| **Change Control** | See §0.3 — no design is changed by editing prose alone; record a Decision Log entry (§17). |

---

## Table of Contents

0. [Document Control](#0-document-control)
1. [Executive Summary](#1-executive-summary)
2. [Game Identity](#2-game-identity)
3. [Game Pillars](#3-game-pillars)
4. [Core Loop](#4-core-loop)
5. [Mechanics](#5-mechanics)
6. [World](#6-world)
7. [Story & Narrative](#7-story--narrative)
8. [Species & Encounters](#8-species--encounters)
9. [Art Direction](#9-art-direction)
10. [Audio Direction](#10-audio-direction)
11. [UI / UX](#11-ui--ux)
12. [Technical Requirements & Architecture](#12-technical-requirements--architecture)
13. [Scope & Milestones](#13-scope--milestones)
14. [Success Metrics](#14-success-metrics)
15. [Risks & Mitigations](#15-risks--mitigations)
16. [Open Questions](#16-open-questions)
17. [Decision Log](#17-decision-log)
- [Appendix A — Controls](#appendix-a--controls)
- [Appendix B — Glossary](#appendix-b--glossary)
- [Appendix C — Real Anchor Points (Excerpt)](#appendix-c--real-anchor-points-excerpt)
- [Appendix D — Full XP Table](#appendix-d--full-xp-table)
- [Appendix E — Resource List](#appendix-e--resource-list)

---

## 0. Document Control

### 0.1 What This Document Is

This GDD is the **single source of truth** for Project Astra Cosmos. It is a *living decision log*: it records not only what the game is, but **what was decided, when, and why**. When prose and a Decision Log entry (§17) conflict, the **most recent Decision Log entry wins** and the prose must be corrected in the same commit.

### 0.2 How to Read It

- Sections **1–8** describe *what the game is* (design).
- Sections **9–11** describe *how it should be perceived* (art, audio, UI).
- Section **12** describes *how it will be built* (technical).
- Sections **13–17** manage *the process* (scope, metrics, risks, decisions).

### 0.3 Change Process

1. Propose a change as a Decision Log entry (ID `D-###`, date, decision, rationale, status `PROPOSED`).
2. If the change touches a **Game Pillar (§3)**, it requires explicit sign-off and is marked `PILLAR-EXCEPTION` (see §3.6).
3. On approval, set status to `ACCEPTED`, update the affected prose sections, and bump the version number below.
4. Rejected proposals stay in the log with status `REJECTED` and their rationale — we never silently forget a decision.

### 0.4 Changelog

| Version | Date | Author | Summary |
|---|---|---|---|
| 1.0 | 2026-09-26 | Lead Designer | Baseline GDD. Full scope, pillars, mechanics, world, story, species, art/audio/UI direction, technical plan, 8-phase milestones, decision log D-001…D-016. |

---

## 1. Executive Summary

- **Working Title:** Project Astra Cosmos
- **Genre:** Story-driven space exploration / survival sandbox (single player; optional online layer in Phase 7)
- **Platform:** Windows 10/11, x86-64, **portable folder** (no installer, no admin)
- **Engine:** Custom engine, **C++17 + Vulkan**
- **Performance Target:** AMD Ryzen 7 + 16 GB RAM + **Radeon Vega 8 iGPU** — under **8 GB RAM**, **30–60 FPS adaptive**
- **Release Format:** Single zip, `astra-launcher.exe` → `astra.exe`, ~250 MB

**Core Fantasy.** You are a lone explorer adrift in a vast, procedurally generated Milky Way rendered at 1:1 scale. Every star you meet is either a real star from our galaxy or a deterministic invention of the same universe — and everything you find is *unique*. Beneath the wonder runs dread: your home star is going dark, and the reason is older than humanity.

**Elevator Pitch.** Your home system is dying, prompting a desperate journey to uncover ancient signals, survive against hostile alien life, and discover the secrets of the galaxy itself. Can you find a way to save humanity before your own star goes dark?

**One-Paragraph Vision.** *Astra Cosmos* is *No Man's Sky*'s sense of infinite wonder with *Interstellar*'s visual gravity and the narrative patience of a novel. A single player pilots the ship *Astra* through a deterministic, 1:1-scale Milky Way anchored by 430 real celestial bodies. The loop is discovery: warp, land, scan, mine, survive, craft, trade, level. The surface world is generated in real time on the CPU. The stakes are real: a network of ancient artificial black holes — the Loom — is unwinding the galaxy, and the only mind that has been trying to warn anyone is a creature made of starlight that appears on 1 in 20 planet landings.

---

## 2. Game Identity

### 2.1 Audience & Tone

- **Primary audience:** Players who left *No Man's Sky*, *Elite Dangerous*, and *Subnautica* wanting deeper story and higher-fidelity spectacle on modest hardware.
- **Tone:** *Wonder with a cold undercurrent.* The galaxy is beautiful and alien; the mystery is unsettling, never campy. Horror is implied, environmental, and rare — when it happens (Phantom encounter, Siphon close flyby) it should be the loudest moment of the player's week.
- **Pacing:** Player-directed. The game offers the map and the mystery; it does not hand-hold. There is a "soft leash" (missions, signals) but never a timer.

### 2.2 Session Shape

| Session | Expected content |
|---|---|
| **First session** (45–90 min) | Tutorial arc on Meridian Station → first solo warp → first planet landing → first scan → first story beat (the signal). |
| **Standard session** (1–2 h) | 1–2 warps, 1–3 landings, one mining or mission loop, one crafting decision. |
| **Long session** (4+ h) | Multi-system survey, Harvester engagement, story chapter. |

Autosave on every major transition (dock, warp, landing, death). No manual save UI at launch.

### 2.3 Influences (what we borrow / avoid)

| Reference | Borrow | Avoid |
|---|---|---|
| *No Man's Sky* | Infinite-but-meaningful discovery, scanning/Codex, survival meters | "Empty procedural space" (our anchors + story density fix this) |
| *Elite Dangerous* | 1:1 scale credibility, Newtonian flight feel | UI complexity, scale that outpaces tooling |
| *Interstellar* (film) | Interstellar-grade black-hole lensing as a *recurring* mechanic, not a cutscene | — |
| *Subnautica* | Environmental dread, biolume creatures, base-crafting comfort | Stationary open ocean (we are mobile & orbital) |
| *Outer Wilds* | Knowledge as progression, mystery structure | Time-loop mechanics (out of scope) |

### 2.4 Hard Non-Goals (launch scope)

- No dedicated-server multiplayer, no PvP, no player-built megastructures.
- No modding API, no cloud saves, no controller-cloud, no voice chat.
- No dynamic weather *between* planets (weather is per-planet-surface only).
- No full lip-synced dialogue, no >200 lines of voice.
- No console, macOS, or Linux runtime (Linux is build-only).

---

## 3. Game Pillars

Pillars are the non-negotiable foundation. Every feature proposal is tested against them (§3.6).

### 3.1 Pillar 1 — Exploration (primary loop is discovery)

**Statement.** Every location must offer something unique. The player's primary verb is *look* — at a sky, a ruin, a signal trace — and the game must never answer "what is here?" with "nothing."

**Design obligations:**
- Density contract: every *Region* (10 ly, §6.1) contains **≥ 1 hand-placed POI** (derelict, signal, landmark, nest) and **≥ 5 scannable uniques** (unique mineral signature, storm, anomaly).
- First-visit bonus: every scan of a *new class-instance* yields a Codex delta, XP, and a map pin. Re-scans only yield data depth (§5.4) — no duplicate rewards.
- The map is a promise: anything visible in the Star Atlas must be reachable in ≤ 5 warp jumps.

### 3.2 Pillar 2 — Story & Mystery

**Statement.** A strong three-act narrative unfolds *through discoveries*, not cutscene dumps. Central mystery: ancient alien tech, artificial black holes, and the fate of the galaxy (§7).

**Design obligations:**
- No forced cutscenes during gameplay. Story is delivered via: signal broadcasts, Codex entries, environmental storytelling (derelicts), and MERIDIAN (the station AI, the only spoken/typed companion).
- The player can always *see* the mystery but not yet *understand* it (signal triangulation is a visible, playable mechanic from Act 1).
- Every major story beat is **player-triggered** (a choice, a scan, a jump), never time-triggered.

### 3.3 Pillar 3 — Survival & Progression

**Statement.** Resource gathering, crafting, and survival against hazards create tension; progression (leveling, ship upgrades, deeper exploration) creates momentum. Tension and momentum alternate in a visible rhythm.

**Design obligations:**
- Five survival meters, always readable at a glance (§5.5): Hull, Fuel, Oxygen, Energy, Temperature.
- Every meter must be able to be *the reason you died once* in a normal playthrough (no dead meters).
- Progression is **skill + gear + knowledge**: XP gates tools (L5 Fuel Scoop … L50 Galactic Core Access, §5.6); crafting converts resources into those tools. No stat-arms races, no gold sinks.

### 3.4 Pillar 4 — Alien Life & Encounter

**Statement.** Three distinct species define the galaxy's ecology and politics: **The Watchers** (passive), **The Harvesters** (hostile miners), **The Architects** (godlike creators) — plus the trans-dimensional **Xenothrix Phantasma** as the mystery's living proof.

**Design obligations:**
- Each species has: a **home** (where it spawns), a **signature** (visual + audio), a **relationship** (what it wants, what it drops), and a **rule of no-confusion** (a player at 500 m must identify species in < 2 s).
- Encounter rates scale with story act, not just level (Act 2 = Harvesters everywhere; Act 3 = Watchers gathering).
- The Architects are **never combat encounters**.

### 3.5 Pillar 5 — Signature Visuals

**Statement.** High-quality effects that push iGPU limits, headlined by *Interstellar*-style black holes and the Xenothrix Phantasma. We spend our effects budget on **few, unforgettable moments** — not ambient prettiness.

**Design obligations:**
- Exactly two "AAV" moments (Above-And-Beyond Visuals) at launch: the **Siphon lensing pass** and the **Phantom encounter**. Everything else is B-quality or below.
- Every AAV moment must have a **low-tier fallback** that reads identically to a player who has never seen the effect (silhouette + audio do the storytelling).
- Effects budget is enforced in-engine: post-FX budget 2 ms/frame on the reference iGPU (§12.2).

### 3.6 Pillar Exception Process

A proposal that conflicts with a pillar must: (a) cite the specific pillar clause, (b) propose an equivalent that preserves the pillar's *design obligation*, (c) be logged as `PILLAR-EXCEPTION` and approved by the project owner. **No pillar may be deleted.** Phases may descope features; pillars may not be descopeed.

---

## 4. Core Loop

### 4.1 The Loop

```
        ┌────────────────────────────────────────────────────────┐
        │                                                        │
        ▼                                                        │
  [STATION] ──► [MISSIONS] ──► [WARP] ──► [EXPLORE] ──► [MINE] ──► [SURVIVE]
        ▲                                                        │
        │                                                        ▼
        │          [LEVEL UP] ◄── [UNLOCK TECH] ◄── [TRADE / CRAFT]
        └────────────────────────────────────────────────────────┘
```

- **Station:** dock, sell, buy, craft, read mission board, talk to MERIDIAN, repair/refuel, upgrade.
- **Missions:** board a directive (Survey, Delivery, Salvage, Defense, Research, Story) — always solvable with current gear, always rewarding a *new capability or story thread*.
- **Warp:** spend fuel + energy to jump 10 ly (Mk I) / 50 ly (Mk II) / 200 ly (Mk III) along the Star Atlas.
- **Explore:** survey the region — every scannable object gives data; POIs give story.
- **Mine:** extract resources from asteroids, gas giants, or surface deposits (§5.7).
- **Survive:** keep the five meters alive while the environment and aliens push back (§5.5, §8).
- **Trade/Craft:** convert resources to credits (trade) or capability (craft) (§5.8–5.9).
- **Level up → unlock tech:** XP from scanning/mining/missions/survival drives the exponential curve (§5.6), which gates the next rung of the loop (bigger scoops, deeper scanners, longer warps, the Core itself).

### 4.2 Player Journey (first 3 hours, canonical)

1. **Wake** — aboard the *Astra*, docked at Meridian Station (Tau Ceti system). MERIDIAN briefs the Tau Ceti dimming incident.
2. **First warp** — 10 ly to the nearest anchor system; tutorial: flight, scan, fuel.
3. **First landing** — a Terrestrial planet; tutorial: EVA, oxygen, mining.
4. **First signal** — a faint repeating burst; triangulation mini-mechanic (Pillar 2 hook).
5. **First Watcher** — passive biolume encounter; Codex entry "The Ones Who Watch"; the signal is *answered*, briefly, by nothing the player can see.
6. **First Harvester sting** — a mining run near a harvest site goes wrong; first kill or first retreat — both valid.
7. **First story beat** — the *Astra Beacon* (the generation ship that founded this colony) is found drifting, its reactor hollow. Act 1 begins for real.

---

## 5. Mechanics

### 5.1 Ship Flight

**Feel target:** Newtonian with inertia, *arcade-controlled*. The ship obeys momentum; the controls forgive.

| Property | Value | Notes |
|---|---|---|
| Mass | 12,000 kg (base) | Changes with cargo + modules |
| Max sustained thrust | 450 m/s² | |
| Max velocity | 10,000 m/s (≈ 1 ly per 26 h at top speed) | Warp is the real interstellar tool; top speed exists for orbital work |
| Inertia model | True F = ma | |
| **Arcade assist (default ON)** | | |
| — Velocity damping | 2% per second of drift at idle | "Feels like a ship, drives like a car" |
| — Auto-collision braking | Soft brake within 200 m of collision course | Prevents cheap orbital deaths |
| — Proximity lock target | One lockable target at a time; F locks nearest POI/ship/planet | |
| — Turn smoothing | Pitch/yaw rates eased over 150 ms | No instant vector flips |
| Physics timestep | Fixed 60 Hz | CCD on weapons; ship uses swept-sphere |

**Controls:** 6-DOF WASD + QE, mouse look (or gamepad dual-stick), Space = brake (damps velocity vector), Shift = overdrive (2× thrust, 2× fuel, 5/s energy).

**Scale convention (game units):** 1 ly = 9,460,800,000,000 m compressed to **9,460,800,000 game-m** (10⁻⁶ real scale for interstellar). Within a system: 1 AU = 60,000 game-m; planet radii 2–6 km (terrestrial). Orbital periods run at **1/1000 of true timescale** so motion is perceptible over a session without breaking physics.

### 5.2 Warp Travel

| Warp | Unlocked | Jump distance | Jump time | Fuel cost | Energy cost |
|---|---|---|---|---|---|
| Mk I (base) | Start | 10 ly | 45 s | 15 | 30 |
| Mk II | **Level 20** | 50 ly | 60 s | 40 | 35 |
| Mk III | Act 3 (Architect component) | 200 ly | 90 s | 90 | 45 |

- Warp target must be visible on the current Star Atlas layer (no blind jumps).
- **Jump cost rule:** fuel cost scales with distance *and* with local "Loom strain" (act-dependent): near active Siphons, jumps cost +25% fuel — the galaxy gets *heavier* to cross as the story worsens.
- Warp is a cinematic moment (5 s of light bending) — one of the three "B+ budget" set-pieces (others: planet landing, Phantom).
- Failed warp (insufficient fuel) = hard stop, no damage. The game never punishes a *planned* mistake twice.

### 5.3 Planet Landing & Surface

- **Descent:** guided approach (auto-stabilize toggle) → touch-down on a generated **Landing Zone (LZ)**: every planet has exactly one LZ, a 1.2 km-diameter cleared area chosen by the generator (flatness constraint).
- **Terrain (real-time CPU generation, §12.4):** two tiers.
  - **Global sphere:** 256×128 heightfield (≈ 33k verts) generated on landing — the planet's face from orbit.
  - **Local detail:** 128×128-vertex chunks, 64 m edge, FBM + domain-warp heightfields, generated ahead of the player (8 chunks visible radius, disk-cached per planet-seed).
- **Surface modes:** two sub-experiences on one planet: **on-foot EVA** (first-person, oxygen meter active, 12 m/s sprint, climbable terrain with slope cost) and **ship-hover** (ship drives at 8 m/s altitude, no O2 drain, but cannot enter structures).
- **Time:** surface time is real-time. Night exists where the planet has a day; a planet with a 24 h day is *not* a night cycle — day length is compressed 1000× like orbits, so a full day lasts ~14.4 min. Darkness is a survival state (temperature, visibility), not a chore.
- **Hazards:** per-biome (radiation storms on Volcanic, subzero on Glacial, acid rain on Toxic, sandstorms on Desert, blackouts on Ocean, micrometeor on Barren). One *primary* hazard per planet, chosen at generation; the player sees it on the scan before landing.

### 5.4 Scanning & Codex

**Scanner (Deep Scanner unlocked at L10):**

| Scan target | Time | First-scan XP | Yields |
|---|---|---|---|
| Star | 3 s | 50 | Spectrum class, hazard band, Loom-strain reading |
| Planet (orbital) | 4 s | 100 | Biome, primary hazard, LZ location, resource signature (top 3) |
| Moon | 2 s | 60 | Miniature of planet scan |
| Asteroid | 1 s | 25 | Mineral vector, mining yield estimate |
| Nebula | 6 s | 150 | Composition, transit risk, Watcher-nest flag |
| Derelict | 5 s | 200 | Crew log fragment (narrative), salvage inventory |
| Black hole / Siphon | 8 s | 1,000 | Loom data depth +1 (story-critical) |
| Watcher | 10 s | 500 | Codex chapter + 1 **Signal Fragment** (guaranteed, first 3 only) |
| Harvester structure | 4 s | 100 | Nest map, drop table preview |
| **Xenothrix Phantasma** | 15 s | 2,500 | Codex chapter + **Phantom Shard** 1–3 |

- **Scan depth:** up to 3 re-scans per object-class add *data depth* (extra numbers, extra Codex sentences) — no XP, no duplicate Codex.
- **Codex:** the game's memory. Entries unlock forever (autosaved), categorized: *Species · Phenomena · Technology · Archaeology · The Loom · MERIDIAN*. 100% Codex completion is an optional endgame trophy ("Cartographer's Oath"), not required for any ending.
- **Signal Fragments:** story-currency from Watchers. 12 fragments total across the game; each unlocks a section of *the Signal* — the distress broadcast the player is chasing (Pillar 2: mystery always visible, always one step ahead).

### 5.5 Survival Meters & Death

All meters 0–100 (Hull 100–300 with upgrades). HUD: §11.

| Meter | Drains from | Restores from | Zero state |
|---|---|---|---|
| **Hull** | Impacts, hostile fire, radiation, thermal extremes | Repair kits (surface/station), station docking | Ship destroyed → death |
| **Fuel** | Thrust, overdrive, warp, scoop use | Station, gas-giant scooping (L5+), fuel-cell crafting | Warp impossible; thrust → 50% |
| **Oxygen** | Surface EVA (6/s baseline, ×3 sprint, ×5 with suit breach) | O2 cells, station, ship cabin (sealed) | Asphyxiation → death (30 s grace at 0, then death at 15 s of 0) |
| **Energy** | Scan, weapons, overdrive, temperature control, scoop, warp | Reactor trickle (2/s), station, power cells | Systems brown-out: scan/weapon off, temp control off (20 s) |
| **Temperature** | Vacuum (falls toward −100 °C), stars/radiation (rises), planet ambient | Cabin/suit control (energy-cost), shading maneuvers | Below 0 or above 100: hull stress 2/s until corrected |

**Rules of interaction (the "interesting triangle"):**
- Warp needs Fuel ≥ 15 and Energy ≥ 30 *before* the jump — a low-energy warp attempt is refused, not failed.
- Overdrive is the only thing that burns Fuel *and* Energy fast — it is the meter's panic valve and its trap.
- Temperature is the slow killer: it is the meter that makes *ignoring a meter* eventually kill you.

**Death & respawn (definitive policy):**
- Triggers: Hull ≤ 0 (ship loss) or Oxygen ≤ 0 (suit loss).
- **Respawn point:** last docked station (Meridian until the player owns/docks elsewhere).
- **Cargo penalty:** 100% of carried cargo is jettisoned at the death site. **50% of it is marked recoverable** as a **Salvage Beacon**, persistent for **24 in-game hours** (a normal-day session). The rest is gone — permanently.
- **No XP loss. No level loss. No story loss.**
- **Refill surcharge:** 25% surcharge on station refuels for 6 in-game hours after death (the "insurance" the station sells).
- Ship is replaced with a **refit of the base ship** (modules at 50% condition) unless the player carried a Ship Frame Blueprint (a late-game craftable that preserves modules).
- The death site is pinged on the map forever as a **Grave Marker** — environmental storytelling to the player's own history (and to other explorers in multiplayer, §13 Phase 7).

### 5.6 XP, Leveling & Tech Unlocks

**Curve (definitive):** `XP required for level L → L+1 = 1000 × L^1.5`.
Total XP from L1 → L50 = **7,248,703**. Full table: [Appendix D](#appendix-d--full-xp-table).

| Milestone | Cumulative XP | Expected session-hours |
|---|---|---|
| L5 — **Fuel Scoop** | 28,204 | ~2 h |
| L10 — **Deep Scanner** | 142,671 | ~6 h |
| L15 | 378,073 | ~11 h |
| L20 — **Warp Mk II** | 760,796 | ~18 h |
| L30 | 2,054,619 | ~32 h |
| L40 | 4,174,972 | ~48 h |
| **L50 — Galactic Core Access** | **7,248,703** | ~65–70 h (endgame gate) |

**XP sources (first-instance values; re-scans excluded):**

| Source | XP |
|---|---|
| Scan (by class) | 25–1,000 (§5.4) |
| Mining, per 100 units extracted | 5–15 (by resource tier) |
| Mission complete (Tier I/II/III) | 200 / 500 / 1,200 |
| First survival of a hazard type | 100 |
| Harvester destroyed (worker/escort/siege) | 50 / 150 / 400 |
| **Phantom Shard** collected | 300 each (12 exist) |
| Story beat | 1,000 |

**Unlock ladder (the complete gated list):**

| Level | Unlock | What it changes in the loop |
|---|---|---|
| 1 | Base ship, Arc Lance, manual mining | Tutorial loop |
| **5** | **Fuel Scoop** | Gas-giant fueling; fuel scarcity ends |
| **10** | **Deep Scanner** | Data depth, Siphon readings, Watcher Signal Fragments |
| 15 | Hull Plating II (Hull → 180), Cargo Bay II | Longer sorties |
| **20** | **Warp Mk II** (50 ly jumps) | Galaxy-scale routing becomes personal |
| 25 | Point-Defense Drones | Harvester engagements get fair |
| 30 | Thermal Shielding (Temp meter ×2 range) | Radiation-heavy regions |
| 35 | Reactor II (energy 4/s trickle) | Scan-and-shoot rhythm |
| 40 | Hull Plating III (Hull → 300), Ship Frame Blueprint craftable | Death stops being game-ending |
| 45 | Loom Interface Module (prerequisite for Core) | Act 3 gating |
| **50** | **Galactic Core Access** | Entry to Sagittarius A* / Prime Loom; final act |

Post-50: no levels. Endgame = endings + 100% Codex + multiplayer charting.

### 5.7 Mining

- **Space mining:** asteroids (1–3 mineral vectors), gas giants (fuel, noble gases — requires L5 scoop + 90 s extraction).
- **Surface mining:** geode clusters per biome; a mining beam (energy 4/s) or a pick for small yields. Yields are deterministic from the planet seed — *the same planet always gives the same veins*.
- **Resource tiers:** Common (Ferrite, Silica, Graphene, Water), Uncommon (Cobalt, Cryon, Phosphor, Argon), Rare (Iridium, Harvest Crystal), **Exotic (Phantom Shard, Loomsteel)** — full list [Appendix E](#appendix-e--resource-list).
- **Overmining rule:** each deposit has a finite stock (deterministic); it regenerates 10% per 24 in-game h. The galaxy is renewable but never infinite.

### 5.8 Crafting

Three categories, recipes from Codex discoveries (first discovery unlocks the recipe) or station vendors:

| Category | Examples | Notes |
|---|---|---|
| **Ship modules** | Hull Plating, Scoop Mk II, PD Drones, Reactor II | Installed in 4 module slots; install at station only |
| **Consumables** | Repair Kit (×3 uses), O2 Cell (100 O2), Fuel Cell (25 fuel), Power Cell (50 energy), Thermal Gel | Craftable on surface (workbench 30 s) or station (instant) |
| **Mission tools** | Salvage Beacon (spare), Suit Patch, Signal Beacon, Core Key (Act 3, 3 rare ingredients incl. Loomsteel) | Some are single-use story tools |

**Economy rule:** crafting is *conversion*, not multiplication — total mass in ≤ mass out (10% efficiency loss as "waste," visible in the recipe). No infinite-money loops: every recipe's input set is checked at design time against station sell prices (a CI gate in Phase 4).

### 5.9 Trade & Economy

- **Currency:** Credits (**cr**). Start: 250 cr.
- **Counterparties:** Station trade arrays (buy/sell) and **Courier Drones** (neutral, deterministic: one per sector, orbiting the sector's busiest anchor, rotating stock every 24 h). No NPCs beyond MERIDIAN.
- **Pricing:** base price × regional modifier (per-Quadrant seeded) × scarcity walk (seeded random walk, ±2%/day, clamped ±40%). **No global market** — arbitrage between Quadrants is a real, discoverable strategy (a Pillar 1 reward).
- **Player vs. player trade:** Phase 7 only (station P2P, no mail, no auction).
- **Death and money:** death never touches credits directly — only cargo (§5.5). The economy must never feel like a punishment system.

### 5.10 Missions

Generated deterministically from (seed, station, day). Board at any station; **max 3 active missions**.

| Type | Verbs | Reward shape |
|---|---|---|
| Survey | Scan N new objects in a region | Credits + regional map unlock |
| Delivery | Move payload A→B | Credits + fuel |
| Salvage | Recover a beconed wreck | Cargo (rare items) + credits |
| Defense | Hold a station/site vs. Harvesters (3 waves) | Credits + XP + module discount |
| Research | Collect samples (biome-specific) | Codex depth + recipe unlock |
| **Story** | Act-driven, single-track, always available | Story beats, Signal Fragments, Core Key ingredients |

Missions are the **soft leash**: at any point, the board contains exactly one Story mission that is the next beat of the player's act, plus ≤ 2 side missions that are *thematically adjacent* to it (e.g., during "The Harvest," side missions skew Survey/Defense in Harvester space).

### 5.11 Combat & Hazards

- **Player weapon (primary slot):** Arc Lance — 50 damage, 3/s fire rate, 4/s energy, heatless. Upgrades: L25 PD Drones (passive, 100 rpm vs. projectiles, 60/damage), L35 Ion Lattice (120 damage, 1.5/s, pierce).
- **No shields.** Hull is the shield — this keeps combat legible and the Hull meter honest (Pillar 3).
- **Hostiles:** Harvesters only, in three tiers (Worker ×4–8 swarms, Escort ×1–3, Siege ×1 every third nest). All are *defendable* — no kill is mandatory. Every Harvester engagement has an escape route by design (nest layouts are generated with ≥ 2 exits).
- **Hazards are not enemies:** storms, radiation, temperature, micrometeor. They never *aim*.

---

## 6. World

### 6.1 Scale & Structure

The universe is the **real Milky Way at 1:1 scale**: a ~100,000 ly disk, ~3,000 ly thick, ~200 billion stars, ~2 trillion planets/moons, **~10¹² total objects** (planets, moons, asteroids, belts, nebulae, derelicts, structures).

**Spatial hierarchy (fixed, used by code, map, and IDs):**

| Level | Size | Count | Contents |
|---|---|---|---|
| Galaxy | 100,000 ly | 1 | Spiral arms, Quadrants, Core, halo anchors |
| Quadrant | 10,000 ly | 4 (disk) + halo | Arm identity (e.g., Orion, Perseus), regional modifiers |
| Sector | 1,000 ly | ~10,000 | ~300 stars, Courier Drone, harvest zones |
| Region | 10 ly | ~1,000,000 | ≤ 40 stars, POI density contract (§3.1) |
| **Chunk** | **1 ly** | ~10¹³ (sparse) | The generation unit; 0–8 stars typical |

Only **visited** Regions are ever materialized; everything else exists as (seed, coordinates) → state at query time.

### 6.2 Procedural Generation: the Hybrid System

**Definitive policy (D-004):** every world state is a pure function of `(galactic_seed, coordinates)`, *except* 430 hand-anchored real celestial bodies, which override the fill.

- **Galactic seed:** `ASTRA-2026-09-26` (hash → 128-bit, committed to source). One universe for every player and every playthrough (enables multiplayer, §13 P7).
- **430 Real Anchor Points:** real stars, pulsars, white dwarfs, black holes, nebulae, clusters, and famous galaxies, at their real sky-positions and real spectral classes (excerpt: [Appendix C](#appendix-c--real-anchor-points-excerpt)). Anchors are the **poetry** of the map: the player can find *Sirius*, *Betelgeuse*, the *Pleiades*, *Cygnus X-1*, and *M31*. The full 430-row table ships as `data/anchors.json` (Phase 2 deliverable) — schema is locked now (below).
- **Deterministic fill:** everything else generated by a xxHash3-based PRNG chain:
  - `system_id = H3(seed, region_id, slot_index)`
  - Star class ← seeded pick weighted by arm density; planets ← 0–7 per star, each with a 7-biome pick (§6.3); asteroids ← belt flag; nebulae ← arm-probability; derelicts ← story-act probability + hand-placed POI slots.
- **Determinism guarantees (tested in CI from Phase 2):**
  1. Same seed + same coordinates ⇒ identical world, bit-for-bit, on all platforms (fixed-point where needed; no floating-order hazards — the PRNG consumes integer streams).
  2. Anchor positions are stored as *sky angles*, not cartesian guesses — real objects never drift.
  3. A 24-h CI "universe audit" re-derives 10,000 random Regions and diffs a golden hash.

**`anchor` schema (locked):**

```json
{
  "id": "HD-109309",          // catalog ID
  "name": "Tau Ceti",
  "ra": "12h 06m 27s", "dec": "-17° 18′ 19″",
  "class": "G8V", "distance_ly": 11.9,
  "story": "HOME",            // HOME | STORY | FLAVOR
  "story_note": "Home star of the Tau Ceti Compact."
}
```

### 6.3 Celestial Object Taxonomy

**Stars (by class):** O, B, A, F, G, K, M (main sequence) + WD (white dwarf) + NS (neutron star) + **Siphon** (artificial black hole, story-class, §7). Hazards per class are deterministic (e.g., O/B: radiation bands; M: dense cold dust lanes; NS: cyclotron radiation; Siphon: spaghettification radius + Loom strain field).

**Planets — the 7 types (definitive list, no 8th at launch):**

| # | Type | Surface character | Primary hazard | Signature resources |
|---|---|---|---|---|
| 1 | **Terrestrial** | Forest/rock, day-night | Sandstorms or flash floods (1/2) | Water, Graphene, Ferrite |
| 2 | **Volcanic** | Basalt, lava rivers | Radiation storms | Iridium, Silica, Ferrite |
| 3 | **Glacial** | Ice plains, geysers | Subzero + whiteouts | Cryon, Water, Cobalt |
| 4 | **Desert** | Dunes, mesas | Sandstorms, heat | Silica, Argon, Graphene |
| 5 | **Ocean** | Shallow seas, reefs | Blackout squalls | Water, Phosphor, Cobalt |
| 6 | **Toxic** | Acid fog, storm caps | Acid rain (suit breach risk) | Argon, Graphene, Iridium |
| 7 | **Barren** | Craters, regolith | Micrometeor showers | Ferrite, Silica, (rare) Loomsteel |

**Asteroid belts** (1–3 per star, 50–500 visible bodies), **Nebulae** (arm-tracked; transit risk 10–40%; Watcher nests 8% of nebulae), **Derelicts** (class: colony ship, cargo hauler, *Architect fragment* — the last kind is story-gated).

### 6.4 The Star Atlas (3 Layers)

The map UI (§11) is exactly 3 layers — the "3-layer Star Atlas" is a pillar of UI clarity, not a cut:

| Layer | View | Data |
|---|---|---|
| **1 — Galaxy** | 100,000 ly, spiral arm texture + anchors | Arms, Quadrant borders, anchors, nebulae, Siphon fields, Core, Loom Gates, player's warp-reach circle |
| **2 — Sector** | 1,000 ly | Stars (class-colored), belts, derelict clusters, harvest zones, Courier Drone, unscanned fog |
| **3 — Region** | 10 ly | Bodies, orbits, hazards, POIs, salvage beacons, grave markers, other-explorer presence (P7) |

Fog of war: Layer 1 is fully lit (the galaxy is a known place); Layers 2–3 reveal by scanning/warping. **The Atlas is the player's promise-keeper** (Pillar 1): anything shown is reachable.

---

## 7. Story & Narrative

### 7.1 Premise & Central Mystery

**Premise.** 400 years after the generation ship *Astra* founded the Tau Ceti Compact (the colony on **Tau Ceti**, a real G8V star 11.9 ly from Sol), the Compact's star is dimming. Not dying naturally — being *drained*. The player is a Cartographer: a lone licensed explorer whose job is to keep the colony's map honest.

**Central mystery (the Loom).** An extinct species — **The Architects** — built a galaxy-scale machine network, the **Loom**, whose nodes are **Siphons**: artificial black holes that harvest stellar mass to *reweave* the galaxy's structure. The Loom is failing. Its Siphons are going out of sequence, draining stars that were never supposed to be drained (the player's first, Tau Ceti), and — worse — the Loom's failure is letting something *through* from adjacent dimensions. **The Xenothrix Phantasma is that something: the Architects' trans-dimensional observer-seed, the only part of the Architects that is still awake, trying to warn a galaxy that no longer speaks its language.**

**Why this mystery works (design check):** it is (a) *visual* — Siphons and the Prime Loom are the game's signature spectacle (Pillar 5), (b) *playable* — the player can scan, triangulate, and measure every piece of it (Pillar 2), (c) *ethical* — the ending is a choice about the galaxy's fate, not a boss kill (Pillars 1+2).

### 7.2 Three-Act Structure

**Act 1 — "The Dying Light" (L1–15, ~0–15 h)**
- Beats: the dimming incident → first warp → first Watcher → the *Astra Beacon* (the founding generation ship, found drifting near Proxima Centauri with its reactor hollow — *hollow by a Siphon*, the player cannot yet know) → first Siphon sighting in the distance (a "star that eats sideways").
- Player knowledge at act end: *something is draining Tau Ceti; it is not natural.*
- Key mechanic introduced: signal triangulation (Signal Fragments from Watchers).

**Act 2 — "The Harvest" (L15–40, ~15–45 h)**
- Beats: Harvesters found striping an Architect ruin (they are *not* the enemy of the story — they are the galaxy's scavengers, harvesting what is falling) → the first Siphon up close (AAV moment #1) → the truth from a Watcher archive: the Loom, its purpose, its failure → the Loom Gate at **Cygnus X-1** (a real black hole anchor) opens the sector's "deep map" → the Compact's fate: Tau Ceti loses another 15% of luminosity in a single visible event (in-engine, on the home star — the moment the player's sky *changes*).
- Player knowledge at act end: *the galaxy itself is being unwound; the Loom must be reached at the galactic core; the Architects left a key.*
- Key mechanic introduced: Loom Interface (scan Siphons for "data depth" that physically opens the next ring of the galaxy).

**Act 3 — "The Loom" (L40–50+, ~45–70 h)**
- Beats: the Prime Loom at **Sagittarius A*** (real Sgr A*, the galaxy's anchor) → the Loom's heart is the original *Astra Beacon*'s hollowed reactor — the founding ship *was* an Architect seed-vessel the colony never knew it was (the twist: humanity's cradle is the machine's first anchor) → the Phantasma's truth: it has been at every landing, in 1 in 20 of them, because it follows *the player's signal*, which is the Beacon's echo → the choice.
- **L50 Galactic Core Access** is the literal door: entry to Sgr A* is gated by the L50 unlock + Loom Interface Module (L45) + a Core Key (crafted from Loomsteel, Iridium, a Signal Fragment — the game's three currencies of *knowledge* rather than wealth).

### 7.3 The Player & Characters

- **The player — the Cartographer.** No name in dialogue (MERIDIAN addresses you as "Cartographer"). Gender/identity-agnostic, no portrait. The fantasy is *lone*: no crew, no companion at the hip.
- **MERIDIAN** — the AI of Meridian Station (the home station). The only "character" with a voice. Personality: dry, precise, faintly fond, increasingly afraid by Act 2 without ever saying "afraid." **< 200 spoken lines total** (audio budget, §10). MERIDIAN is the player's *emotional anchor* — the game's thesis: you explore the infinite, but home is a voice on a frequency.
- **The Watchers / Harvesters / Architects / Phantasma** — species as characters, §8.
- **No other named NPCs at launch.** (Deliberate: every face in the game should earn its pixels.)

### 7.4 Endings (three, all reachable, all require L50 + Core Key)

| Ending | Condition | What happens |
|---|---|---|
| **Restore the Loom** | Siphon scan-depth ≥ 90% + 12/12 Signal Fragments | The player splices the Beacon into the Prime Loom; the Loom reboots in sequence; Tau Ceti's light returns over one visible year; the Phantasma, fulfilled, dissolves into the starfield. *Bittersweet-epic.* |
| **Sever the Loom** | Siphon scan-depth ≥ 90% (fragments optional) | The player collapses the Prime Loom's event horizon on itself; the galaxy keeps its stars and loses the machine; Tau Ceti stabilizes but the Compact must learn to live without the Loom's "weather." *Defiant, human.* |
| **Commune with the Loom** | 12/12 Signal Fragments + 100% Phantasma Codex depth | The player merges the Cartographer's log into the Loom's memory; the galaxy gains a new voice — the player's — speaking on every frequency forever. The epilogue is the game's epigraph, rewritten by the player's actual play log (deterministic text generation from mission/survey counts). *The strange one.* |

Ending choice is **made once, at the Core, no undo, no preview** — the game shows the consequences in the epilogue. The choice screen presents what each ending *costs*; it never states which is "best."

### 7.5 Narrative Delivery Rules

1. No cutscene block > 40 s; none interruptible-by-story (skippable by player).
2. Every Codex chapter is written *in-universe* (log fragments, archive entries, MERIDIAN analyses) — the player assembles the truth from documents, in the player's order.
3. Signal broadcasts are audio-only (Opus, §10) — the player *hears* the mystery before they can read it.
4. The 12 Signal Fragments are the only mandatory collectibles; all other content is optional by design (Pillar 1: optional content must *feel* optional, not like homework).

---

## 8. Species & Encounters

### 8.1 The Watchers — "The Ones Who Watch" (passive)

- **Form:** 30–120 m translucent bioluminescent leviathans; body is a lattice of light-lines; slow drift, no locomotive urgency.
- **Home:** nebulae (8% of nebulae host nests of 3–12), around Siphons (always), and in Act 3 they *gather* — whole-nest migrations to the Core, visible on Layer 1 as moving light (the most hopeful image in the game).
- **Relationship:** passive observers. They do not attack, ever. Sustained weapon fire makes them *leave* (not retaliate — they are above retaliation). Scanning a Watcher yields a **Signal Fragment** (first 3) + Codex chapter.
- **Design rule:** a Watcher sighting must always be *calm after awe*. Audio: a near-silent 120 Hz shimmer + a slow 3-note biolume chime (AudioDesign §4.4).
- **Story role:** the galaxy's memory. Their archives (nests) are the source of Act 2's truth.

### 8.2 The Harvesters (hostile)

- **Form:** mechanical-organic: chitinous carapaces grown over hull-frames, mandible drills, engine-wombs. 3–40 m per unit.
- **Tiers:**
  - **Worker** (3–8 m): swarm, drill-attack, 200 HP, drops Ferrite/Cobalt + **Harvest Crystal** (20%).
  - **Escort** (12–20 m): single, 2,000 HP, flak + lance, drops Loomsteel (10%) or module discount tokens.
  - **Siege** (40 m): one per nest, 12,000 HP, rail-spike, never leaves the nest's perimeter.
- **Home:** **Harvest Sites** — Architect ruins and dying stars they strip. Generated with ≥ 2 exits (fair-combat rule, §5.11).
- **Relationship:** hostile to anything that enters a site; *indifferent* elsewhere (they do not patrol the void). Looting a site is a legitimate economy — the danger is the price.
- **Story role:** the scavengers of a dying machine. Killing them is survivability; *understanding* them (Research missions) is what lets the player read the Loom's edge. They are villains in Act 2 scenes and *neighbors* in Act 3.

### 8.3 The Architects (godlike creators)

- **Form:** not encountered. What the player meets: **ruins** (monoliths, rings, the *Loom Gates*), **fields** (gravity warping that bends light *and* the map UI — Layer 2 visibly curves near gates), and **artifacts** (Loomsteel, Core Key components, the Beacon).
- **Presence effects:** time dilation pockets (game time runs 50% slower inside — a legible, non-punitive anomaly), lensing visuals (reusing the Siphon pass at lower intensity), and one rule: **an Architect structure is never a combat encounter.** The threat model is awe + environment, never guns.
- **Story role:** the authors who left. Every Architect element must answer exactly one mystery question when scanned at Loom-depth — no flavor archaeology that doesn't pay rent on the plot.

### 8.4 Xenothrix Phantasma

- **What it is:** a trans-dimensional entity — the Architects' observer-seed, "a phase of the galaxy itself" in the Codex's words.
- **Spawn rule (definitive):** **5% chance on any planet landing** (per planet, per landing; rolls once at LZ generation, cached — the same landing, re-entered, cannot re-roll). First possible sighting: the player's first landing. By L50 the player will have seen it 1–3 times in a normal playthrough; 100% Phantasma Codex depth requires ~40 landings in the right places (endgame commitment, not gate).
- **Appearance (AAV moment #2):** a **200–500 m** tall, thin humanoid silhouette (form per Art Bible A-005: 9:1 proportions, crouched many-jointed encounter pose; **transparent starfield body** — a region of absolute void shaped by the creature, the stars *behind it* bending at its edges (light-lens rim), blue-white Cherenkov edge glow, phasing limbs, and a **void face with tracking eye-points**). It does not cast a shadow; shadows cast *away* from it. It phases: 3 s visible / 7 s absent, on a 10 s cycle, until the player looks *directly* at it, which locks the cycle to a continuous visible state for 60 s (the encounter).
- **Behavior:** non-hostile, unkillable, unscannable-until-encounter. During the encounter it *replays the player's first scan* (the first object ever scanned, re-rendered in its body) — the game is watching the player watch it watch you. Then it leaves, dropping **Phantom Shards** (1–3, 300 XP each, story-currency).
- **Audio:** no sound of its own — the encounter is defined by *what stops*. Canonical timing (per AudioDesign AU-002): ambient + music fade to digital silence over 2 s, then a continuous **18 Hz infrasonic sine bed (felt, not heard)** runs under the whole encounter; the player's own breathing (synthesized, §10) returns at 3 s; one sub-bass note lands at 45–55 s, HRTF-positioned at the creature's face. The bed and breathing fade out as it leaves; the ambient returns slower than it went.
- **Story role:** the mystery's living proof. Shards + Codex depth gate the third ending and the Act 3 truth.
- **Implementation budget:** 20k-tri procedural mesh + a full-screen "void body" shader (no textures — the starfield is the scene's own starfield, masked and inverted — free of extra assets).

### 8.5 Encounter Design Rules (all species)

1. **Identifiability:** at 500 m, species is identifiable in < 2 s by silhouette + color (Watchers = cyan lattice; Harvesters = rust-red + chitin; Phantasma = void silhouette).
2. **No ambushes:** any hostile encounter begins ≥ 2 km away or from a known site. The player always sees the threat before the threat is close.
3. **Escape is always possible:** every combat space has ≥ 2 exits; PD Drones (L25) make late-game Harvester encounters about *choices*, not survival.
4. **Story scaling:** Act 1 (Watchers-heavy, no Harvesters within 50 ly of home), Act 2 (Harvester zones expand ×4), Act 3 (Watchers converge on the Core; Harvesters go quiet — they are leaving, too).

---

## 9. Art Direction

> **Visual authority:** [ArtBible.md](ArtBible.md) v1.0 defines the full visual system (biome palettes, lighting/LUT architecture, AAV render specs, hero assets, UI art). This section states the design intent; the Art Bible says *how it looks*; decisions are cross-logged (GDD D-018 ↔ Art Bible A-002/A-003/A-005).

### 9.1 Style: Stylized Proceduralism

> **Vertex colors over textures. Low-poly hero assets. Bold, saturated colors.**

The art identity is *deliberately cheap on memory, expensive on light*. No diffuse textures on world geometry at launch; color lives in vertices and in the atmosphere/shader layer. This is both an aesthetic (post-Slay-the-Spire/Deep Rock clarity at space scale) and an engineering decision (VRAM budget, §12.2).

### 9.2 Pipeline Rules

1. **World geometry:** vertex-color only. Palette-quantized to 64 colors per mesh to keep attribute bandwidth low.
2. **Hero assets** (player ship, Watcher, Harvester tiers, Phantasma, Siphon, station): low-poly (≤ 20k tris each), vertex-color base + **one small texture each allowed** (≤ 1k, roughness/normal only). Total hero texture memory **≤ 4 MB** — a constraint we are proud of.
3. **Lighting:** one directional (the star) + hemispheric sky + per-object emissive (biolume, engine, Siphon disk). No dynamic shadows in space; one blob shadow on surface (player-only, 1024², reused).
4. **Atmosphere is the painter:** the "painting" happens in atmosphere/shell shaders — rim scatter, planet limb, nebula volumetric billboards (≤ 16 concurrent), starfield layers (3: static far / parallax mid / warp-streak near).

### 9.3 The Palette (locked v1 — extended by Art Bible §2.2)

The **master signal palette** below is locked. The **world/biome palette** (7 biomes + Exotic/Loom) is defined in [ArtBible.md §2.2](ArtBible.md) — see D-018. "Black is reserved" applies across both palettes (Volcanic uses charcoal `#2A2226`, not black).

| Role | Hex | Used for |
|---|---|---|
| Void Indigo | `#0B0E2A` | Space background, UI base |
| Star Amber | `#FFB347` | Warm stars, UI primary, hope |
| Biolume Cyan | `#4DF0E0` | Watchers, scanning, data |
| Rust Red | `#C0392B` | Harvesters, warnings, danger |
| Architect Gold | `#E8C547` | Loom tech, story UI |
| Phantom Void | `#000000` + starfield | Phantasma only — black is *reserved* |

Rule: **black is reserved.** Nothing in the game is pure black except the Phantasma and true void. The player's eye is trained to treat "black" as "the wrong kind of space."

### 9.4 Signature Visuals (the two AAVs)

**AAV-1 — The Siphon (Interstellar-class black hole lensing).**
- A real gravitational-lensing post pass: screen-space ray-bend around the Siphon using an analytic Schwarzschild approximation (bend angle α = 4GM/(c²b), solved per-pixel within a 25% screen-radius mask; outside the mask, cheap glow). Accretion disk: procedural shader (no texture) — relativistic beaming + Doppler shift + time-dilated frame rotation.
- **Readability at all tiers:** even at Low (glow + disk only, no lensing), the silhouette reads as "a hole in the sky that is looking back."
- **Reused at half-intensity** for Loom Gates and near Architect structures (the map UI's curve, §8.3, is the same math in 2D — one signature, three applications).

**AAV-2 — The Phantom (§8.4).**
- Starfield-inversion body shader + 20k-tri procedural silhouette + the audio cut. No extra texture memory, no extra lights. The effect is *absence*, which is cheap and unforgettable.

### 9.5 Performance-Aware Art

- Dynamic resolution: 100% → 66% governor (§12.2).
- Spheres/asteroids: 4 LODs; planets: 3 LODs + always-on atmosphere shell.
- Nebula billboards & starfield layers scale with resolution tier (bigger at 100%, fewer at 66%).

---

## 10. Audio Direction

> **Audio authority:** [AudioDesign.md](AudioDesign.md) v1.3 is the complete sound design spec (synthesis-over-storage strategy, engine/pulsar/radio/Phantasma parameters, HRTF + Doppler, music/dialogue Opus plan, 50 MB RAM cap, full SFX index). This section states the design intent; the audio document says *how it sounds*.

**Philosophy: synthesize what is procedural, stream what is composed.** The game's soundscape is 70% generated in real time on the CPU (a small custom DSP: oscillators, filters, delay/convolver-lite, granular) and 30% pre-composed, streamed from **Opus** (the lowest-overhead codec that still sounds alive; fits the iGPU/low-RAM target — audio is CPU-bound, not GPU).

| Layer | Method | Content |
|---|---|---|
| **SFX** | Real-time CPU synthesis | Thrusters (speed → filter cutoff + pitch), scanning (class → timbre — every scan type has a *voice*), weapons, impacts, UI, suit (breathing, O2 valve ticks — the O2 valve is the oxygen meter's *heartbeat*), storm/radiation beds (noise-shape per biome) |
| **Music** | Opus stream, **5 tracks** | *Drift* (space, ambient), *Landing* (surface, pulse), *Harvest* (tension), *Loom* (awe/terror — the Act 2+ theme), *Meridian* (home, the warmest 90 seconds in the game). Mono 32 kbps; ≈ 11 MB across all 15 stems (AudioDesign §7). Adaptive: layer stems (drone/pulse/lead) mixed by game state — 5 tracks, 15 layers of feel. |
| **Voice** | Opus stream | MERIDIAN only, < 200 lines, mono 32 kbps, ≈ 3.2 MB (AudioDesign §7). Signal broadcasts: 12 fragments × ~40 s, a distorted chorus *not* MERIDIAN (the Architects' voice — granular-processed at load time). |
| **Spatialization** | **HRTF** | 8-directional HRTF filter set applied per-source (headphone-first; auto-detected to stereo speakers with a simplified model). Targets: planet surfaces (wind, biolume chimes, Harvester drills), station interiors, and the Phantasma encounter (the single sub-bass note comes *from the creature's face* (AU-002/A-013), which the player can't fully resolve — audio is the only map). |

**Budget:** audio RAM < 100 MB; DSP cost < 1.5 ms/frame; all synthesis on a dedicated audio thread (48 kHz, 256-sample period).

---

## 11. UI / UX

### 11.1 Principles

1. **Minimal HUD:** the default screen is *the galaxy*. Meters are small, cornered, and honest (no animated fluff).
2. **Three keys to everything:** **F1** Star Atlas · **F2** Codex · **F3** Ship/Inventory. The rest is context-driven.
3. **The map is a promise** (§6.4) — the UI never lies about distance, reachability, or danger.

### 11.2 Default HUD

```
┌──────────────────────────────────────────────────────┐
│ [distance/lock ribbon — top center, 60% width]       │
│                                                      │
│                                                      │
│                                              ┌────┐  │
│                                             [atlas│  │  ← top-right: 3-layer toggle, 120 px
│                                             thumb] │
│                                                      │
│                              [scan ring + prompt]    │
│                                                      │
│  [Hull ▮▮▮▮▮]                                       │
│  [Fuel ▮▮▮▮▯]      (left-bottom meter stack,       │
│  [O2   ▮▮▮▮▮]       5 bars, 2 px tall, no labels    │
│  [Egy  ▮▮▮▮▯]        until hovered)                │
│  [Temp ▮▮▮▮▮]                                       │
└──────────────────────────────────────────────────────┘
```

- **Lock ribbon:** nearest target name, class, distance (in ly or km — the UI knows which space you're in), hazard glyph.
- **Scan prompt:** appears when looking at a scannable object; ring fills with scan time; first-scan entries show **NEW** in Biolume Cyan — the game's dopamine color.
- **No minimap in space** (the lock ribbon + Atlas replace it). On surface: a 90 px compass ribbon (top) instead.

### 11.3 Major Screens

| Screen | Key | Content |
|---|---|---|
| **Star Atlas** | F1 | 3 layers (§6.4), zoom = layer, click = warp target, right-click = info card (scan depth visible) |
| **Codex** | F2 | Tree: Species · Phenomena · Technology · Archaeology · The Loom · MERIDIAN. Entries: text + one schematic diagram each (procedural line-art, no texture art) |
| **Ship** | F3 | Hull diagram with 4 module slots, cargo (24 slots → 48 upgradable), meters detail, condition |
| **Inventory** | F4 | Grid + quick-craft wheel (recent 6 recipes) |
| **Crafting** | F5 | 3 categories (§5.8), recipe locked = shows ingredient *silhouettes* (the Codex-driven tease) |
| **Missions** | F6 | Board + 3 active slots + Story mission pinned |
| **Settings** | Esc > | Graphics (3 presets + dynamic-res toggle), Audio (spatialization on/off), Control remap |

### 11.4 Full Control Map

[Appendix A](#appendix-a--controls) is canonical. Design rules: every bind remappable; no action needs more than 2 held keys; the gamepad is a first-class citizen (full dual-stick 6-DOF).

---

## 12. Technical Requirements & Architecture

### 12.1 Platform & Hardware (the fixed target)

| | |
|---|---|
| OS | Windows 10 64-bit / 11 |
| CPU | AMD Ryzen 7 (8-core baseline) — all real-time generation is CPU work; the engine is written for 8 cores: 1 render, 1 audio, 1 physics/logic, 4 generation/streaming workers, 1 idle/steal |
| RAM | 16 GB system, **hard budget: process < 8 GB** (resident, steady-state, Act 3, 1080p) |
| GPU | **AMD Radeon Vega 8 iGPU** — the reference GPU; VRAM budget **< 1.5 GB** (shared memory counts against the 8 GB) |
| FPS | **Adaptive 30–60**: governor targets 60, floors at 30 (dynamic resolution 100%→66%, then LOD bias, then nebula/starfield counts) |
| Storage | < 250 MB total install; planet terrain disk cache unbounded but self-trimming (2 GB max) |

**Portability:** single zip → `astra-launcher.exe` (native C++ Win32, ~1 MB) → `astra.exe` + data files. No installer, no admin, no UAC, no telemetry (crash dumps local-only, opt-in).

### 12.2 Performance Budget (reference iGPU, 1080p @ 100%)

| System | Budget |
|---|---|
| Total frame | 16.6 ms (60 FPS) |
| — Draw/render (Vulkan) | 7 ms |
| — **Post FX (lensing pass active)** | **2 ms** |
| — CPU logic/physics | 4 ms |
| — **Terrain generation (amortized, avg)** | **2 ms** |
| — Audio DSP | 1.5 ms |
| — IO/streaming | 1 ms |
| VRAM | 1.5 GB (meshes 0.6 / shaders+UBOs 0.2 / terrain 0.4 / atmosphere 0.2 / misc 0.1) |
| RAM (process) | < 8 GB (world cache 2.5 / terrain disk cache memory 0.5 / assets 0.5 / audio 0.1 / engine 1.0 / headroom 3.4) |

Budgets are enforced in-engine (draw-call, VRAM, and per-system frame-time assertions on the reference GPU; over-budget = logged, not hidden).

### 12.3 Engine Architecture (C++17 + Vulkan)

No engine licensing — a purpose-built engine, owned by the project.

```
astra
├── core/        allocators (frame + arena), log, config, json, xxHash3, timers
├── render/      Vulkan 1.1 (VK_KHR_swapchain, VK_EXT_extended_dynamic_state),
│                PBR-lite (vertex-color first), post pipeline (lensing pass), GPU-driven culling
├── ecs/         archetype storage, EntityId (u64: gen<<32|id), components are POD
├── physics/     fixed 60 Hz: swept-sphere ship, sphere/capsule surface, CCD weapons
├── procgen/     the universe: seed → region → system → body chain; xxHash3 PRNG;
│                anchors.json override; disk-cache (LZ4-compressed region blobs)
├── terrain/     CPU heightfield generator (FBM + domain warp, 4+2 octaves),
│                global-sphere + local-chunk two-tier (§5.3), mesh build on worker threads
├── audio/       PortAudio (static) + OpenAL Soft (bundled DLL) + libopus, custom DSP synth, HRTF mixer (TDD T-003)
├── ui/          immediate-mode (custom, no external UI lib), atlas-driven, no textures > 1k
├── net/         (Phase 7) action-log replication, delta compression, host-authoritative sessions
└── game/        systems: flight, meters, scan, mine, craft, trade, missions, story, species AI, phantom
```

**Key architectural decisions (locked):**
- **ECS, not OOP entities.** World state is data; systems are functions. Enables deterministic replay (below) and worker-thread generation.
- **Determinism by construction:** all game-time randomness flows from `xxHash3(seed, salt, draw_index)` — integer streams, no `rand()`, no floating-point-ordered aggregates in the PRNG path. Save = seed + action log (§12.6).
- **Vulkan 1.1 baseline** (iGPU-friendly, no ray tracing, no DX12 mental tax); pipeline cache on disk for the portable folder.

### 12.4 Procedural Generation Architecture

- **Region derivation:** `region(seed, coords)` is O(1) hash work (no traversal); a Region yields its systems; a system yields its bodies — each level a fixed-size integer-stream decode (≤ 200 µs cold, disk-cached after first visit).
- **Terrain:** generated on 4 worker threads; the terrain system is *pull-based* (the render thread requests 1-ring-ahead chunks); generation never blocks the main thread (worst case: a 32 m "soft ground" fallback shell for ≤ 100 ms).
- **CI determinism gate (Phase 2, permanent):** 10,000 random regions/planet-seeds re-derived nightly, golden-hash diffed. A universe that drifts between builds is a build-blocking defect.

### 12.5 Audio Architecture

**Backend (updated 2026-09-27 per TDD T-003 / decision D-017):** PortAudio (MIT, static) as the real-time playback core, OpenAL Soft (`OpenAL32.dll`, bundled) as the spatial/HRTF layer, libopus for music/VO decode. The original custom-WASAPI plan is superseded — PortAudio cross-compiles more cleanly under MinGW-w64 and removes months of driver-edge-case work. DSP graph: 64 voice slots → seeded synth bus → OpenAL positioning + custom CPU HRTF filter stage (AudioDesign Appendix C) → stereo master. Opus: three decoder instances (music / VO / fragments) into 32-bit float ring buffers (AudioDesign §2.4). All synthesis functions are pure (seeded) → the SFX are also deterministic (a free win for the action-log architecture: audio is a function of the same state).

### 12.6 Save & the Multiplayer-Ready Save

**Save = (galactic_seed, player_state, action_log).**

- `action_log`: append-only JSON-lines of player *intent* operations (`warp(target)`, `mine(deposit_id, units)`, `craft(recipe, inputs)`, `scan(obj)`, `dock(station)`, `combat(action)`…).
- World state at any point = **replay(seed, log)** — the same function the multiplayer layer uses to sync other players (Phase 7: log replication, no world snapshots).
- Consequences: (a) saves are tiny (MBs, not GBs), (b) save corruption is bounded (truncate to last valid op), (c) "travel back" is free (replay to an earlier op — a debug tool at launch, a time-travel Easter egg if we ever want it), (d) cheating in online sessions is structurally hard (a lie in the log is visible on replay).
- Compaction: every 500 ops, the log is compacted to a checkpoint (deterministic world snapshot, LZ4) + tail — keeps replay O(tail), not O(history).

### 12.7 Build & Distribution

- **Build:** Linux (Ubuntu 22.04 CI) → **MinGW-w64** cross-compile → Windows x64. CMake toolchain file in-repo; static link everything except `kernel32/user32/gdi32/vulkan-1/winmm` (+ WIL-free Win32 wrappers we own).
- **Dependencies (all cross-compilable, vendored):** xxHash, LZ4, libopus, stb-style headers (our own, minimal), no other third-party.
- **Artifact:** `astra-windows-x64-portable.zip` (launcher + exe + data + shader cache dir). Launcher: sets up data paths, detects GPU class (auto-selects Low/Med/High preset on Vega-class iGPUs), writes `astra.cfg`, launches.
- **CI gates:** build + determinism audit + performance smoke (headless frame-time harness on a reference VM) on every PR to `main`; release tag builds the portable zip + SHA-256 manifest.

### 12.8 Future-Ready Data Structures (the "designed for tomorrow" list)

| Structure | Shape (locked) | Future system it enables |
|---|---|---|
| `EntityId` | u64 (gen<<32 \| id) | ECS swap-ins, netcode entity translation |
| `RegionBlob` | LZ4-composed, versioned (v1) | Cloud region cache, offline map services |
| `action_log` opset | Versioned JSON-lines, 1 op = 1 intent | Multiplayer replication, replays, anti-cheat |
| `CodexEntry` | {id, category, chapter, depth 0–3, text, schematic_id} | Community survey ledger (P7): shared, anonymous discovery stats merge as depth-deltas |
| `AnchorRecord` | §6.2 schema (14 fields) | Mod anchors / community star catalogs |
| `ResourceLedger` | float64 mass, per (entity, resource_id) | P2P trade, market analytics, future trading hubs |
| `MissionTemplate` | JSON: verbs, rewards, gates, story_tag | Mod missions; P7 community mission packs |
| `MeshAsset` | vertex-color + optional 1k tex, 4-LOD chain | Future hero-asset swaps without pipeline change |
| `HRTFProfile` | 8-direction filter coefficients | Head-tracking, per-user spatialization tuning |
| `SiphonField` | {center, G, strain, seq_index} | Loom simulation; any future gravity gameplay |

**Rule:** any system added after launch must consume these shapes, not fork them. If a fork is truly needed, that is a `PILLAR-EXCEPTION`-class decision (§17).

---

## 13. Scope & Milestones

Eight phases. Each has **entry criteria** (what must be true to start) and **exit criteria** (measurable, not vibes). Dates are planning estimates; the exit criteria are the contract.

> **Canonical schedule:** [MilestonePlan.md](MilestonePlan.md) v1.0 — MP Phase n = GDD Phase n+1 (MP0 = Phase 1 … MP7 = Phase 8). The Milestone Plan owns timing; this section keeps the phase content and exit criteria.

### Phase 1 — Pre-Production (est. 2 weeks)
- **Entry:** Green light.
- **Deliverables:** this GDD v1.0 (done), `anchors.json` full 430-row table (real data pass), palette v1 + 2 concept sketches (Siphon, Phantom), budget sign-off, control scheme frozen.
- **Exit:** every §12.8 shape has a header file; anchors.json passes schema + sky-position audit; GDD v1.0 committed.

### Phase 2 — Engine Foundation (est. 6 weeks)
- **Entry:** Phase 1 exit.
- **Deliverables:** `core`, `render` (Vulkan hello-world → lit vertex-color cube), `ecs`, `procgen` (seed → region → system → body, **anchor override live**, CI determinism audit running), `audio` (PortAudio synth loop + one Opus stream + OpenAL spatial bus), CI: Linux→MinGW build green, crash-dump harness, frame-time harness.
- **Exit:** a blank 1-ly Region renders with 3 generated stars + 1 anchor star (Tau Ceti at real sky position) at 60 FPS on the reference iGPU; determinism audit green for 7 consecutive days; audio: one synthesized hum + one streamed note, spatialized.

### Phase 3 — Exploration Core (est. 8 weeks)
- **Entry:** Phase 2 exit.
- **Deliverables:** flight (Newtonian + arcade assist, lock ribbon), warp Mk I, Star Atlas 3 layers, scanning + Codex (first 60 entries), planet landing + two-tier terrain + EVA, first 15 Codex chapters, Meridian Station (interior + dock + MERIDIAN text).
- **Exit:** the **canonical first 3 hours** (§4.2) is playable end-to-end by a new player with zero help; 0 crashes in a 2 h dogfood; terrain hitch < 100 ms (the soft-ground fallback has not triggered > 2×/session); 60/30 FPS on the reference iGPU in space/surface respectively.

### Phase 4 — Survival & Economy (est. 6 weeks)
- **Entry:** Phase 3 exit.
- **Deliverables:** 5 meters + death/respawn protocol (§5.5), mining (space + surface), crafting (3 categories), trade (stations + Courier Drones), missions (all 6 types + board), XP curve + unlock ladder L1–50 wired (unlocks gate real systems), overmining/regeneration.
- **Exit:** a full loop — dock → warp → land → mine → survive → craft → trade → level → new tool — completed in a 90-minute playtest with no meter that never mattered (each meter causes ≥ 1 death in 5 test sessions); economy CI gate green (no craft = net-positive value loop).

### Phase 5 — Story / Aliens / Phantom (est. 8 weeks)
- **Entry:** Phase 4 exit.
- **Deliverables:** Act 1–3 story content (all beats, 12 Signal Fragments, 3 endings), Watchers (nests, migrations), Harvesters (3 tiers + nest generation + AI), Architects (ruins, Loom Gates, time-dilation pockets), **Phantasma** (5% landing rule, AAV-2 encounter, shards), Siphons (AAV-1 lensing pass, Loom-strain warp costs), MERIDIAN VO (< 200 lines) + Signal audio (12).
- **Exit:** a 70-hour playthrough blueprint hit: all 3 endings achieved in separate test saves; Phantom encounter triggers exactly ~5% over 200 scripted landings (χ² within tolerance); AAV-1 holds 2 ms post budget on the reference iGPU; zero "cutscene interruption during player action" in the QA pass.

### Phase 6 — Visual Polish (est. 6 weeks)
- **Entry:** Phase 5 exit.
- **Deliverables:** full palette pass, atmosphere shaders, starfield layers, nebulae, planet limb/limb-scatter, AAV-1/AAV-2 final passes + low-tier fallbacks verified, UI polish (all §11 screens), dynamic-res governor, audio mix final (all 5 music tracks, HRTF pass).
- **Exit:** the two AAVs are *the* moments in a blind playtest (playtesters independently name them as the top-2 visuals); 1080p @ 100% on the reference iGPU: mean 60 FPS, p99 hitch < 200 ms; the palette hex table (§9.3) matches the shipped build (screenshot diff of 20 canonical scenes).

### Phase 7 — Multiplayer & Map (est. 6 weeks)
- **Entry:** Phase 6 exit.
- **Deliverables:** online layer over the action-log architecture: presence (other explorers on Layer 3, their grave markers), co-op salvage (2 players, host-authoritative), station P2P trade, the **Survey Ledger** (anonymous, aggregated community discoveries enriching Codex depth — no personal data, no maps-of-players), netcode: opset replication, delta compression, session list (open invite + code only, no lobbies UI).
- **Exit:** 2 players on different networks complete a co-op salvage with < 150 ms perceived delay (loopback test) and < 1% opset divergence over a 1 h session; P2P trade completes across 5 test transactions; Survey Ledger merges 1,000 simulated discoveries without corruption.

### Phase 8 — QA / Launch (est. 4 weeks)
- **Entry:** Phase 7 exit.
- **Deliverables:** 2-week playtest (external, 10–20 players), crash-triage pass, performance certification (reference iGPU, 3 preset tiers), portable-zip + SHA-256, launch page, manual (in-game), post-launch plan (P7 follow-ups, mod-anchor discussion).
- **Exit:** < 30 open P1/P2 bugs; reference iGPU: 30–60 FPS adaptive holds in a 1 h stress (Act 3 region, full post); portable zip installs and runs from a USB stick on 3 clean Windows VMs; the full GDD decision log reconciled with the shipped build (every `ACCEPTED` decision implemented or formally descoped).

---

## 14. Success Metrics

**Design KPIs (measured in playtests, not guesses):**
1. First story beat reached by ≥ 80% of new players within 60 minutes, unprompted.
2. ≥ 70% of playtesters describe Act 2's "Compact sky event" (Tau Ceti dims, on-screen) as "the moment I got it."
3. ≥ 90% of landed planets generate terrain with < 2 s to walkable (from descent start).
4. Each of the 5 survival meters is the *stated* cause of death for ≥ 1 player in a 20-person cohort.
5. ≥ 60% of players name a Siphon or the Phantom as the most memorable visual, unprompted.
6. Retention proxy: ≥ 50% of cohort self-reports a 3+ session week in playtest weeks 1–2.

**Engineering KPIs (continuous, CI):**
1. Frame: mean 60 / floor 30 FPS on reference iGPU; p99 hitch < 200 ms; post budget 2 ms.
2. RAM: process steady-state < 8 GB (Act 3, 1080p).
3. Determinism audit: 100% golden-hash pass, 24 h cadence.
4. Build: Linux→MinGW green in < 20 min CI; zip < 250 MB.
5. Crashes: 0 P1 in 2 h dogfood windows (Phase 3+), < 30 P1/P2 open at launch.

---

## 15. Risks & Mitigations

| # | Risk | Likelihood | Impact | Mitigation (owned by phase) |
|---|---|---|---|---|
| R1 | iGPU VRAM ceiling bites (terrain + post + nebulae) | High | High | Two-tier terrain with disk cache; VRAM budget assertions from Phase 2; AAVs have no-VRAM fallbacks (Pillar 5); dynamic res governor from Phase 3 |
| R2 | CPU terrain gen causes hitches | Med | High | Pull-based 4-worker generation; 100 ms soft-ground fallback; amortized 2 ms budget; CI hitch measurement |
| R3 | Audio backend cross-compile friction | Med | Med | Mitigated by design: PortAudio + OpenAL Soft chosen for clean MinGW-w64 cross-compilation (TDD T-003, D-017); audio thread is TIME_CRITICAL on core 7 with a 10 h loop test gate |
| R4 | Scope creep in story/content volume | High | High | Content is *gated* by Codex depth, not quantity; descoping rule: cut optional Codex chapters first, never story beats; pillar-exception process |
| R5 | Determinism breaks (float ordering, parallel gen) | Med | Critical (kills save + P7) | Integer PRNG streams only; CI audit is build-blocking; replay tests in every PR touching `procgen`/`terrain` |
| R6 | 70-hour curve drags (L20→L40) | Med | Med | Economy check: unlock pacing validated against Appendix D in Phase 4 playtests; the Loom-strain warp cost is the pacing dial (tighten = faster travel = faster pacing) |
| R7 | Phantom spawn feels random/unfair | Low | Med | 5% rule is *landings*, not planets — communicated in Codex ("it follows the landings, not the stars"); first guaranteed sighting window: by the player's 20th landing (soft ceiling) |
| R8 | Multiplayer slips launch | Med | Low (it's P7) | P7 is additive: the single-player game is complete and shippable at P6 exit; P7 ships as a content update if needed |
| R9 | Real-anchor data errors (wrong positions/classes) | Low | Med | anchors.json schema audit in P1 + a public errata file (the game's fiction tolerates a "catalog revision" — one Codex wink) |
| R10 | Solo-developer velocity | High | High | The 8-phase exit criteria are *the* schedule; any phase overrunning triggers the descope ladder (R4) before timeline cuts quality |

---

## 16. Open Questions

Logged here; each must become a Decision Log entry before the phase that needs it starts.

| ID | Question | Needed by |
|---|---|---|
| OQ-1 | Distribution channel at launch: itch.io, Steam (if budget allows), or direct-download page? | P8 |
| OQ-2 | MERIDIAN VO: one voice actor, or a *dry text-to-speech-style* synthesized voice (on-brand, near-zero cost)? | P5 |
| OQ-3 | Do Watcher migrations (Act 3, Layer-1 moving lights) need a second, smaller AAV budget, or are they billboard-only? | P6 |
| OQ-4 | Post-launch: modding = anchor packs only (cheap, safe) or mission-template packs (the structures already exist, §12.8)? | Post-launch |
| OQ-5 | Should the "Commune" ending's player-log epilogue be shareable as a text card (viral surface)? | P8 |
| OQ-6 | Controller gyro for mouse-look (a first for the genre on this hardware)? | P6 |

---

## 17. Decision Log

**Format:** `D-###` · date · decision · rationale · status (`PROPOSED` / `ACCEPTED` / `REJECTED` / `SUPERSEDED BY D-###`). The log is append-only; a superseded entry keeps its full text.

| ID | Date | Decision | Rationale | Status |
|---|---|---|---|---|
| D-001 | 2026-09-26 | **The game is single-player at core.** Online (P7) is an additive layer over the same deterministic universe, never a fork of the game. | The core fantasy is *lone explorer*; multiplayer must enrich the map, not crowd it. The action-log save (§12.6) makes P7 structurally cheap. | ACCEPTED |
| D-002 | 2026-09-26 | **Newtonian physics with arcade assist (default ON), not toggle-off.** Inertia is the feel; damping + auto-brake are the forgiveness. Pure Newtonian and pure arcade both rejected. | Pillar 3 demands survival tension that *makes sense*; full Newtonian on iGPU-grade hardware with a mouse is a frustration engine; full arcade loses the signature flight identity. | ACCEPTED |
| D-003 | 2026-09-26 | **Home star is Tau Ceti** (real G8V, 11.9 ly from Sol); home station is **Meridian Station**; the home world is **Thessaly**; the player's ship is **Astra** (after the generation ship that founded the colony). | "Anchored by real celestial bodies" is a Pillar-1 promise that starts at home. Tau Ceti is Sun-like enough for "the sky I know, going dark" to land emotionally. | ACCEPTED |
| D-004 | 2026-09-26 | **One deterministic universe, hybrid generation: 430 real anchors + deterministic fill, seed `ASTRA-2026-09-26`.** | Real anchors give the map poetry and truth; deterministic fill makes it shippable on iGPU hardware and makes multiplayer a data problem, not a simulation problem. A fixed public seed means "the galaxy" is one place, for everyone, forever. | ACCEPTED |
| D-005 | 2026-09-26 | **Save format = seed + append-only action log (+ compaction checkpoints).** World = replay(seed, log). | Tiny saves, bounded corruption, free "time travel" debug, and the *entire* multiplayer sync layer for free. The single most future-ready decision in this document. | ACCEPTED |
| D-006 | 2026-09-26 | **Death policy:** respawn at last docked station; cargo jettisoned, 50% recoverable for 24 in-game h; 25% refill surcharge 6 h; **no XP/level/story loss.** | Pillar 3: tension must be survivable. XP loss is a rage-quit generator; cargo loss is *thematic* (you died, your stuff is out there) and is made fair by the salvage beacon. | ACCEPTED |
| D-007 | 2026-09-26 | **Exactly 7 planet types** (Terrestrial, Volcanic, Glacial, Desert, Ocean, Toxic, Barren). An 8th type requires a PILLAR-EXCEPTION review. | 7 types × (hazard × resource × POI) matrices already give the loop depth; an 8th biome on iGPU VRAM is a performance tax, not a feature. | ACCEPTED |
| D-008 | 2026-09-26 | **The Architects are never combat encounters.** All Architect presence is environment, ruin, and field. | Pillar 2/5: the Architects must remain *incomprehensible* to read as godlike. Making them shootable turns the game's central mystery into a boss. | ACCEPTED |
| D-009 | 2026-09-26 | **Phantom spawn rule: 5% per planet landing, rolled once at LZ generation and cached.** First guaranteed sighting by the 20th landing (soft ceiling). | Per-landing (not per-planet) keeps the fantasy "it follows *your* landings"; the cache prevents re-roll exploits and keeps it deterministic for replay/P7. The soft ceiling protects first-time players from "it never appeared" confusion. | ACCEPTED |
| D-010 | 2026-09-26 | **Two AAV moments only at launch:** the Siphon lensing pass and the Phantom encounter. All other effects are B-quality. | Pillar 5: unforgettable moments on an iGPU budget. Concentrating the FX budget (2 ms) on two moments beats spreading it across fifteen "okay" moments. | ACCEPTED |
| D-011 | 2026-09-26 | **Art: vertex colors over textures; total hero texture memory ≤ 4 MB; black is reserved for the Phantom.** | The aesthetic *is* the performance budget made visible. Black-reservation makes the Phantom's silhouette legible by training, not by explanation. | ACCEPTED |
| D-012 | 2026-09-26 | **Audio: real-time CPU synthesis for SFX; Opus streaming for music (5 tracks) and MERIDIAN VO (< 200 lines); HRTF spatialization, headphone-first.** | A thin, owned transport + one Opus codec is the lowest-RAM, highest-character path on this hardware. Sparse VO protects the voice budget and keeps MERIDIAN's lines earned. *Implementation backend set by D-017 / TDD T-003 (PortAudio + OpenAL Soft).* | ACCEPTED |
| D-013 | 2026-09-26 | **Tech stack: C++17 + Vulkan 1.1, custom engine (no engine license), Linux→MinGW-w64 cross-compile, portable folder distribution (launcher + exe, no installer).** | Every line of the game (incl. the save format and P7) depends on owning the toolchain; iGPU friendliness favors Vulkan 1.1 over newer feature sets; portable zip matches the audience (modest-PC explorers who are wary of installers). | ACCEPTED |
| D-014 | 2026-09-26 | **XP curve fixed: `1000 × L^1.5` per level; L50 total 7,248,703 XP; the five gated unlocks at L5/L10/L20/L45/L50 are the canonical ladder.** | The curve is a contract with the player's time (65–70 h to the Core). The five gates map 1:1 to loop expansions — every gate must *widen the loop*, not just raise a stat. | ACCEPTED |
| D-015 | 2026-09-26 | **Three endings (Restore / Sever / Commune), choice made once at the Core, no preview, no undo.** | Pillar 2: the mystery's payoff must be an *ethical* fork, not a difficulty fork. No-preview is the price of the choice meaning something; the cost disclosure replaces the preview. | ACCEPTED |
| D-016 | 2026-09-26 | **Star Atlas is exactly 3 layers** (Galaxy / Sector / Region). No 4th layer at launch. | UI clarity on modest hardware is a pillar-level concern (Pillar 1: the map is the promise). Zoom = layer keeps the mental model at one verb. A 4th layer (e.g., orbital) is a P8+ candidate, logged as an open question, not a promise. | ACCEPTED |
| D-017 | 2026-09-27 | **Audio backend = PortAudio (static) + OpenAL Soft (bundled DLL) + libopus, per TDD T-003.** Supersedes the custom-WASAPI implementation plan in §12.5 (the *design* of D-012 — CPU-synthesized SFX, Opus-streamed music/VO, HRTF — is unchanged). | Technical decision, owned by the TDD: PortAudio is the lowest-friction real-time callback under MinGW-w64 cross-compilation; OpenAL Soft provides the HRTF layer without a bespoke audio engine. §12.5 updated in place. | ACCEPTED |
| D-018 | 2026-09-27 | **Art Bible v1.0 adopted as visual authority.** Pointer decisions: **A-002** (world/biome palette extends the locked §9.3 master signal palette; Volcanic "black" = charcoal `#2A2226`, preserving the black-reservation rule), **A-003** (Exotic purple/warp reserved for Loom phenomena & warp space only), **A-005** (Phantasma form refined: tall thin humanoid 9:1, crouched many-jointed encounter pose, Cherenkov edge, tracking eye-points — §8.4 updated in place). | Visual decisions are owned by the Art Bible per the document-family rule; these three alter *design-relevant* looks (palette scope, a reserved color's usage, the Phantasma's form), so they are pointer-logged here. §8.4 and §9.3 updated in place. | ACCEPTED |
| D-019 | 2026-09-27 | **Phantasma encounter audio = 2 s ambient/music fade to silence + continuous 18 Hz infrasonic sine bed (felt not heard) + breathing at 3 s + one sub-bass note at 45–55 s at the creature's face. Per AudioDesign AU-002.** Refines the "4 s cut" in §8.4. | Audio design is owned by the audio document; the 18 Hz bed turns the encounter into a physiological event (the body knows before the ears). §8.4 and the Art Bible §4.2 sequence updated in place. | ACCEPTED |

---

## Appendix A — Controls

**Canonical (all remappable in Settings):**

| Action | Keyboard | Gamepad |
|---|---|---|
| Thrust / Pitch | W / S | Left stick |
| Strafe / Roll | A / D | Right stick |
| Yaw | Q / E | Left stick (X, soft) |
| Mouse look | Mouse | Right stick (Y, fine) |
| Brake (velocity damp) | Space | Right trigger (hold) |
| Overdrive | Shift (hold) | LT (hold) |
| Proximity lock | F | D-pad down |
| **Star Atlas** | F1 | View / Select |
| **Codex** | F2 | Touchpad / D-pad left |
| **Ship** | F3 | Options / D-pad right |
| Inventory | F4 | D-pad up |
| Crafting | F5 | LB |
| Missions | F6 | RB |
| Scan (hold) | Left click / X | X |
| Fire (Arc Lance) | Right click | RT |
| Warp (target locked) | T | Y |
| EVA in/out | G | B |
| Mining beam | C | Hold RT (surface) |
| Drop / pickup | R | LB+RB |
| Settings | Esc | Options |

Rules: no 3-key chords; gamepad has full 6-DOF without menus; every action discoverable within 5 min (tutorial maps each bind once).

## Appendix B — Glossary

| Term | Definition |
|---|---|
| **AAV** | Above-And-Beyond Visual — one of the two signature, over-budget moments (Siphon, Phantom) |
| **Anchor** | A real celestial body (of 430) whose position/class overrides procedural fill |
| **Arc Lance** | Base primary weapon |
| **Cartographer** | The player; the game's address for them |
| **Chunk** | 1 ly³ — the generation unit |
| **Core Key** | Act 3 story tool: Loomsteel + Iridium + Signal Fragment |
| **Courier Drone** | Neutral rotating trader, one per Sector |
| **cr** | Credits, the currency |
| **Deep Scanner** | L10 unlock: data-depth + Siphon readings |
| **LZ** | Landing Zone — the one generated touch-down area per planet |
| **Loom** | The Architects' galaxy-scale machine network |
| **Loom Gate** | Architect structure near a real black hole anchor; opens a deep-map ring |
| **Loomsteel** | Exotic resource; Architect alloy |
| **MERIDIAN** | Home-station AI; the only voiced character |
| **Meridian Station** | Home station in the Tau Ceti system |
| **Phantom Shards** | Phantasma drops; story-currency (12 exist) |
| **POI** | Point of Interest — a hand-placed, scannable unique |
| **Prime Loom** | The Loom's heart, at Sgr A* |
| **Quadrant** | 10,000 ly region of the disk |
| **Region** | 10 ly³ — the scannable neighborhood; carries the POI density contract |
| **Salvage Beacon** | Death-site cargo recovery marker (24 in-game h) |
| **Sector** | 1,000 ly³ — ~300 stars |
| **Signal Fragment** | One of 12 Watcher-given pieces of *the Signal* |
| **Siphon** | Artificial black hole; Loom node; AAV-1 |
| **Star Atlas** | The 3-layer map UI |
| **Thessaly** | The home world (Tau Ceti system) |
| **Warp Mk I/II/III** | 10 / 50 / 200 ly jumps (base / L20 / Act 3) |

## Appendix C — Real Anchor Points (Excerpt)

The full 430-row `anchors.json` (schema §6.2) is a Phase 1 deliverable. Representative anchors, by category:

**Home & story (5):** Tau Ceti (HOME), Proxima Centauri (the *Astra Beacon* drift site), Sol (Sol system — the old home, a story region, no colony), Cygnus X-1 (Loom Gate), Sagittarius A* (the Core / Prime Loom).

**Famous stars (≈ 90):** Sirius A, Sirius B (WD), Vega, Altair, Arcturus, Aldebaran, Betelgeuse (K-M supergiant, story-flavored "the Red"), Antares, Polaris, Deneb, Spica, Regulus, Fomalhaut, Canopus, Rigil Kentaurus, Barnard's Star, Gliese 876, Epsilon Eridani, 61 Cygni, Procyon, Achernar, Capella, Bellatrix, Mintaka, Alnitak, Saiph, Rigel, Naos, Alkaid, Dubhe, Alcor, Mizar, Hamal, Diphda, Menkar, Menkent, Gacrux, Mimosa, Acrux, Mirach, Alpheratz, Markab, Scheat, Algenib, Rasalhague, Kaus Australis, Peacock, Albireo, Castor, Pollux, Alhena, Mirfak, Alphard, Dschubba, Sargas, Nashira, Alsephina, Zosma, Kocab, Pherkad, Izar, Alkaid …

**Compact objects (≈ 40):** Sirius B (WD), Procyon B (WD), 40 Eridani B (WD), Van Maanen's Star (WD), PSR B1919+21 (the first pulsar), CRB Pulsar, Vela Pulsar, Cygnus X-1 (BH), A0620-00 (BH candidate), M87* (extragalactic, halo-region anchor, flavor-only), 47 Tucanae, Omega Centauri, M13, Pleiades (M45), Hyades.

**Nebulae & structures (≈ 60):** Orion (M42), Crab (M1), Ring (M57), Helix (NGC 7293), Lagoon (M8), Trifid (M20), Eagle (M16), Cat's Eye (NGC 6543), Horsehead (B33), Veil, Cygnus Wall, Carina Nebula, Tarantula (LMC — halo flavor), Rosette, Bubble (NGC 7635), Flame, Witch Head, Horsehead, Omega Cen (cluster+nebula), NGC 869 (Double Cluster).

**Galaxies (halo/sky-region anchors, ≈ 30):** Andromeda (M31), Triangulum (M33), Whirlpool (M51), Sombrero (M104), Pinwheel (M101), Bode's (M81), Cigar (M82), Oval (M32), Heart (M58), Black Eye (M64), Lily (M106), Fireworks (M95), Bode's Companion, LMC, SMC, Maffei I/II, Holmberg II, NGC 253, Sculptor (NGC 253/SMC region), Ursa Minor dwarf …

**The rest (≈ 205):** a curated long-tail of named stars, WDs, pulsars, and nebulae, chosen for (a) sky-coverage (no Quadrant without ≥ 8 anchors), (b) sky-lore density (arms get the famous names; the halo gets the strange ones), (c) story hooks (≈ 20 with `story: STORY` flags feeding Codex flavor).

## Appendix D — Full XP Table

`XP(L→L+1) = 1000 × L^1.5` (rounded). Total L1→L50 = **7,248,703**.

| Level | XP to next | Cumulative |
|---:|---:|---:|
| 1 | 1,000 | 1,000 |
| 2 | 2,828 | 3,828 |
| 3 | 5,196 | 9,024 |
| 4 | 8,000 | 17,024 |
| 5 | 11,180 | 28,204 |
| 6 | 14,697 | 42,901 |
| 7 | 18,520 | 61,421 |
| 8 | 22,627 | 84,048 |
| 9 | 27,000 | 111,048 |
| 10 | 31,623 | 142,671 |
| 11 | 36,483 | 179,154 |
| 12 | 41,569 | 220,723 |
| 13 | 46,872 | 267,595 |
| 14 | 52,383 | 319,978 |
| 15 | 58,095 | 378,073 |
| 16 | 64,000 | 442,073 |
| 17 | 70,093 | 512,166 |
| 18 | 76,368 | 588,534 |
| 19 | 82,819 | 671,353 |
| 20 | 89,443 | 760,796 |
| 21 | 96,234 | 857,030 |
| 22 | 103,189 | 960,219 |
| 23 | 110,304 | 1,070,523 |
| 24 | 117,576 | 1,188,099 |
| 25 | 125,000 | 1,313,099 |
| 26 | 132,575 | 1,445,674 |
| 27 | 140,296 | 1,585,970 |
| 28 | 148,162 | 1,734,132 |
| 29 | 156,170 | 1,890,302 |
| 30 | 164,317 | 2,054,619 |
| 31 | 172,601 | 2,227,220 |
| 32 | 181,019 | 2,408,239 |
| 33 | 189,571 | 2,597,810 |
| 34 | 198,252 | 2,796,062 |
| 35 | 207,063 | 3,003,125 |
| 36 | 216,000 | 3,219,125 |
| 37 | 225,062 | 3,444,187 |
| 38 | 234,248 | 3,678,435 |
| 39 | 243,555 | 3,921,990 |
| 40 | 252,982 | 4,174,972 |
| 41 | 262,528 | 4,437,500 |
| 42 | 272,191 | 4,709,691 |
| 43 | 281,970 | 4,991,661 |
| 44 | 291,863 | 5,283,524 |
| 45 | 301,869 | 5,585,393 |
| 46 | 311,987 | 5,897,380 |
| 47 | 322,216 | 6,219,596 |
| 48 | 332,554 | 6,552,150 |
| 49 | 343,000 | 6,895,150 |
| 50 | 353,553 | 7,248,703 |

## Appendix E — Resource List

| Tier | Resource | Found in | Primary use |
|---|---|---|---|
| Common | **Ferrite** | Barren, Volcanic, belts | Hull repairs, most structures |
| Common | **Silica** | Volcanic, Desert, belts | Reactor housing, glasswork |
| Common | **Graphene** | Terrestrial, Toxic, belts | Lightweight plating, PD drones |
| Common | **Water (H₂O)** | Terrestrial, Glacial, Ocean | O2 cells, thermal gel, habitats |
| Uncommon | **Cobalt** | Glacial, Ocean, Volcanic | High-heat components, scoop upgrades |
| Uncommon | **Cryon** | Glacial | Coolant, thermal shielding |
| Uncommon | **Phosphor** | Ocean, Terrestrial | Biolume systems, signal beacons |
| Uncommon | **Argon** | Toxic, Desert | Shield gas, welding, station refit |
| Rare | **Iridium** | Volcanic, Toxic, deep Barren | Core Key, endgame modules |
| Rare | **Harvest Crystal** | Harvester kills (20%), harvest sites | Advanced weapon crafting |
| Exotic | **Loomsteel** | Architect ruins, Escorts (10%), deep Barren | Core Key, Loom Interface |
| Exotic | **Phantom Shard** | Phantasma drops only (12 exist) | Core Key, ending gates, Codex |

**Balance rule (checked in CI, Phase 4):** no recipe's total input mass sells for more than its station market value at base prices; exotic resources have no market value at all (they are *knowledge*, not wealth).

---

*End of document — v1.0 baseline. Next review: Phase 1 exit (anchors.json audit). All future changes through the Decision Log (§17).*
