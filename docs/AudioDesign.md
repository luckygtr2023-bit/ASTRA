# Project Astra Cosmos — Audio Design Document

| | |
|---|---|
| **Document** | Audio Design Document — the complete sound design & audio implementation spec |
| **Version** | 1.3 — Family-aligned (3rd audit pass) |
| **Date** | 2026-09-27 |
| **Status** | **Authoritative for all audio decisions.** |
| **Companion** | [GDD](GDD.md) §10 (audio intent) · [TDD](TDD.md) §2.9 (audio plumbing: PortAudio/OpenAL/Opus on core 7) · [Art Bible](ArtBible.md) §4 (AAV audio moments). Conflict rule: this document wins on *how it sounds*; the TDD wins on thread/latency plumbing; the GDD wins on design intent. Audio decisions are logged as `AU-###` in §10; design-affecting ones get pointer entries in the GDD decision log. |

---

## Table of Contents

0. [Document Control](#0-document-control)
1. [Audio Philosophy & Strategy](#1-audio-philosophy--strategy)
2. [Audio Engine Architecture](#2-audio-engine-architecture)
3. [Spatialization](#3-spatialization)
4. [Sound Design — Synthesized SFX](#4-sound-design--synthesized-sfx)
5. [Music & Dialogue (Opus Streaming)](#5-music--dialogue-opus-streaming)
6. [Mix, Loudness & Platform](#6-mix-loudness--platform)
7. [Resource Management & Budgets](#7-resource-management--budgets)
8. [SFX Category Index (complete)](#8-sfx-category-index-complete)
9. [Audio Production Plan (by Phase)](#9-audio-production-plan-by-phase)
10. [Audio Decision Log](#10-audio-decision-log)
- [Appendix A — Voice Slot Priorities](#appendix-a--voice-slot-priorities)
- [Appendix B — Per-Biome Ambient Recipes](#appendix-b--per-biome-ambient-recipes)
- [Appendix C — HRTF Specification & Validation](#appendix-c--hrtf-specification--validation)
- [Appendix D — Opus Stream Manifest (example)](#appendix-d--opus-stream-manifest-example)

---

## 0. Document Control

### 0.1 Change Process

1. Propose `AU-###` (date, decision, rationale, status `PROPOSED`).
2. Decisions that change *perceived game behavior* (the Phantasma moment, a story audio cue) get a pointer entry in GDD §17.
3. On acceptance, affected prose updates in the same commit; version bumps.

### 0.2 Version History

| Version | Date | Summary |
|---|---|---|
| 1.0 | 2026-09-27 | Baseline. Philosophy (synthesis over storage), engine architecture (core 7, 64 voices, HRTF, 100 ms Opus rings, **50 MB RAM cap**), spatialization (CPU HRTF + Doppler), full sound design specs (engines by ship class, warp/docking/deflect, combat, environmental incl. pulsars, radio static, ice crackle, lava bubbling, gravity distortion, **Phantasma 18 Hz bed + 2 s silence** §4.6, UI/HUD incl. scan-complete), music (5 adaptive Opus tracks) + dialogue (MERIDIAN, 12 Signal Fragments), mix/loudness, complete SFX index, phase plan, decisions AU-001…AU-012. |
| 1.1 | 2026-09-28 | Re-issued & verified. Full re-audit against the audio brief (24/24 parameters confirmed in place). Internal-consistency fixes: §2.4 RAM table arithmetic corrected (Opus rings 525 KB, synth state 544 KB, **design total ≈ 1.9 MB**, not ≈ 1.7 MB), §2.3/§7 ring figure corrected to ≈ 0.5 MB, §8.1 engine voice priority reconciled with Appendix A (P1, not P0), AU-007 figure updated; cross-document version refs (GDD §10, TDD §2.9/T-013) bumped to v1.1. No design changes. |
| 1.2 | 2026-09-28 | Re-issued & verified (2nd audit pass — cross-reference & storage-math audit). Fixes: **5 dangling/mislabeled cross-refs** (acid-rain particle ref → Art Bible §6.2; AU-003 brownout → GDD §5.5; gyro OQ → GDD §16 OQ-6; HRTFProfile storage → TDD §3.2 `astra.cfg`; settings surface → TDD §3.2 — TDD has no §11/§12/§16), **music storage math corrected** (15 stems × 3 min at 64 kbps = 21.6 MB, which would have broken the ≤ 20 MB CI cap; music re-based to Opus mono **32 kbps** — transparent for ambient drone material — giving ≈ 16 MB total; §5.1, §7, §9 P8 gate ≤ 20 MB, Appendix D manifest), TOC anchors verified against the live GitHub markdown renderer (em-dash `--` fragments correct as written). No design changes. |
| 1.3 | 2026-09-28 | Family-aligned (3rd audit pass — GDD/TDD/Art Bible reconciliation against this document). GDD §10: music 32 kbps / ≈ 11 MB (was 64 kbps / < 8 MB), MERIDIAN ≈ 3.2 MB (was < 12 MB), sub-bass note at the creature's *face* (was "position"), Watcher audio = 120 Hz shimmer + 3-note lattice chime; GDD §12.5: three Opus decoders + float rings (was 2 decoders + 16-bit downmix), custom CPU HRTF filter stage named. TDD §2.9: music 32 kbps, DSP-budget pointer → TDD §3.3, OpenAL = positioning layer (HRTF stage is this document's CPU DSP). Art Bible §4.1: Siphon fallback audio named per §4.4. **New:** absorbed the Art Bible's gaze-response cue (bed pitch −10% when the player looks away) into §4.6/§8.5 — the audio document is now a superset of every audio claim in the family. Version refs bumped (GDD §10, TDD §2.9). |

---

## 1. Audio Philosophy & Strategy

### 1.1 The Core Principle: Synthesis Over Storage

> **Real-time CPU synthesis for infinite variation, zero storage cost.**

**Implementation: algorithmic generation for engines, pulsars, radio, ambience — Opus streaming only for music and dialogue.**

The game's sonic identity is *computed*, not recorded. Every sound the player hears that is **not music or a human voice** is generated in real time by a small DSP engine on the audio thread:

| Generated (synthesized, zero storage) | Streamed (Opus, the only storage) |
|---|---|
| Engines, thrusters, warp, docking | Music — 5 atmospheric tracks (GDD §10) |
| Pulsars, black-hole drones, gravity distortion | Dialogue — MERIDIAN VO (< 200 lines) |
| Radio static & the Architects' signal broadcasts | 12 Signal Fragments (the Architects' voice, ~40 s each) |
| All ambience (wind, ice crackle, lava bubbling, storms) | Cutscene one-shots (if any — none planned, A-011) |
| All combat, UI, and event SFX | — |

**Why synthesis is the strategy, not a shortcut:**
1. **Infinite variation from tiny memory.** A seeded noise generator + filters produces a unique-sounding engine note on every run, every planet, every ship class — from a recipe table of a few hundred bytes. Storage-based SFX would need megabytes to fake this variety; synthesis *is* the variety.
2. **Determinism is free.** Synth functions are pure: `sound = f(seed, state, t)`. Audio becomes a function of the action log (GDD §12.6) — replays and multiplayer (Phase 7) reproduce sound exactly, and the 10 h music-loop test is the only place storage touches time.
3. **The iGPU's memory belongs to the world.** Zero SFX storage keeps the shipped zip small (TDD §5.3) and the RAM budget honest (§7).
4. **Parameter-driven design.** A sound is a *function of game state* — throttle, damage, distance, biome, story act — so the audio responds to the player with the same resolution as the visuals. A recorded sample can only be pitched and faded; a synthesis recipe can *change shape*.

### 1.2 The Acoustic Vantage: the Ship

Sound does not travel in vacuum — so the game's audio has a defined **vantage point**: *everything in space is what the Astra's hull sensors, radio, and systems hear*. That single decision resolves the "sound in space" problem and grounds the specs:

- **Doppler is always ship-relative** (§3.2) — the ship moves through the field, so the field shifts.
- **The vacuum is never dead-silent** — the ship's own systems (engine, hum, creaks, valve ticks) are always audible; what changes is the *world* layer.
- **On a surface**, the vantage shifts to the suit: the helmet hears the world directly (wind, ice, lava) with the ship's systems ducked under it.
- **Radio is a system, not a speaker** — static bursts, signal fragments, and Watcher answers all arrive *through the radio* (filtered, delayed, with carrier bleed), which makes the mystery diegetic: the player is literally listening to the galaxy on a frequency.

### 1.3 Strategy Rules

1. **Two textures, no third:** synthesized (infinite, responsive, quiet) vs. streamed (composed, fixed, loud). Music and MERIDIAN are the only *fixed* things in the soundscape — that contrast is what makes them feel real.
2. **Silence is a material.** The rarest and most powerful audio event in the game is *absence* (the Phantasma, §4.6; the Compact Sky Event, Art Bible §4.3). Budgets protect silence the same way the Art Bible protects black (Art Bible Principle 3).
3. **One sound per story truth.** Every major system has exactly one audio signature (Siphon = the 18 Hz-family sub-bass + sheared drone; Watcher = the lattice chime; Harvester = the drill whine; MERIDIAN = warm mono voice). The player learns the galaxy by ear before they can name it.
4. **Readable before felt** (mirrors Art Bible weather rule): hazards are *heard* from range (a drill at 2 km, a storm at 30 s) before they are seen or touch the player.

---

## 2. Audio Engine Architecture

### 2.1 Thread & Callback (affirms TDD §2.9)

- **Audio thread = core 7** (TDD T-002), `TIME_CRITICAL`, runs the **PortAudio callback at 48 kHz, 256-sample period (~10.67 ms)**. Synthesis, Opus decode, and spatialization **all execute on this thread** (per spec) — nothing audio blocks on main, and nothing but audio runs here (the P7 opset receive is the same thread's *other* duty, scheduled in the callback's dead time).
- **DSP graph (per frame):**

```
 64 VOICE SLOTS (stealable, priorities in Appendix A)
   ├── synth voices      (engines, ambience, SFX, pulsars, radio, phantom bed)
   ├── opus voices       (music stems ×≤3, VO, signal fragments)
   └── player-locked     (breathing, suit creak — never spatialized)
        │
        ▼  per voice: source → level → (3D: distance · panner · HRTF · doppler)
        │
 8-DIR HRTF BUS (CPU, Appendix C)  →  reverb-lite (2048-sample conv, scene-tuned)
        │
 MASTER: music/ambient/SFX buses → ducking → limiter → PortAudio out
```

- **Voice management:** 64 slots (TDD §2.9). Stealing is priority-ordered (Appendix A): P0 voices (Phantasma bed, player breathing, player weapon, warp) are **never stolen**; P3 voices (decorative one-shots) yield first. Steal = 30 ms fade-out, slot re-armed immediately.
- **Latency budget:** end-to-end < 20 ms (256-sample period + PortAudio + 100 ms Opus chunk alignment for VO only). UI click → speaker is a hard-feel target; weapon fire → sound < 1 frame.
- **Determinism:** all synth recipes consume PRNG streams salted `AUDIO` (TDD §2.2 NFR-1) — a replayed session sounds identical. UI sounds use *no* stream (fixed recipes, AU-010).

### 2.2 CPU Budget (affirms TDD §2.3: audio ≤ 1.5 ms/frame)

| Block | Worst case | Budget |
|---|---|---:|
| PortAudio callback I/O | 64 slots, 256 samples | 0.2 ms |
| Synthesis | 24 active synth voices (noise gen + filter + env) | 0.6 ms |
| Opus decode | 8 streams, 100 ms chunks amortized | 0.3 ms |
| HRTF + spatialization | 32 active 3D sources | 0.3 ms |
| Mix + ducking + master chain | 4 buses + limiter | 0.1 ms |
| **Total** | | **≤ 1.5 ms** |

Synth voice cost is dominated by noise generation (2048-sample noise LUT per voice, generated once, looped with LFO offset — never generated per-sample from a PRNG on the hot path).

### 2.3 Opus Streaming (per spec)

- **100 ms chunks decoded into ring buffers.** Pipeline per stream: `file (assets/, ZSTD container) → Opus decoder → 100 ms PCM frame (stereo float) → 2-frame ring buffer (2 × 38.4 KB) → mix`. The consumer pulls frames; the decoder refills on the audio thread when ring < 1 frame.
- **Low latency:** worst-case added latency = 1 chunk (100 ms) for *music* (inaudible); for **VO** the ring is pre-primed (next 3 lines of a conversation decoded ahead) so line start is < 30 ms.
- **All active streams share the 50 MB RAM cap** (§7) — worst-case concurrent: 3 music stems + outgoing track (crossfade) + 1 VO + 1 fragment ≈ 7 streams ≈ 0.5 MB of rings. The cap is a guardrail, not a pressure point.
- **Gapless & adaptive:** loop points are pre-marked in the Opus headers (Appendix D manifest); stem transitions are 4 s crossfades with gain automation (AU-009). No stream ever pops: a fade-in/out floor of 8 ms is hardware-enforced in the ring writer.

### 2.4 Resource Management (per spec: **total audio RAM < 50 MB**)

| Block | Size |
|---|---:|
| PortAudio double ring (2 × 256 smp stereo float) | 4 KB |
| Opus stream rings (worst 7 streams × 2 × 100 ms × stereo float) | 525 KB |
| Opus decoders (3 instances: music / VO / fragments) | 240 KB |
| HRTF per-voice delay + filter state (64 voices) | 300 KB |
| HRTF coefficient tables (8 azimuth × 2 ears × 2nd-order) | 2 KB |
| Synth voice state (64 × 512 B = 32 KB) + noise LUTs (64 × 8 KB shared pool = 512 KB) | 544 KB |
| SFX recipe/preset tables (all of §4, incl. biome tables) | 256 KB |
| Reverb-lite convolution impulse (2048 smp stereo) | 16 KB |
| Ducking/transition automation buffers | 64 KB |
| VO pre-prime buffer (3 lines × ~8 KB) | 24 KB |
| **Total in use (design)** | **≈ 1.9 MB** |
| **Hard cap (AU-007)** | **< 50 MB** — includes synthesizer memory per spec; headroom reserved for per-planet ambient variation tables (P3+), P7 presence audio, and any future cutscene one-shots |

This cap **refines GDD §12.2's 100 MB audio allocation** and sits inside the TDD's 0.5 GB Audio/IO pool (T-006 → T-013 pointer). Debug builds assert the cap per frame (watermark logging, TDD §3.5).

---

## 3. Spatialization

### 3.1 CPU-Based HRTF (per spec)

- **8-directional HRTF filter set** (azimuth 0/45/…/315°), 2nd-order IIR per ear, applied per 3D source on the audio thread (TDD §2.9, GDD §10). **Headphone-first:** full binaural when the output device is 2-channel stereo; **speaker mode** (auto-detected: > 2 channels, or user toggle) falls back to azimuth panner + elevation via level only — positional information is never *lost*, only simplified.
- Elevation is derived from source height relative to the ship/suit reference plane (−30°…+60°), mapped to a 3-level HRTF blend set (low / eye / high).
- **Head tracking: none at launch** (the vantage is the ship/suit — GDD §16 OQ-6 gyro question stays open; the HRTF profile shape, `HRTFProfile` (carried in `astra.cfg` per TDD §3.2), is already per-user-ready).
- Full spec + blind-position validation: Appendix C.

### 3.2 Doppler (per spec: from relative velocity)

- `f' = f · c / (c − v_r)` where `v_r` = **radial relative velocity between source and ship** (both velocities known to the physics sim; the audio thread reads the interpolated per-source vector each frame).
- **Capped at ratio ∈ [0.5, 1.5]** — beyond that, Doppler reads as a glitch, not motion; the cap preserves musicality for weapons and storms.
- **Applied to:** weapons & impacts, enemy attacks, storms (approaching fronts), pulsars (flying through a pulsar's pulse field is a classic — the thud rate *sweeps* as you pass), the Siphon drone (§4.4).
- **Never applied to:** UI, music, breathing, docking beeps (docking beeps encode *gap distance*, not motion — beeps are the instrument).
- Vacuum justification: the ship-vantage principle (§1.2) — Doppler is what the hull sensors measure.

### 3.3 Distance, Zones & Culling

| | Surface | Space (helm vantage) |
|---|---|---|
| Reference distance | 1 m | 10 km (scaled to scene) |
| Roll-off | −40 dB per doubling (log curve) | −30 dB per doubling (sensors "hear" further) |
| Near clamp | 0.5 m | 0.5 km |
| Far cull (LOW/BAL/HIGH) | 200 / 500 / 800 m | 20 / 50 / 100 km |
| Zone behavior | < 10 m: full detail; 10–100 m: base layer only; beyond: bed only | < 10 km: full; beyond: bed + Doppler only |

---

## 4. Sound Design — Synthesized SFX

### 4.1 Ship Engines (per spec: noise generator modulated by throttle; smooth pitch & volume; varies by ship class)

**Base recipe (all classes):** `filtered noise (2048-sample LUT loop, LFO-offset) → bandpass (cutoff = f(throttle)) → gain = g(throttle)` + a low rumble oscillator (per class) + RCS burst layer. **All parameter changes pass a 50 ms-attack / 200 ms-release one-pole smoother** — no zipper noise, no throttle steps; the engine *breathes* with the throttle.

| Ship class | Noise base | Bandpass sweep (0→100% throttle) | Rumble | Character (what the player learns) |
|---|---|---|---|---|
| **Astra (player, sleek)** | pink noise | 300 → 2,400 Hz | 55 Hz sine, −18 dB | bright, fast "hi-zoom"; overdrive adds a 1.5 kHz whistle, +10% pitch, +3 dB |
| **Courier Drone (fat, friendly)** | brown noise | 80 → 600 Hz | 40 Hz sine, −12 dB | slow hum, almost a purr |
| **Harvester worker** | brown noise | 150 → 900 Hz | 35 Hz + 0.7 Hz LFO wobble | grown drone, slightly *wrong* rhythm |
| **Harvester escort** | brown noise | 100 → 500 Hz | 30 Hz + chitin rattle (granular ticks, seeded) | heavier, rattling, close-quarters dread |
| **Harvester siege** | brown noise | 60 → 250 Hz | 25 Hz, −6 dB | sub-audible presence; you feel it in the hull (the 25 Hz sits at the felt/heard border — deliberate) |
| **Watcher** | — | — | 120 Hz sine, −24 dB, AM 0.1 Hz | near-silence; a shimmer that means "something is watching" (passive cue, never alarm) |
| **Derelict** | — | — | none | dead — silence + debris impacts only |

- **RCS bursts:** 400–900 Hz filtered-noise puffs (60 ms), panned by jet direction (port/starboard/fore/aft = 4 fixed HRTF slots, not full 3D — cheap and precise).
- **Brake (Space key, TDD §5.1):** a 200 ms hiss-down through the bandpass (throttle-independent — braking is audible even at idle).
- **Idle at dock:** 12% throttle floor — the ship is never fully silent (the ship-vantage hum is the player's "alive" signal; MERIDIAN's station is the *other* always-on warm layer).

### 4.2 Warp, Docking, Deflect (the "ship actions" set)

- **Warp jump (45 s Mk I):** Shepard-tone rise (log sweep, 220 → 1,760 Hz over 40 s) + rising noise sweep (the tunnel) + **the fold** at 40 s (200 ms of band-limited white → 80 Hz) + **arrival chime** — a 3-note motif seeded by the *destination region ID* (every system of arrival has its own chime; the player starts to recognize places by ear). Mk II/III: same shape, +1 octave headroom, arrival chime gains a 4th note.
- **Docking:** approach **beeps** (gap-distance instrument: 1 Hz at 2 km → 4 Hz at 50 m, Star-Amber "warm" timbre — the only beeps in the game; they *are* the HUD's spatial sense), **clamp servo** (2-frame metallic impact + 80 ms hiss), **pressurization** (air rush through a formant-filtered noise — the station's "inhale"), **undock** (the reverse exhale, −6 dB).
- **Deflect / "shield" SFX (AU-003):** the GDD has **no shield system** (§5.11: "Hull is the shield"), so the brief's "shields" category is realized as the game's actual energy-defense events: **PD drone intercept** (a crisp 2.2 kHz ping + 40 ms decay — the L25 drone's "tag"), **energy brownout** (all power sounds flicker 2 × 40 ms — the systems-dying stutter, GDD §5.5), **hull glancing deflection** (metallic slide + tick, 60% of an impact's volume). No shield-wall hum exists — a hum would imply a system the game doesn't have.
- **Fuel scoop (L5):** vacuum-suction bed (low-pass noise, 200 Hz) + particle clatter (granular, seeded) + a slow "fill" pitch rise as the tank approaches 100% (the scoop *sounds full* when it's full).
- **Scanner hum:** during a scan, a per-class timbre drone (Appendix A voice P1) — the scan has a *voice* (GDD §10), and the voice is different per object class (star ≠ nebula ≠ Siphon).

### 4.3 Combat

- **Arc Lance (player):** seeded per shot — pitch ±3%, timbre ±5% (two parallel noise paths with independent LFO), 60 ms body + 150 ms tail. The seed is the shot index within the action log → replays are sound-identical.
- **Ion Lattice (L35):** deeper (−4 semitones), slower (1.5/s), a 30 ms compression "thwip" before the body (pierce read).
- **PD drone ping:** as §4.2; + a 0.5 Hz "drone idle" hum when the player is under fire (dread layer, P2 voice).
- **Explosions:** three sizes (small/med/large) × two media — **vacuum** (no boom: metal crunch + debris granular + a *pressure wave in the hull* 0.3 s later — you feel the explosion through the ship) vs. **surface** (sub-bass thud + rumble + crackle, distance-rolled). Enemy deaths in vacuum are *small* (the universe is not a firework show); player-visible feedback comes from the hull-wave, which is more honest and more unsettling.
- **Harvester attacks:** **drill whine** (the signature — 900 → 1,400 Hz sine + 40 Hz rattle, ramps up 500 ms before a strike — *readable before felt*, §1.3.4), **flak** (sharp crack + 60 ms shrapnel granular), **rail spike** (compression whoosh, 200 ms, *then* impact — the whoosh arriving before the spike is the tension), **nest chitin creak** (slow 60–200 Hz groans, proximity-driven — the nest is *alive*).
- **Player damage:** hull impact = directional thud (HRTF-positioned at impact vector) + 1 tick alarm; **O2 breach** = the suit hiss (high-pass noise, continuous while breached — the oxygen meter's sound, GDD §5.5); suit breach = hiss + the valve tick accelerating.

### 4.4 Environmental

- **Wind:** biome-filtered noise bed, per-planet seed (the same planet always sounds the same — the audio's determinism promise), direction from physics (a left wind pans left through the HRTF). Per-biome recipes: Appendix B.
- **Ice crackle (Glacial):** event-driven pops — 500 Hz–2 kHz click + 80 Hz thud, 10–40 s apart (seeded), density rises with the temperature meter's danger zone. A glacier calving (rare, 1 in 30 landings on Glacial): 3 s of rolling sub-bass + the visual of the shelf falling — one of the game's quiet showpieces.
- **Lava bubbling (Volcanic):** granular pops (20–200 Hz, seeded density by distance-to-river) over a 30 Hz rumble bed; a river crossing plays the pops *underfoot* (low-mid emphasis) — you hear the lava you are walking past.
- **Gravity distortion (Siphon proximity — the audio signature of AAV-1):** three layers: (1) a **sub-bass drone** (28 Hz sine + 56 Hz harmonic, fades in over 2 km of screen space), (2) **sheared nearby sources** — every 3D source within 5 km gets its pitch *bent* by up to ±15% as your bearing crosses the Siphon (the audio does what the lensing pass does visually — the sound field *folds*), (3) a **fold tick** the instant the visual bend crosses 25% screen radius (a single 1 kHz, 30 ms, −24 dB tick — the player learns: *fold tick = I am close to the edge*). No music plays within the field (the Loom track ducks out; the drone is the music).
- **Pulsars (celestial field; AU-004):** a carrier sine (800–1,600 Hz, chosen by the neutron star's anchor class/temperature) **amplitude-modulated by a pulse gate at rate `f = 1/period`** — per the brief's example, a **30 ms period = 33 Hz** modulation. The result is **rhythmic and distinctive**: a star that keeps time. Perceptual banding by period: **> 80 ms (< 12.5 Hz) = discrete thuds** (a slow pulsar is a *heartbeat in the sky*); **30–80 ms = fused buzz/tremolo** (the 33 Hz case — a *buzzing star*, the most uncanny band); **< 30 ms (> 33 Hz) = a solid tone with a shimmer edge**. Approaching one, Doppler sweeps the pulse rate (§3.2 — the heartbeat *quickens* as you fly past; the cap glides, Appendix C). The 430-anchor **real** pulsars use their **real periods**: PSR B1919+21 (1.337 s → 0.75 Hz — felt more than heard) and the Vela Pulsar (89 ms → 11 Hz) are audible in-game at their real sky positions. Voice: P2 bed at 20 km, rising to P1 within 2 km.
- **Radio static (per spec: white noise bursts triggered by event flags):** the radio bus (band 400 Hz–3.2 kHz, carrier bleed −30 dB) fires a **white-noise burst (0.5–2 s, seeded shape)** when a **story/scan event flag** is set — flags defined by the story system: `SIGNAL_HEARD`, `BEACON_DRIFT`, `WATCHER_REPLY`, `LOOM_GATE_OPEN`, `COMPACT_EVENT` (the Act 2 sky event starts with 10 s of static before MERIDIAN's line — the galaxy's noise giving way to one voice). The Architects' broadcasts (Signal Fragments) are the *same* bus, formant-shaped to read as **almost-words** — the player's brain does the rest (GDD §10: "the player hears the mystery before they can read it").
- **Others (recipes in Appendix B):** radiation storm (whine shimmer, 6–9 kHz, amplitude-modulated at the storm's front speed), sandstorm (low rumble + grain, the wind recipe's Desert variant at 3×), acid rain (sizzle — high-pass noise bursts on "drop" events, Art Bible §6.2's acid-green rain lines (`#A8E83A`) each carry a micro-sizzle at BALANCED+), ocean squall (dark rain + distant lightning: 40 Hz crack, 120 ms delay for the "far" strike), micrometeor shower (whistling streaks — 2–4 kHz descending sweeps, seeded count + HRTF positions, then impacts), aurora hum (Glacial night, 90 Hz, −28 dB — the only "beautiful" environmental sound), geothermal vent (hiss, proximity), Watcher lattice chime (0.1 Hz, 3-note biolume shimmer — the Watcher's signature, §1.3.3), salvage beacon (deterministic 2-note ping, 1 Hz — the sound of your own lost cargo calling).

### 4.5 UI & HUD

- **Design rule (AU-010): UI is constant, world is variable.** UI sounds use *fixed* synth recipes (no seed variation) — the same button must always sound the same, or the interface feels broken. All variation lives in the world.
- **Button click:** soft 1.8 kHz tick, 30 ms, −18 dB (a fingertip, not a keyboard).
- **Menu navigation:** 40 ms air-whoosh between categories (band-pass noise, 800 Hz), −24 dB.
- **Confirm / cancel:** 2-note up (660 → 880) / 1-note down (440, 80 ms).
- **Scan complete (the "scan voice," per class):** star = bright single ping (1.2 kHz); planet = 2-note (520 → 780); moon = 1 note (520); asteroid = soft double-tick; nebula = 3-note shimmer chord; **derelict = 2 notes + a 200 ms "log tape" flutter** (you found a story); **Watcher = the lattice chime** (§4.4); **Siphon = 3 low notes (110/165/220, 400 ms spread — the sound of depth)**; **Phantasma = silence, then one high tone (1.76 kHz, 800 ms, −12 dB) — the only scan that "answers" with something almost human**.
- **NEW codex entry:** the Biolume-Cyan two-note chime (the game's dopamine sound, Art Bible §7.1) — 784 → 1,175 Hz.
- **Level up:** rising 4-note (pentatonic, seeded *per level* — level 5 always sounds the same, level 20 always sounds the same — progression is a lullaby the player has learned).
- **Warnings (the meters' heartbeat, GDD §10):** hull < 20% = 1 Hz hull tick; O2 low = the suit's **valve tick accelerating** with O2 (the oxygen meter's sound is the valve — the meter you can *hear*); fuel < 30% = none (fuel's scarcity is the *absence* of scoop/warp capability, heard as silence); energy brownout = the flicker (§4.2); temperature extreme = a 0.5 Hz "thermal groan" in the hull.
- **Death:** one low tone (98 Hz, 1.2 s, no sting — death is quiet, the respawn station is loud). **Respawn:** a soft 3-note return chord (the station's "you're back" — warm, MERIDIAN-adjacent).
- **FPS/system readouts:** none. Numbers do not make sounds.

### 4.6 The Xenothrix Phantasma — the Encounter (per spec; AU-002 canonical)

The game's most important sound design is an **absence with a pulse**. Parameters: *ambient fades to silence over 2 s; a deep 18 Hz infrasonic sine (felt, not heard); real-time synthesis on the audio thread.*

| Element | Spec |
|---|---|
| **The silence (t=0–2 s)** | On encounter lock: **all ambient beds + all music stems fade to digital silence over 2.0 s** (linear; the floor is true 0 dBFS — no low-end bleed, no "room"). The fade is the first beat: the game's feedback systems *stop* |
| **The 18 Hz bed (t=2 s → t≈58 s)** | A single **18.0 Hz sine oscillator, synthesized in real time on the audio thread** (core 7) — the cheapest voice in the engine, and the most important. Level: 18 Hz sits **below hearing threshold** on laptop speakers (≈ −20 dB relative to master); on headphones it is a **chest pressure, not a pitch** — *felt not heard*. A **36 Hz harmonic at −30 dB** (re-voiced to −20 dB in speaker mode, §6) guarantees the cue exists on every hardware class. Fade-in 0.5 s at t=2 s; continuous; fade-out 2 s as it leaves. **Gaze response:** when the player looks away from the face, the bed's pitch dips 10% (18.0 → 16.2 Hz, 200 ms glide) and back — it is still watching; you only know because the sound does (Art Bible §4.2 eye-points) |
| **Voice slot** | A **dedicated P0 slot — never stolen** (Appendix A). If the Phantasma is present, that voice is reserved before anything else plays |
| **Breathing (t=3 s)** | The player's own breathing (seeded noise + bandpass, player-locked 2D) returns — the first sound after the silence is *yourself*. Rate tracks the O2 meter (GDD §5.5); during the encounter it is held at a slow, controlled 4/s (the player's calm is the game's instruction) |
| **The replay (t=15–45 s)** | **Silent.** The first-ever scan replayed inside its body makes *no scan sound* — the one scan in the game that doesn't chime. Even that is being watched |
| **The note (t=45–55 s)** | One sub-bass note: 55 Hz sine + 110 Hz harmonic, 1.5 s, 50 ms attack, **HRTF-positioned at the creature's face** (Art Bible A-013 — where you've been looking, or where you *were* looking). It is the only melodic event in the encounter; everything else is bed and breath |
| **The return (t=55–60 s)** | Asymmetric (AU-002): bed + breathing out in 2 s; **the ambient comes back in 4 s** — silence is entered fast and left slowly. The world re-asserts itself around the dropped **Phantom Shards** (a single, small, high crystalline ping per shard on impact — the only "cute" sound in the encounter, placed 2 s after the note so it lands in the quiet) |
| **Mute scope** | During the encounter, **all meter warnings are muted** (a hull-critical tick does not fire) — the encounter is outside the game's feedback systems. You can hear the world, the 18 Hz, and yourself. Nothing else. UI clicks: −12 dB (usable, never loud). **No music, ever** (§5.1 rule) |
| **Scan response** | The only scan that "answers": silence → one 1.76 kHz tone, 800 ms (the §4.5 class table) |

**Why 18 Hz:** it sits at the very floor of human hearing (18–20 Hz is the literature's "infrasonic dread" band) — modern headphones render it as pressure, laptop speakers mostly can't, which is why the 36 Hz harmonic carries the speaker-mode experience (§6, AU-012). The design intent is that a playtest on reference hardware reports the Phantasma as *felt* — "my chest did something" — not "I heard a low note."

---

## 5. Music & Dialogue (Opus Streaming)

### 5.1 The Five Tracks (affirms GDD §10; adaptive per spec)

| Track | Key location(s) | Character | Stems |
|---|---|---|---|
| **Drift** | Open space, warp corridors, unexplored sectors | ambient drone, one slow pulse every 8 s | drone + pulse + lead |
| **Landing** | All planet surfaces (biome = filter + 1 ornament stem) | pulse, textural, ground-level | drone + pulse + lead |
| **Harvest** | Harvester zones, Act 2 regions | tension, motoric — a rhythm that is *working* | drone + pulse + lead |
| **Loom** | Siphon fields, Loom Gates, Act 3 (and the Compact Sky Event) | awe/terror — the act's theme | drone + pulse + lead |
| **Meridian** | Meridian Station, Tau Ceti system | warmth — the game's warmest 90 seconds | drone + pulse + lead |

- **Format:** Opus **mono 32 kbps** (music — ambient drone is low-entropy, 32 kbps is transparent here; the budget buys *stems*, not bitrate: 15 layers of feel for the storage of ~5 single tracks), loop point pre-marked per track (Appendix D), 3 stems per track → **5 tracks, 15 layers of feel** (TDD §2.9).
- **Adaptive transitions (per spec):** location change = **4 s crossfade** (location bus automation); intensity change = **stem automation** — entering Harvester space: +pulse stem at +6 dB; Siphon proximity: Loom lead +1 layer *and the location bus ducks to the gravity drone* (§4.4); Act 2 Compact Sky Event: Meridian → Drift, all stems −6 except drone, 20 s — the home track *leaving* is the moment.
- **Rules:** music never interrupts a story plate (plates duck music −12 dB, GDD §7.5); within a Siphon field, the gravity drone *is* the music (5.1's Loom track is suspended, not layered); the Phantasma encounter has **no music at all** (§4.6).

### 5.2 Dialogue

- **MERIDIAN VO:** < 200 lines, Opus **mono 32 kbps**, warm close-mono (the voice is *in the station*, not in the world — MERIDIAN lines play 2D-mixed with a faint station-room tail; she is the only always-2D human voice). Sparse by design (GDD §10: the lines are earned).
- **The 12 Signal Fragments:** ~40 s each, Opus mono 32 kbps, **granular-processed at load time** (the Architects' voice — the granular pass is CPU, deterministic, seeded by fragment index — same voice, never the same *texture* twice: the machine speaks slowly, Art Bible §7.4). Played through the radio bus (§4.4).
- **No other dialogue at launch.** Every other "voice" is a synth (the drill, the valve, the lattice) — the two real voices (MERIDIAN, the Architects) are the only fixed, *stored* human sounds in the game, and that rarity is the point (§1.1).

---

## 6. Mix, Loudness & Platform

- **Targets:** integrated **−14 LUFS**, true peak **−1 dBTP** (measured on the reference mix: 1 h scripted flight, BALANCED). Laptop-speakerrable by construction — the mix is mastered for the *reference hardware's* speakers first, headphones second (the iGPU audience's default output is the laptop speaker; headphones are the upgrade).
- **The 18 Hz problem (speaker mode):** most laptop speakers cannot reproduce 18 Hz. The Phantasma bed therefore runs **18 Hz at full + 36 Hz harmonic at −30 dB**; in **speaker mode** the 36 Hz harmonic is re-voiced to −20 dB (the "felt-not-heard" cue becomes a faint, wrong, *low* thrum — still correct: the cue is *pressure, not pitch*). Headphone mode is the intended experience (Art Bible/TDD HRTF headphone-first).
- **Bus structure:** `MUSIC / AMBIENT / SFX / UI / PLAYER` (5 buses) → ducking matrix (story plates, encounter, docking) → compressor (2:1, slow) → limiter. Music never compresses SFX; SFX never duck music (the world's sounds are *in front of* the music, always).
- **Master chain is fixed** (no user EQ) — the mix *is* the design; the user gets volume + headphone/speaker mode + spatialization toggle only (TDD §3.2 settings surface).

---

## 7. Resource Management & Budgets

| Budget | Value | Enforced by |
|---|---|---|
| **Total audio RAM (incl. synthesizer memory)** | **< 50 MB** (design ≈ 1.9 MB) | per-frame watermark assert (TDD §3.5), AU-007 |
| Opus active streams (worst) | 7 streams ≈ 0.5 MB rings | ring accounting |
| Shipped Opus storage | **≈ 16 MB total** (music 15 stems × 3 min × 32 kbps ≈ 11 MB · MERIDIAN < 200 lines ≈ 3.2 MB · 12 fragments ≈ 1.9 MB) | CI asset audit (TDD §5.1 step 3) |
| **Shipped SFX storage** | **0 bytes** | the principle (§1.1) — CI asserts no non-Opus audio in `assets/` |
| DSP CPU | ≤ 1.5 ms/frame (§2.2) | profiler (TDD §3.5), per-block |
| Callback overruns | **0** (PortAudio stream status) | nightly 10 h loop test (TDD §7) |
| Voice slots | 64, P0 never stolen | steal-policy assert in debug |
| Latency (UI click → out) | < 20 ms | startup self-test (TDD §7) |

---

## 8. SFX Category Index (complete)

Voice priorities per Appendix A. "Recipe" = synth graph (all components from §2's DSP set).

### 8.1 Ship Actions

| Sound | Trigger | Recipe | Pri |
|---|---|---|---|
| Engine (per class, §4.1) | throttle | class-table noise + bandpass + rumble | P1 (player + others, per Appendix A) |
| RCS burst | strafe/turn | 400–900 Hz noise puff, 60 ms, jet-panned | P1 |
| Brake | Space | bandpass hiss-down, 200 ms | P1 |
| Overdrive whistle | Shift | +1.5 kHz partial, +3 dB | P0 |
| **Warp tunnel** | jump (45 s) | Shepard rise + noise sweep + fold + seeded arrival chime | P0 |
| Docking beeps | approach gap | 1→4 Hz warm beeps (gap instrument) | P1 |
| Clamp servo / pressurize / undock | dock event | impact+hiss / formant rush / exhale | P1 |
| **Deflect set (AU-003):** PD ping · brownout flicker · hull glancing | defense events | 2.2 kHz ping / 2×40 ms flicker / slide+tick | P1 |
| Fuel scoop | L5 scoop | low-pass suction + granular clatter + fill-rise | P1 |
| Scanner hum | scan in progress | per-class timbre drone (§4.2) | P1 |

### 8.2 Combat

| Sound | Trigger | Recipe | Pri |
|---|---|---|---|
| Arc Lance (seeded) | fire | dual-path noise, pitch ±3%, 60+150 ms | P0 |
| Ion Lattice (L35) | fire | −4 st, 30 ms thwip + body | P0 |
| PD drone idle / ping | under fire / intercept | 0.5 Hz hum / 2.2 kHz ping | P2 / P1 |
| Explosion small/med/large (vacuum) | impact | metal crunch + debris granular + hull-wave 0.3 s | P1 |
| Explosion surface | impact | sub-bass thud + rumble + crackle | P1 |
| Harvester drill whine | strike ramp (500 ms) | 900→1,400 Hz sine + 40 Hz rattle | P1 |
| Flak / rail spike | enemy fire | crack+shrapnel / 200 ms whoosh → impact | P1 |
| Nest chitin creak | proximity | 60–200 Hz groans, seeded | P2 |
| Hull impact (player) | hit | directional thud + 1 tick | P0 |
| O2 breach / suit breach | meter event | high-pass hiss (continuous) / hiss + valve accel | P0 |

### 8.3 Environmental

| Sound | Trigger | Recipe | Pri |
|---|---|---|---|
| Wind (biome-filtered, §App B) | surface, continuous | biome noise bed + direction HRTF | P2 |
| **Ice crackle** (Glacial) | seeded events 10–40 s | 500 Hz–2 kHz click + 80 Hz thud; calving variant (3 s sub-bass) | P2 / P1 (calving) |
| **Lava bubbling** (Volcanic) | proximity to river | 20–200 Hz granular pops + 30 Hz rumble bed | P2 |
| **Gravity distortion** (Siphon, §4.4) | proximity 2 km | 28+56 Hz drone + source pitch-shear ±15% + fold tick | P1 |
| **Pulsar** (celestial, §4.4) | proximity 20 km | AM sine at 1/period (33 Hz @ 30 ms), real periods on anchors | P2 → P1 |
| Radiation storm | hazard | 6–9 kHz shimmer, AM at front speed | P2 |
| Sandstorm / acid rain / squall / micrometeor / aurora hum / vent | hazard/seeded | Appendix B recipes | P2 |
| **Radio static + flags** (§4.4) | story/scan flags | 400 Hz–3.2 kHz bus, white-noise burst 0.5–2 s, seeded shape | P1 |
| Watcher lattice chime | Watcher proximity | 0.1 Hz 3-note shimmer | P2 |
| Salvage beacon ping | beacon active | deterministic 2-note, 1 Hz | P1 |

### 8.4 UI & HUD

| Sound | Trigger | Recipe | Pri |
|---|---|---|---|
| Click / hover / nav / confirm / cancel | UI | §4.5 fixed recipes | P1 |
| **Scan complete (per-class voice)** | scan done | §4.5 class table (Phantasma = silence → 1.76 kHz) | P1 |
| NEW codex chime | first discovery | 784 → 1,175 Hz | P1 |
| Level up (per-level motif) | level event | seeded-per-level pentatonic rise | P1 |
| Mission complete / unlock | mission | 2-note+stamp / chord | P1 |
| Meter warnings (hull tick / valve / groan) | meter thresholds | §4.5 | P0 (O2) / P1 |
| Death tone / respawn chord | death / respawn | 98 Hz 1.2 s / 3-note warm chord | P0 |

### 8.5 The Encounter (Phantasma, §4.6)

| Sound | Trigger | Recipe | Pri |
|---|---|---|---|
| **Ambient→silence fade** | encounter lock | 2.0 s linear fade of all ambient + music buses | — (bus automation) |
| **18 Hz infrasonic bed** | t=2 s → t≈58 s | 18.0 Hz sine (real-time, audio thread) + 36 Hz harmonic (−30 dB / −20 dB speaker) | **P0, never stolen** |
| Player breathing (encounter pace) | t=3 s | seeded noise + bandpass, 4/s, player-locked 2D | P0 |
| **The note** | t=45–55 s | 55 Hz + 110 Hz, 1.5 s, HRTF at creature's face | P0 |
| Shard impact pings | t≈57 s | 1 high crystalline ping per shard, −18 dB | P1 |
| **Gaze dip (bed pitch −10%)** | player looks away from the face | 18.0 → 16.2 Hz, 200 ms glide, on the P0 bed voice | P0 (bed) |
| **Phantasma scan response** | scan complete | silence → 1.76 kHz, 800 ms | P1 |

---

## 9. Audio Production Plan (by Phase — maps to GDD §13)

| Phase | Audio deliverables | Gate |
|---|---|---|
| **P1 Pre-Production** | Philosophy sign-off (this doc); offline synth-preview tool v0 (audition recipes headless); HRTF profile v0 tuned on the reference box's headphones; **5 music tracks composed + stems cut (15 layers)**; MERIDIAN recording (< 200 lines) + direction notes; 12 Signal Fragments produced + granular recipe; Opus manifest schema (Appendix D); loudness target (−14 LUFS / −1 dBTP) | All 5 tracks auditioned by the team; HRTF blind-position pass on 3 subjects (±45°) |
| **P2 Engine** | PortAudio callback on core 7 (48 kHz/256); 64-voice manager + steal policy (Appendix A); DSP graph + HRTF bus; 1 Opus stream E2E (100 ms rings); engine synth v1 (Astra class); UI click; **18 Hz bed + breathing voices (P0 slots)**; CPU measurement harness; 50 MB watermark assert; latency self-test | On the reference iGPU: **0 callback overruns, ≤ 1.5 ms DSP, watermark green, < 20 ms click latency** |
| **P3 Exploration** | 7-class engine variants (§4.1); warp journey + seeded arrival chimes (§4.2); docking set; scan voices (10 classes, §4.5); wind v1 (Terrestrial, Appendix B); Drift + Landing tracks live + location crossfades; HUD sound set; Doppler on (pulsar fly-through §3.2); pulsar synth (AU-004) | Playtest: "the first 3 hours are *audibly* distinct — station / space / surface" |
| **P4 Survival** | Combat set (weapons, vacuum/surface explosions, drill/flak/rail, nest creak); environmental set (ice crackle, lava bubbling, storms, acid sizzle, micrometeor); **radio static + 5 event flags** (AU-006); Harvest track live; MERIDIAN VO in-context; ducking matrix; meter warnings (valve tick) | Blind test: all 5 survival meters are *identifiable by sound alone* |
| **P5 Story/Aliens/Phantom** | **Phantasma encounter final (§4.6, AU-002 timeline)**; Siphon gravity distortion (drone + shear + fold tick, §4.4); Watcher lattice chime; Loom track + Compact Sky Event transition (§5.1); 12 Signal Fragments in the radio bus; HRTF validation (Appendix C) | The Phantasma moment = "most-remembered audio" in a blind playtest; **0 voice-starvation asserts** over a 2 h scripted session |
| **P6 Polish** | Full mix + master chain; speaker mode (36 Hz re-voice, §6); cutscene audio (typewriter tick + music bed); final loudness certification; **10 h music-loop test (0 gaps)**; per-biome ambient tuning (Appendix B pass) | −14 LUFS / −1 dBTP certified; 0 overruns in 10 h |
| **P7 Multiplayer** | Presence audio (other explorers: their engines, their warps, their scans are audible in range; their *ghost* silhouettes are silent); session one-shots synced via opset (deterministic sound = same action log, §2.1) | 2 players on different networks: both hear the other's ship events, < 150 ms, no divergence in the 1 h session test |
| **P8 QA/Launch** | Audio regression suite (40 canonical events spot-checked per build); steal-violation audit (P0 never stolen); final RAM/CPU certification on the reference box; Opus manifest audit (≤ 20 MB) | All gates green; zip audio total ≤ 20 MB |

---

## 10. Audio Decision Log

**Format:** `AU-###` · date · decision · rationale · status. Append-only; design-affecting entries get a GDD §17 pointer.

| ID | Date | Decision | Rationale | Status |
|---|---|---|---|---|
| AU-001 | 2026-09-27 | **Synthesis over storage is absolute: 100% of SFX/ambience is real-time synthesized; Opus streaming is reserved for music + dialogue (MERIDIAN + 12 fragments). Zero SFX storage — CI-enforced.** | The brief's core principle; makes audio deterministic (a function of the action log), keeps the zip/RAM honest, and gives infinite variation from recipe tables. TDD §2.9's DSP graph is the implementation. | ACCEPTED |
| AU-002 | 2026-09-27 | **Phantasma encounter audio (canonical, §4.6): 0–2 s ambient+music fade to digital silence → 18 Hz infrasonic sine bed (felt not heard; 36 Hz harmonic for speaker mode) from t=2 s → breathing returns t=3 s → silent body-replay → one sub-bass note at 45–55 s, HRTF-positioned at the creature's face → bed+breathing out 2 s, ambient back 4 s (asymmetric). Real-time synthesis on the audio thread; dedicated P0 voice slot, never stolen; all meter warnings muted during the encounter.** | The brief's parameters (2 s fade, 18 Hz, real-time) are authoritative and **refine GDD §8.4's "4 s cut" and the Art Bible's "0–10 s silence" sequence** (both updated in place, pointer D-019 / A-013). The 18 Hz bed makes the encounter a *physiological* event — the player's body knows before their ears do; the asymmetric fades (2 s in, 4 s out) encode "silence is entered fast, left slowly"; muting the warnings places the encounter outside the game's feedback systems. | ACCEPTED |
| AU-003 | 2026-09-27 | **The brief's "shields" SFX category is realized as the deflect set: PD intercept ping, energy-brownout flicker, hull glancing deflection.** | GDD §5.11: "No shields. Hull is the shield." A shield-wall hum would imply a system the game doesn't have; the game's actual energy-defense events (PD drones L25, brownout GDD §5.5) carry the brief's intent. | ACCEPTED |
| AU-004 | 2026-09-27 | **Pulsars: carrier sine amplitude-modulated at 1/period (e.g. 30 ms period = 33 Hz modulation — the brief's example). Perceptual banding: period > 80 ms = discrete thuds; 30–80 ms = fused buzz/tremolo (the 33 Hz case — a buzzing star); < 30 ms = solid tone + shimmer. The 430-anchor real pulsars use their REAL periods (PSR B1919+21: 1.337 s → 0.75 Hz heartbeat; Vela: 89 ms → 11 Hz).** | The brief's formula, made buildable; the perceptual banding turns the science into three distinct *emotional* registers (heartbeat / buzz / hum) instead of one sound at variable speed. Real periods on anchor pulsars extend the 430-anchor promise to the ears (you can *hear* the real pulsar). | ACCEPTED |
| AU-005 | 2026-09-27 | **Engines: throttle-modulated noise generator + per-class recipe table (§4.1); all parameters smoothed 50 ms attack / 200 ms release; 7 class variants incl. near-silent Watcher and dead derelicts.** | The brief's three requirements (noise-modulated, smooth, class-varied) as a parameter table; the smoothing makes the engine feel *mechanical* rather than digital; class variety makes ship class legible by ear before the player looks. | ACCEPTED |
| AU-006 | 2026-09-27 | **Radio static: white-noise bursts (0.5–2 s, seeded shape) on the 400 Hz–3.2 kHz radio bus, triggered by story/scan event flags (SIGNAL_HEARD, BEACON_DRIFT, WATCHER_REPLY, LOOM_GATE_OPEN, COMPACT_EVENT); Signal Fragments play on the same bus, granular-shaped at load.** | The brief's mechanism (event flags → bursts) as the diegetic channel for the mystery — all mystery audio arrives *through the radio*, which makes the player an eavesdropper on the galaxy (GDD §10: "hears the mystery before they can read it"). | ACCEPTED |
| AU-007 | 2026-09-27 | **Total audio RAM < 50 MB including synthesizer memory (design ≈ 1.9 MB, §2.4 table); Opus in 100 ms chunks into 2-frame ring buffers; all active streams share the cap. Refines GDD §12.2's 100 MB audio line; sits inside TDD's 0.5 GB Audio/IO pool (T-013).** | The brief's resource caps, measured rather than assumed; the 50 MB is a guardrail with ~48 MB of headroom reserved (per-planet ambient tables, P7 presence, cutscene one-shots). | ACCEPTED |
| AU-008 | 2026-09-27 | **Spatialization: 8-directional CPU HRTF on the audio thread (headphone-first, speaker fallback), elevation 3-level blend; Doppler from ship-relative radial velocity, ratio capped [0.5, 1.5], applied to world/weapon/storm/pulsar/Siphon sources only; distance log curves, 3-zone culling per preset.** | The brief's two spatial requirements (CPU HRTF, Doppler from relative velocity) as a spec; the ship-vantage principle (§1.2) is what makes ship-relative Doppler *correct* in vacuum; the cap keeps Doppler musical; UI/music/breathing are excluded by design. | ACCEPTED |
| AU-009 | 2026-09-27 | **Music: 5 Opus tracks (Drift/Landing/Harvest/Loom/Meridian) with 3 stems each = 15 layers; location = 4 s crossfade, intensity = stem automation; Siphon fields replace music with the gravity drone; Phantasma = no music.** | The brief (atmospheric Opus tracks for key locations, adaptive transitions) mapped onto the GDD's 5-track plan (GDD §10); "music is the only fixed thing" (§1.3.1) makes its automation the game's emotional steering wheel — including *leaving* (the Compact Sky Event). | ACCEPTED |
| AU-010 | 2026-09-27 | **UI sounds are constant (fixed recipes, zero seed variation); world sounds are variable (seeded).** | A UI that sounds different twice is a UI that feels broken; a world that sounds the same twice is a world that feels recorded. The brief's synthesis-over-storage principle, applied with an exception where consistency beats variety. | ACCEPTED |
| AU-011 | 2026-09-27 | **Warp = 45 s Shepard-tone journey + fold + seeded arrival chime (3–4 notes, per destination region).** | The jump is the game's travel verb — it must be a *journey* you can hum, and the per-region arrival chime makes systems recognizable by ear (the audio half of "every location offers something unique", GDD Pillar 1). | ACCEPTED |
| AU-012 | 2026-09-27 | **Master: −14 LUFS integrated, −1 dBTP; speaker-mode re-voices the 36 Hz harmonic to −20 dB; fixed master chain (no user EQ); laptop-speaker-first mastering.** | The reference audience's default output is the laptop speaker; a mix that only works on headphones fails the hardware target (GDD §12.1). Loudness targets keep 1 h of space flight fatigue-free and the 18 Hz bed safe on consumer speakers. | ACCEPTED |

---

## Appendix A — Voice Slot Priorities

64 slots (TDD §2.9). Steal order: lowest priority first; P0 is **never stolen** (a P0 request with no slot = hard assertion in debug, "audio voice starvation" logged as a P1 defect).

| Pri | Voices | Stealable? |
|---|---|---|
| **P0** | Phantasma 18 Hz bed (+harmonic) · player breathing · player weapon · warp tunnel · hul impacts · O2/suit breach | **never** |
| **P1** | engines (player + nearest 4) · radio/static · scan hum + scan-complete · docking · deflect set · combat (enemy) · Siphon gravity drone · mission/UI event sounds · salvage beacon | yes, P1-first |
| **P2** | ambient beds (biome wind/storm/etc.) · Watcher chime · PD idle · distant ships · pulsar bed (> 2 km) · decor one-shots | yes |
| **P3** | decorative micro-sfx (particle sizzles, geode ticks) | first to go |

**Player-locked (never spatialized, never distance-rolled):** breathing, suit creak, UI, MERIDIAN, music — the 5-bus PLAYER/UI/MUSIC split in §6.

## Appendix B — Per-Biome Ambient Recipes

Base bed + event table (all seeded per planet — same planet, same sound, always). Wind filter = biome's bandpass; "grain" = granular layer density.

| Biome | Bed | Filter | Grain | Signature events |
|---|---|---|---|---|
| Terrestrial | wind + insect bed (seeded 2–4 kHz) | 400 Hz–4 kHz | low | rain (lines → plops), storm (distant 40 Hz crack, 120 ms delay), river proximity (looping 600 Hz water) |
| Glacial | wind (cold, high-passed) + aurora hum (night, 90 Hz −28 dB) | 800 Hz–6 kHz | none | **ice crackle** (10–40 s, click+thud), calving (rare, 3 s sub-bass), whiteout (grain 3×, bed −6 dB) |
| Volcanic | 30 Hz rumble bed + ash hiss | 40 Hz–1.5 kHz | high | **lava bubbling** (20–200 Hz pops, distance-driven), radiation storm (6–9 kHz shimmer), vent hiss |
| Desert | wind (low, dry) + grain | 200 Hz–3 kHz | very high | **sandstorm** (grain 3×, rumble), night: silence + 1 cricket (the desert's one joke of life) |
| Ocean | wave bed (seeded 0.1–0.3 Hz swell LFO) + spray | 100 Hz–5 kHz | med | **squall** (dark rain + far lightning), reef proximity (biolume-chime-adjacent glints, −24 dB) |
| Toxic | fog bed (low, wet) + **sizzle** (drop events, high-pass) | 150 Hz–8 kHz | high | **acid rain** (sizzle density by rate), supercell (40 Hz crack, 200 ms delay — closer than other storms; Toxic doesn't give distance), spore drift (slow 200 Hz shimmer) |
| Barren | near-silence (the most silent biome — the void's floor) | 200 Hz–2 kHz | none | **micrometeor shower** (descending 2–4 kHz whistles, seeded count, HRTF positions, impacts), regolith shift (footstep-grain on EVA), Loomsteel glint (a single 1.2 kHz ping, 1 in 50 landings — the rare "something is here" in the honest biome) |

Space (helm vantage, all biomes N/A): ship hum (engine §4.1) + starfield drone (Drift track bed) + proximity layers (pulsar §4.4 / Siphon §4.4 / Watcher chime / Harvester drill) — the "empty" of space is the ship, never the void.

## Appendix C — HRTF Specification & Validation

- **Profile:** `HRTFProfile` (TDD §3.2, rides `astra.cfg`): 8 azimuths × 2 ears × 2nd-order IIR (6 coeffs, float32) + 3-level elevation blend (low/eye/high) + delay line (256 smp, for TOA at 50 m reference). Tables = 2 KB (§2.4). Source profiles tuned for the *reference hardware's headphones* (one of the reference-box's assets) — per-user tuning is a post-launch candidate (a `HRTFProfile` blob in `astra.cfg`, TDD §3.2).
- **Application (audio thread, per 3D source per frame):** `azimuth → nearest profile (45° steps) → delay = d/c → IIR pair (L/R) → level (elevation blend)`. No per-sample HRTF math — the per-voice cost is 2 IIRs + 1 delay read (the 0.3 ms budget in §2.2).
- **Speaker mode:** azimuth → panner (−1…+1, cos-curve), elevation → level (−3 dB per 30° up); the HRTF bus is bypassed (a 0.1 ms save, and speaker HRTF is theater).
- **Validation gates (P5, TDD §7):**
  1. **Blind position test:** 10 subjects, 12 positions (az × el), single ping — ≥ 75% correct azimuth within ±1 profile step (±45°).
  2. **Doppler sanity:** fly-through of a pulsar at 1,000 m/s → the pulse-rate sweep is *monotonic and continuous* (no discontinuity at the 0.5/1.5 caps — the cap engages with a 50 ms glide).
  3. **Speaker-mode regression:** same 12 positions, panner — ≥ 70% left/right (azimuth half-plane) correct.
  4. **Callback overrun = 0** over the 10 h loop (TDD §7) — the HRTF path must never brown out the callback.

## Appendix D — Opus Stream Manifest (example)

`assets/audio.astropack` → `manifest.json` (schema v1):

```json
{
  "schema": 1,
  "sample_rate": 48000,
  "streams": [
    {
      "id": "music_drift",
      "role": "MUSIC", "location": "space",
      "stems": [
        { "file": "drift_drift.opus",  "kbps": 32, "mono": true, "loop": [37250, 96050], "duck_curve": "linear4s" },
        { "file": "drift_pulse.opus",  "kbps": 32, "mono": true, "loop": [37250, 96050] },
        { "file": "drift_lead.opus",   "kbps": 32, "mono": true, "loop": [37250, 96050] }
      ]
    },
    { "id": "vo_meridian", "role": "DIALOGUE", "kbps": 32, "mono": true,
      "lines": [ { "id": "MIR_001", "file": "vo/mir_001.opus", "prime_ahead": 3 } ] },
    { "id": "signal_fragment", "role": "SIGNAL", "kbps": 32, "mono": true,
      "fragments": [ { "id": "SIG_01", "file": "sig/sig_01.opus", "granular_seed": 1 } ] }
  ]
}
```

Rules enforced at build (TDD §5.1): every `file` exists · total Opus bytes < 20 MB · all music stems of a track share one loop pair · VO ids match the story table (GDD §7.5) · fragment count = 12.

---

*End of Audio Design Document v1.3 (family-aligned, 3rd audit pass). Next review: Phase 2 exit (DSP graph on core 7, ≤ 1.5 ms, 0 overruns, 50 MB watermark green). Audio authority for all mixes, sound design, and audio budgets from this date.*

