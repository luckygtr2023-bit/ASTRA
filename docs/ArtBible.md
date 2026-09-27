# Project Astra Cosmos — Art Bible

| | |
|---|---|
| **Document** | Art Bible — visual style, aesthetic principles, asset & reference authority |
| **Version** | 1.1 — Re-issued & verified against the Art Director brief |
| **Date** | 2026-09-27 |
| **Status** | **Authoritative for all visual decisions.** |
| **Companion** | [GDD](GDD.md) v1.0 (design intent) · [TDD](TDD.md) v1.0 (implementation). Conflict rule: GDD wins on *what exists in the game*, this document wins on *how it looks*, TDD wins on *how it is computed*. Art decisions are logged as `A-###` in §11; design-affecting ones get a pointer entry in the GDD decision log. |

---

## Table of Contents

0. [Document Control](#0-document-control)
1. [Artistic Philosophy](#1-artistic-philosophy)
2. [Color System](#2-color-system)
3. [Lighting, Skies & Atmosphere](#3-lighting-skies--atmosphere)
4. [Key Visuals — the AAVs](#4-key-visuals--the-aavs)
5. [Hero Assets](#5-hero-assets)
6. [Procedural Visualization](#6-procedural-visualization)
7. [UI / UX Art](#7-ui--ux-art)
8. [Visual References (Placeholder Library)](#8-visual-references-placeholder-library)
9. [Asset Pipeline, Naming & QA](#9-asset-pipeline-naming--qa)
10. [Art Production Plan (by Phase)](#10-art-production-plan-by-phase)
11. [Art Decision Log](#11-art-decision-log)
- [Appendix A — Full Palette (hex)](#appendix-a--full-palette-hex)
- [Appendix B — Star Class → Color](#appendix-b--star-class--color)
- [Appendix C — Icon Glyph Table](#appendix-c--icon-glyph-table)
- [Appendix D — `[VISUAL REF]` Slot Template](#appendix-d--visual-ref-slot-template)

---

## 0. Document Control

### 0.1 Change Process

1. Propose `A-###` (date, decision, rationale, status `PROPOSED`).
2. Decisions that change *gameplay-relevant* visuals (a meter, an icon's meaning, a spawn look) also get a pointer entry in GDD §17.
3. On acceptance, affected prose is updated in the same commit and the version bumped.

### 0.2 Version History

| Version | Date | Summary |
|---|---|---|
| 1.0 | 2026-09-27 | Baseline Art Bible. Philosophy, full color system (master + 8 biome/phenomenon palettes), lighting/sky LUT architecture, AAV specs (Siphon, Phantasma), hero asset program (anchor planets, stations, ships — four-wing ship), procedural visualization (vertex-color surfaces, weather particles, nebula slices), UI/UX art (HUD, Star Atlas, menus/cutscenes), placeholder reference library, pipeline + QA, phase plan, decisions A-001…A-012. |
| 1.1 | 2026-09-27 | Full re-issue at the owner's request: document regenerated and re-verified line-by-line against the Art Director brief (all spec points confirmed present). One fix: §10 P1 gate corrected to match §8's actual slot inventory (39 slots: 19 named, 20 to-place). No decisions changed; A-001…A-012 and all GDD/TDD cross-references (D-018, T-007/T-009) intact. |

---

## 1. Artistic Philosophy

> **Stylized Proceduralism. Wonder through color, lighting, and computational effects — not photorealism.**

The galaxy is *computed and painted*, not photographed. Every surface the player can stand on is generated on the CPU and colored by vertex; every effect that makes the player stop breathing is computed, not captured. Photorealism is the wrong target on two grounds: the Vega 8 cannot afford it, and it is the wrong *emotional register*. We want the sense of a **hand-colored star chart by a very strange cartographer** — bold, legible, a little uncanny.

### 1.1 The Five Art Principles

1. **Wonder is color and light, not detail.** A 600-tri planet can stun if its limb glow, its day/night terminator, and its palette are right. Detail is the last 5%, not the foundation. Budgets protect light and color first; polygons are spent where a silhouette lives (GDD §9.2).
2. **Readable at 500 m.** Any object at 500 m must be identifiable by **silhouette + 2 colors** in under 2 seconds (GDD §8.5 rule 1). The silhouette test is the first gate of asset QA (§9.3).
3. **Black is reserved.** Nothing in the game is pure black except **the Phantasma and true void** (GDD §9.3). "Dark" surfaces are charcoal, so when real black appears, the player's nervous system knows before their eyes do.
4. **Cheap on memory, expensive on light.** Vertex colors over textures; ≤ 4 MB of hero texture memory in the entire game (GDD §9.2). The "painting" happens in shaders, LUTs, and vertex data (TDD §2.1) — which means the art *is* the algorithm. A biome is a function; the artist's job is to tune the function beautifully.
5. **The unsettling is the rarest material.** Dread visuals (Phantasma, Siphons, Loom fields) are **rare, reserved, and never comedic** (GDD §2.1 tone). Saturated wonder is the default register; the cold undercurrent appears in ≤ 5% of any given session.

### 1.2 Tone in One Image

*Interstellar*'s Gargantua hanging over a sunlit field; *No Man's Sky*'s first-planet color hit; *Subnautica*'s biolume leviathan in a dark sea; *Slay the Spire*'s icon clarity; *Monument Valley*'s palette restraint applied to a 100,000-light-year canvas. That mixture — **spectacle you can read** — is the game.

---

## 2. Color System

### 2.1 Master Signal Palette (affirms GDD §9.3 — unchanged, v1 locked)

| Role | Hex | Used for |
|---|---|---|
| Void Indigo | `#0B0E2A` | Space background, UI base |
| Star Amber | `#FFB347` | Warm stars, UI primary, hope, station lights |
| Biolume Cyan | `#4DF0E0` | Watchers, scanning, "NEW", data |
| Rust Red | `#C0392B` | Harvesters, warnings, hull-critical |
| Architect Gold | `#E8C547` | Loom tech, story UI, **anchor** map icons |
| Phantom Void | `#000000` | **Reserved**: Phantasma body + true void only |

### 2.2 Biome & Phenomenon Palettes (Art Bible extension — A-002/A-003)

Each palette: **base** (60% of surface), **secondary** (30%), **accent** (10% — the "jewel"), **hazard** color, **emissive** (light sources). Quantized to 64 colors per mesh at export (GDD §9.2 rule 2).

| Biome / Phenomenon | Base | Secondary | Accent | Hazard | Emissive |
|---|---|---|---|---|---|
| **Terrestrial** (green/blue) | canopy `#3FA86A` | soil `#7A5C3E` | water `#2E9BD6` | storm grey `#8A94A8` | window-warm `#FFB347` |
| **Glacial / Ice** (white/cyan) | ice `#DFF3F7` | deep ice `#7FD4E8` | open water `#1E5E7A` | whiteout `#C8D8E0` | aurora `#4DF0E0` |
| **Volcanic** (red/charcoal) | basalt `#2A2226` | rock `#3A2E33` | **lava `#FF5A2A`→`#FFB347`** (gradient) | radiation `#FF7A3D` | magma veins `#FF7A3D` |
| **Desert** | dune `#E0A458` | rock `#B0684A` | oasis `#4DA8A0` | storm ochre `#C88A4A` | — |
| **Ocean** | surface `#2E8FBF` | deep `#123E5E` | reef `#5AD9A0` | squall `#2A3A4A` | biolume `#4DF0E0` |
| **Toxic** | fog `#8FB83A` | ground `#4A5E2A` | spore glow `#C8F04C` | **acid rain `#A8E83A`** | spores `#C8F04C` |
| **Barren** | regolith `#6A6A72` | crater `#52525A` | Loomsteel glint `#E8C547` | meteor streak `#E8E8F0` | — |
| **Exotic / Loom** (purple/warped — A-003) | field `#4A2A6A` | warp `#7A3ABF` | streak `#B06AE8` | core white `#FFFFFF` | **Loom gold `#E8C547`** |

**Palette rules:**
1. **Exotic purple is a phenomenon, not a place.** Purple/warp belongs *only* to Loom fields, Siphon strain space, Loom Gates, and warp-jump space. No planet biome uses it — the moment the sky turns purple, the story has reached that place.
2. **Volcanic "black" is charcoal `#2A2226`** — the black-reservation rule (Principle 3) is never waived for a biome.
3. **Hazard = warm, danger = hot.** Every hazard color sits in the amber→red→acid-green family; Biolume Cyan is *never* a hazard color — cyan always means "information/safe/you".
4. **Two-color silhouette test:** any two objects that can coexist in one view must be distinguishable by base color hue *alone* (color-blind-safe pairs are checked at palette QA, §9.3).

### 2.3 Per-Biome Mood (one line each)

- **Terrestrial** — the most "Earth" biome; the player's comfort baseline, so everything else can be stranger.
- **Glacial** — silence made visible; light does the art (aurora, subsurface ice-glow).
- **Volcanic** — the palette *is* the hazard; light sources are the danger.
- **Desert** — scale and shade; the dunes do the depth, the ochre does the mood.
- **Ocean** — depth and biolume; the reef is the jewel.
- **Toxic** — the saturated-wonder register deliberately turned sick; green is not biolume here.
- **Barren** — the honest one; gray is allowed to be beautiful (crater rims at star-amber light).
- **Exotic/Loom** — unreality; color *moves* (warp streaks animate in vertex color).

---

## 3. Lighting, Skies & Atmosphere

### 3.1 Lighting Model (affirms GDD §9.2 / TDD §2.7)

- **One directional light** (the star — color from §Appendix B), **hemispheric sky fill** (from the sky LUT), **per-object emissive** (biolume, engines, lava, station windows). No dynamic shadows in space; one 1024² blob shadow for the player on surface (GDD §9.2).
- **Emissive is a first-class material channel.** The game's "night scenes" are emissive scenes: Watcher lattices, lava rivers, acid rain, station windows, Loom fields. Emissive budget: ≤ 3 emissive draws per frame beyond the mandatory ones.
- **Starlight is a mood instrument:** G8V (Tau Ceti, home) is warm amber-white; M-dwarf systems run deep red and *long-shadowed*; O/B systems are blue-white and harsh (no warm light exists there — the player feels it).

### 3.2 Skies: Pre-Computed Scattering LUT (A-006)

No runtime Rayleigh/Mie math. **Offline** (Python tool, TDD §5.4 lineage) we precompute, per **atmosphere class** (12 variants: N2-O2, CO2-heavy, CH4, acid-haze, thin, none, …):

- A **256×256 LUT** (elevation × azimuth → RGB) for the sky dome, for 8 sun-angle steps × the star's blackbody color (baked per star class — Appendix B),
- Horizon limb color + inner/outer atmosphere shell gradient stops,
- A 32-step "day↔night terminator" curve.

**Runtime cost: one LUT sample per sky pixel + shell shader constants.** LUT memory: 12 × 8 × 256 KB raw → 1.5 MB ZSTD-compressed (fits the asset pool with margin).

**Sky art rule:** a sky must be *readable as a promise* — its color tells the player the hazard before the scan does (Terrestrial: soft blue-amber; Toxic: sick green at the zenith; Glacial: pale cyan with aurora bands).

### 3.3 Nebulae: Billboard Slices (A-006)

- Each nebula = **6–12 crossed billboard slices** (512×512-vertex gradient planes, *no textures* — filament structure is **per-vertex noise** in vertex color, 64×64 grid per slice).
- Slices are camera-facing, additive-blended, parallax-offset (each slice 0.2–1.0× drift), with a per-slice hue from the master signal palette (nebulae lean Biolume Cyan / Exotic purple — purple *with* gold threads = Loom-adjacent, §2.2 rule 1).
- Live budget: ≤ 16 nebula slices total on screen (TDD §3.2 preset table). A Watcher-nest nebula (8% of nebulae, GDD §6.3) gets one extra cue: a faint **lattice glow** in its core.
- Reference for silhouette language: §8 (REF-NEB-*).

### 3.4 Starfield

- Three parallax layers (TDD §2.1): far static (the preset's 100K/500K/1M points), mid parallax, near warp-streak layer.
- Per-star color by spectral class (Appendix B); 1 in 200 stars gets a **4-pt diffraction spike** (a vertex cross, not a texture) — a reminder that "real stars are here" (anchor stars *always* have the spike + a gold tick at 4× size — the map's "you can find Sirius" promise, GDD §6.2).

---

## 4. Key Visuals — the AAVs

The two Above-And-Beyond Visuals (GDD D-010 / §9.4). Each has: a **full spec**, a **low-tier fallback**, and a **reference block** in §8.

### 4.1 AAV-1 — The Siphon (Interstellar-class black hole)

**The image:** a hole in the sky. The accretion disk bends over and under the event horizon (the classic "hat" silhouette), one side of the disk is **Doppler-brighter and blueshifted** (approaching side), a thin **photon ring** traces the horizon, and the *starfield behind it is bent* — visible arcs of stars where the disk is thin. It does not look like a picture of a hole. It looks like the sky **folded**.

| Element | Spec |
|---|---|
| Event horizon | 1k-tri sphere, **Phantom Void `#000000`** (the one other user of pure black) |
| Accretion disk | Procedural shader (no texture): particle-stream noise orbiting at Kepler-scaled speed, **Doppler beaming** (approaching side +40% brightness, hue shifted toward blue-white; receding side dimmer, deeper amber), inner edge temperature white → outer edge Star Amber → `#C0392B` rim |
| Photon ring | Thin bright annulus at 1.5× horizon radius, white-amber, pulses ±5% at 0.2 Hz (the "breathing" that makes it feel alive) |
| **Lensing** | Screen-space ray-bend (analytic Schwarzschild, TDD §2.7) inside a 25%-screen-radius mask: starfield + disk bend around the horizon; the disk's *far side appears above the horizon* — the "hat" |
| Strain field | In Siphon-adjacent space: a faint **Exotic purple** warp shimmer on the starfield (vertex-color perturbation) + the warp-fuel penalty made visible (GDD §5.2) |
| **Vega 8 render strategy (A-004)** | **The lensing pass renders at ½ resolution offscreen and upscales** (bilinear + 1-px sharpen) — the bend is low-frequency, so half-res loses nothing perceptible and cuts the post cost ~4×. Disk + horizon are full-res. |
| Preset tiers | **LOW:** glow billboard + disk shader, *no lensing* (the "hat" silhouette is faked by a second, bent disk strip — the fallback must read as the same object). **BALANCED:** full half-res lensing + disk + photon ring. **HIGH:** + 16-step volumetric light-bend raymarch inside the mask + accretion detail + time-dilated disk rotation |
| Fallback rule (GDD §3.5) | A player on LOW must describe the Siphon identically to a HIGH player in a post-viewing interview: "a hole in the sky that is looking back." Audio (sub-bass drone + sheared sources, AudioDesign §4.4) carries the scene when pixels don't. |

### 4.2 AAV-2 — Xenothrix Phantasma

**The image:** something that has the *proportions* of a standing person, at a scale that makes that the worst possible thing.

| Element | Spec |
|---|---|
| **Form (A-005)** | **Tall, thin humanoid silhouette** — 9:1 head-to-body, long limbs, narrow shoulders; surface encounter scale per GDD §8.4 (the entity looms over the landing zone; its head is a horizon). Encounter pose: **crouched, many-jointed** (GDD §8.4) — the stillness of a predator that has already decided |
| **Body** | **Transparent starfield body**: the body is a *cut-out of the scene* — the near world is absent, and the **far starfield shows through** (the game's own starfield layer, masked and inverted — TDD/GDD: no extra asset, the effect is *absence*). Rim: stars bend at the body edges (lensing rim reusing the AAV-1 math at ¼ intensity) |
| **Edge glow** | **Blue-white Cherenkov rim** — fresnel-based, `#DDEBFF` core → `#7FD4E8` falloff, pulse 0.1 Hz. Cherenkov = the color of *something moving through water faster than light* — the palette's word for "it should not be able to move like that" |
| **Phasing limbs** | Limbs phase **independently** on the GDD cycle (3 s visible / 7 s absent, 10 s cycle): left arm, then right, then the far leg — a rag-doll flicker of *almost* coherence. When the player looks directly at it: cycle locks **continuous for 60 s** (the encounter) |
| **The face** | **Void face with tracking eye-points**: no features, 2–7 points of cold white light in the dark of the face. They **track the player with a 400 ms delay** — you move, they follow, slightly late, like a camera operator. When the player looks *away*, the eye-points **drift toward where the player is not looking** (audio cue: the sub-bass shifts 10% lower — it is still watching; you only know because the sound does) |
| **Encounter sequence (60 s)** | Audio per AudioDesign AU-002 (D-019): 0–2 s ambient + music fade to digital silence; from 2 s a continuous **18 Hz infrasonic bed (felt not heard)**; 3 s: the player's breathing returns (synthesized, TDD §2.9). 12 s: it locks visible. 15–45 s: it **replays the player's first-ever scan** inside its body (the first object, re-rendered in miniature in the starfield-cut — the game watches the player watch it watch you; the replay is *silent* — only the bed and the breathing). 45–55 s: one sub-bass note, HRTF-positioned at **the creature's face** (where you've been looking, or where you were looking — either is worse). 55–60 s: it leaves; bed + breathing fade out over 2 s, the ambient returns over 4 s (silence is entered fast, left slowly); **Phantom Shards** (1–3, `#DDEBFF` prisms, 300 XP) fall where it stood |
| **Never** | Never casts a shadow (shadows bend *away* — GDD §8.4), never appears in two frames of the same place, never repeats a body configuration (procedural joint angles from the encounter seed), never reacts to weapons (firing at it does *nothing*, including the recoil — nothing) |
| **Preset tiers** | LOW: silhouette + Cherenkov edge only (no cut-out — a flat void-shape with the rim). BALANCED+: full cut-out + rim lens + eye-tracking. The *silhouette + audio* must carry the scene at every tier (GDD §3.5 fallback rule) |

### 4.3 B+ Set-Pieces (fixed budget, not AAVs)

1. **Warp jump** — 5 s: stars stretch to streaks (near layer), a ring of Biolume Cyan closes, 2 s of folded light, arrival. Reuses the starfield + one post pass.
2. **Landing** — descent haze, terrain resolving (the generation *is* the effect: the planet's face assembles as you fall — 2 s of visible vertex build at BALANCED+), touchdown dust ring.
3. **The Compact Sky Event** (Act 2, GDD §7.2) — at home: Tau Ceti dims 15% in 90 s of real time, *on the sky the player is standing under*, with MERIDIAN's single quiet line. No shader trick — the directional light and the LUT actually change. The game's quietest and loudest moment.

---

## 5. Hero Assets

### 5.1 Low-Poly Hero Asset Program & Budgets (affirms GDD §9.2; Vega 8-safe)

Low-poly deliverables, clean simple shapes, procedural geometry + vertex coloring (A-001/A-008). Every row below is the *shipped* poly count; the Blender source may be hi-poly — the iGPU never sees it.

| Asset | Source (Blender) | **Deliverable** | Tri budget | Texture |
|---|---|---|---|---|
| Player ship *Astra* | 300k hi-poly | **12k** | 12k | 1×1k (roughness) |
| Meridian Station | 1.2M hi-poly | modular, **80k** (LOD3→8k) | 80k | 1×1k |
| **Anchor planets (8)** | 4M hi-poly sculpt | **hero sphere 512×256 (≈131k)** + 8 landmark props (≤5k each); LODs to the 33k global sphere | 131k + 40k | **0** (vertex-color hand-paint) |
| Watcher | 250k hi-poly | **8k** | 8k | 1×1k |
| Harvester worker / escort / siege | 100k / 200k / 500k | **6k / 10k / 18k** | 34k | 0 (chitin = vertex-color bands) |
| Xenothrix Phantasma | procedural | **20k** (TDD §2.7) | 20k | 0 (shader-only) |
| Siphon | procedural | horizon 1k + full-screen pass | 1k | 0 |
| Derelict (3 classes) | 300k | **15k** | 45k | 0 |
| Asteroids (4 variants) | — | 800 ea, instanced | 3.2k | 0 |
| Surface props (geode, rock, monolith, ruin, beacon) | — | 200–2k each, instanced | — | 0 |
| **Total hero texture memory** | | | | **≤ 4 MB** (GDD §9.2, proud constraint) |

**Anchor planets (A-008) — "high-detail source, reduced deliverable":** the 8 anchor planets (Thessaly, the Proxima worlds, the Sirius system, Betelgeuse's cloud, the Cygnus X-1 field, Sgr A* approach, M45's bright ones, Sol — the old home) are **sculpted at high detail in Blender as the canonical reference**, then **reduced to the 512×256 hero sphere** with a hand vertex-color pass (the generator's biome functions are *retuned to match the hand-paint* on anchor planets — the procedural system learns from the hero art). Each anchor planet carries **8 hand-placed landmarks** (monoliths, ruin rings, the Beacon, Loom Gate sites) — the POI density contract (GDD §3.1) at story-grade quality.

### 5.2 Station Design Language — "functional futuristic, grown not placed"

- **Grammar:** modular habitat rings + cylinders, exposed truss, **4 docking spurs** with approach-light strings (Star Amber), 2 radiator wings (charcoal with heat-line veins), antenna masts. Every part *does a job*; nothing is decorative unless the job is "let the crew see the stars" (the observation dome).
- **Lived-in light:** station windows are the **only warm light in the void** — warm interior glow, some windows dark (shift change), a few flickering. The station reads as *occupied* at 10 km. This is the emotional register of "home" (GDD §7.3: MERIDIAN is a voice on a frequency; the station is where the voice lives).
- **Meridian Station (the home):** central truss, 3 habitat rings, 4 docking spurs, 2 radiator wings, 1 observation dome. The dome's inner light is a soft **Biolume Cyan → Star Amber** gradient — MERIDIAN's "face" is a *mood*, never a face.
- **Contrast rule:** Harvesters never build "stations" — their nests are *grown* (chitin over frame, no right angles, Rust Red at the seams). The player should feel the difference in grammar within one glance: **right angles = human; curves = grown; gold threads = Architects.**

### 5.3 Ship Design Language — *Astra* (A-009a)

- **Configuration (per spec): four-wing, fast and agile.** Long slender fuselage (4:1 length:width), **four wings in an X arrangement, fore/aft staggered** (two pairs, swept back 35°) — the stagger gives the dart a twisting, agile read at speed; the X reads cleanly at 500 m (silhouette test).
- **Visual mass forward:** cockpit is a single canopy slit near the nose; engines cluster at the tail (4 vectored nozzles + 1 main). The ship looks like it wants to be somewhere else.
- **Identity light:** one **Biolume Cyan strip** runs nose→tail along the fuselage spine (the ship's "name-light"; blinks twice on dock); engines glow **Star Amber**, overdrive shifts the glow toward white.
- **Scale cues:** the ship is *small* — 24 m, 12 t. Nothing in the ship's design should make the player feel powerful; it should make them feel *quick*.
- **Fleet contrast:** Courier Drones = fat, friendly, cargo-slewed (Amber); Harvester ships = grown, no right angles (Rust); derelicts = human grammar, dead light.

### 5.4 Silhouette & Style Rules (all hero assets)

1. **One shape, one story:** the ship is a dart; the Watcher is a lattice; the Phantasma is a person-shaped hole. If a silhouette can be mistaken for another species at 500 m, it fails.
2. **Two-color max in silhouette** (base + emissive).
3. **Nogreeble tax:** detail is *functional* (hatches, struts, masts) — ornament is forbidden on functional craft; ornament is *allowed* on Architect ruins (the only things in the game whose job is to be strange).
4. **Vertex-color paint pass** on every world asset: base biome color + AO baked into vertex darkening + one accent band. Painted in Blender, exported quantized to 64 colors (TDD §2.5/GDD §9.2).

---

## 6. Procedural Visualization

### 6.1 Planet Surfaces — No Texture Maps, Ever (A-001)

> **CPU-calculated biome vertex colors. Flat-shaded, varied landscape.**

- The surface is a heightfield (TDD §2.6) with **flat shading** — the faceted look *is* the style (a map drawn by hand, not a photo). Flat normals + vertex-color bands make every chunk read as **painted terrain**.
- **Variety comes from the function, tuned per biome:** amplitude / frequency / domain-warp / feature-set per biome (the artist's knobs, baked into the biome tables the generator reads):

| Biome | Height character | Feature set (instanced props + vertex paint) |
|---|---|---|
| Terrestrial | Rolling, mid amp | forests (canopy clumps), rivers (water-vertex channels), cliffs, meadow accents |
| Glacial | Smooth, high plateaus | ice shelves, geysers (emissive vents), open-water inlets, aurora-scorched flats |
| Volcanic | Jagged, high amp | lava rivers (emissive gradients), calderas, basalt columns, ash flats |
| Desert | Smooth + sharp mesas | dune bands (vertex stripe noise), mesas, oasis jewels |
| Ocean | Low amp + reef bumps | reef jewels, atolls, shelf cliffs, spray bands |
| Toxic | Rolling + fog pools | spore fields (emissive), acid pools, storm-cap ridges |
| Barren | Cratered | craters (nested rings), regolith ridges, Loomsteel glints |

- **Vertex-color "banding":** each feature writes a 2–4 vertex-color gradient (not a hard cut) so the landscape *shades* between biomes — the flat facets carry the style, the bands carry the depth.
- **Quality feel rule:** the player should be able to tell a Terrestrial from a Barren from *orbit* (global-sphere LOD) before scanning — the biome is legible at 100 km. That is the test for every biome's vertex-paint table.

### 6.2 Weather — Simple Particles (A-007)

**Four primitive particle types, mapped per biome** (all point/line sprites, one draw per type, TDD §2.7; budgets: ≤ 200 / 400 / 800 live particles at LOW / BALANCED / HIGH):

| Primitive | Spec | Used for |
|---|---|---|
| **Rain = lines** | 8–16 px line sprites, gravity + wind, teal-grey (Terrestrial) / **acid green `#A8E83A`** (Toxic) / dark `#2A3A4A` (Ocean squalls) | rain, acid rain, squalls |
| **Snow = slow particles** | 2–4 px soft points, low gravity, wind-drift, white | snow, spore drift (green variant), ash fall (charcoal variant) |
| **Storms = lightning flashes** | 3-frame double-flash: directional light pulse (1 frame hot white, 1 frame warm) + 1 screen-edge flash (10% white, 30 ms) + optional fork sprite (single-line, vertex-color) | Terrestrial storms, Ocean blackout squalls, Toxic supercell |
| **Sand/curtain = noise curtain** | full-screen noise-driven alpha curtain + streak particles, ochre (Desert) / orange shimmer (Volcanic radiation) | sandstorms, radiation storms |
| **Streaks = micrometeor** | fast white line streaks + impact puff (3-frame) | Barren showers |
| **Whiteout = fog** | distance-fog ramp + wind particles, `#C8D8E0` | Glacial whiteouts |

**Weather art rules:** weather is *readable before it is felt* — a storm is visible on the horizon 30 s before it arrives (the sky LUT's terminator curve drives the pre-warning band). Weather never blinds for > 5 s without an audio substitute (GDD §10: sound is a map).

### 6.3 Props & Landmarks

- All instanced, vertex-color, 200–2k tris (§5.1). Four prop families: **geological** (rocks, geodes, columns), **living** (forests, spores, reef), **ruin** (monoliths, ring fragments — Architect grammar: right angles *eroded by curvature*), **beacon** (human: Amber; Watcher: Cyan lattice; Harvester: Rust).
- **The monolith rule:** an Architect monolith is never taller than it is wide, never touches the ground at a right angle, and has exactly **one gold thread** — the Loom's signature, visible only up close (the player must *arrive* to read it).

---

## 7. UI / UX Art

### 7.1 Typography

- **Single family: JetBrains Mono (SIL OFL 1.1)** — weights 400 / 600 / 800. Monospaced is the cartographer's register: data, calm, exact. (A-009.)
- UI palette roles: **Biolume Cyan** = data / "NEW" / interactive-hover · **Star Amber** = primary action / hope · **Architect Gold** = story-critical (story missions, Signal Fragments, endings) · **Rust Red** = danger only · Void Indigo base.
- Type scale: HUD 14 px (12 px minimum), panels 16/20/28, story text 20 px / 1.5 line-height. **No text ever sits on busy color** — story text always on a 90%-Void-Indigo plate.

### 7.2 HUD — Minimal, Readable (A-009)

> **Speed, fuel, hull, O2, system name, FPS. Monospaced, uncluttered.** The default screen is the galaxy; the HUD is a *margin note*.

```
        [ SYSTEM: TAU CETI   |   SECTOR: ORION-7   |   60 FPS ]     ← top ribbon: system name + FPS (dev toggle F8), 12 px, 60% opacity
        [ ▸ LOCK: 4.2 ly · TERRESTIAL · HAZARD: STORM ]             ← lock ribbon (GDD §11.2)
                                                                            (60 FPS — cyan 60 / amber 45 / red 30)
                                                        ┌──────────────┐
                                                        │  atlas thumb │  ← 120 px, top-right (GDD §11.2)
                                                        └──────────────┘
        [ SCAN RING + prompt — center, context only ]
  ┌─────────────────────────────┐
  │ SPEED 04 210 m/s   ▸ overdrive armed │   ← speed readout: 6-digit monospace, left-bottom
  │ FUEL  ▮▮▮▮▯▯▯▯  61%      │   ← meter stack: 5 bars (GDD §11.2), 2 px tall,
  │ HULL  ▮▮▮▮▮▮▮▮▮▮  100%   │      no labels until hovered; Fuel/Hull/O2/Egy/Temp
  │ O2    ▮▮▮▮▮▮▮▮▮▮  100%   │
  │ EGY   ▮▮▮▮▮▮▮▮▯▯  80%    │
  │ TEMP  ▮▮▮▮▮▮▮▮▮▮  21°C   │
  └─────────────────────────────┘
```

**Meter bar design:** 10 segments (▮ = 10%), flat, 2 px tall, no borders. Color by state, not by meter: **Biolume Cyan** (> 50%) → **Star Amber** (20–50%) → **Rust Red** (< 20%, 1 Hz pulse). The *only* meter that may show a number is **Speed** (the player's instrument) and **Temp** (°C, because "feeling" needs a unit). Fuel reads % when < 30% (scarcity earns the digits — GDD §5.2).
**FPS readout:** top-right of the system ribbon, 12 px; colored by preset floor (cyan ≥ preset target / amber ≥ 30 / red < 30); toggleable, on by default in the first session (transparency about the iGPU budget is a trust feature).

### 7.3 Star Atlas — Map Art (affirms GDD §6.4; A-010)

- **Layer 1 (Galaxy): top-down Milky Way** — the spiral drawn as **arm-light**: soft Biolume-Cyan/Indigo arm bands on Void Indigo, core glow Star Amber (dimmed — the Core is *where you must not look yet*; it brightens 10% per act, the map itself foreshadowing), halo dust faint. **Spiral arms** are navigational furniture: arm names (Orion, Perseus, Sagittarius, Scutum-Crux) in 12 px caps along the arm.
- **Player marker:** a small **Biolume Cyan chevron** pointing in the warp direction + a 10-ly reach circle (GDD §3.1 promise circle) at 20% opacity. The chevron is the only "self" in the game's art.
- **Fog of war:** Layer 1 fully lit (the galaxy is known); Layers 2–3 reveal by scanning/warping — unrevealed is not "black", it is **Void Indigo at 40% with a 1-px dot-grid** (a chart awaiting ink).
- **Icon system (shape = category, color = meaning — A-010):**

| Icon | Shape | Color | Meaning |
|---|---|---|---|
| **Anchor** | ◆ diamond in a **gold ring** | **Architect Gold `#E8C547`** | **Real celestial body (of the 430)** — the map's poetry |
| **Black hole / Siphon** | ● circle in a **red ring** | **Rust Red `#C0392B`** (+ gold thread if story-gated) | BH / Siphon — the map's dread |
| Star (generic) | ● | class color (Appendix B) | system |
| Watcher nest | ◈ lattice dot | Biolume Cyan | passive, data-rich |
| Harvester zone | ▲ rust triangle | Rust Red | hostile territory |
| Loom Gate | ◇ gold-outlined diamond | Architect Gold | story gate |
| Derelict | ⬠ | pale grey | salvage |
| Salvage beacon | ▽ | Biolume Cyan | your lost cargo |
| Grave marker | ✝ (thin) | pale grey | your history (GDD §5.5) |
| P7 explorer presence | ◦ | Biolume Cyan 40% | another explorer (Phase 7) |

- **Icon rule:** every icon is **≤ 24 px, 2 colors, one shape family**; the map is readable at 50% zoom on a 720p screen (LOW preset). The gold/red pair (anchors/BH) is the map's *emotional axis*: gold is the galaxy's memory, red is the galaxy's wound.

### 7.4 Menus & Cutscenes — Minimalist (A-011)

- **Menus:** full-bleed **still** (a cached in-engine render of the player's last region — *the menu is your galaxy*) at 90% Void-Indigo plate, monospace type, one Biolume Cyan rule line. No menus "themed" — one menu style for everything (main, load, settings, Codex, Atlas) with the same 3-element grammar: **plate / rule line / type**.
- **Cutscenes (GDD §7.5: no forced cutscenes; ≤ 40 s; skippable):** **scripted camera + static images with text** — each beat is a **pre-rendered in-engine still** (4K, offline render at build time, ZSTD in `assets/`, ≤ 40 stills, ~8 MB) with a **slow scripted camera move** (Ken Burns drift, ≤ 1.5%/s — the "camera" is a parallax pan/zoom over the still, not a render) + **typewriter text** (JetBrains Mono, 14 ms/char) + audio. **No real-time animation cutscenes at launch** — the iGPU budget and the 40 s rule both point the same way, and a beautiful still is *honest* about what the game is.
- **Story text plates:** 90% Void Indigo, 280 px wide, one Biolume Cyan rule, Gold for Signal Fragments (the Architects' text is Gold-tinted and 200 ms slower typewriter — the machine speaks more slowly than the player).

---

## 8. Visual References (Placeholder Library)

**Convention:** reference images are **internal mood boards only — never shipped, never in the zip** (A-012). Files land in `art/refs/<element>-NN.<ext>`; each slot below names the *kind* of reference and its source; "TO PLACE" slots are empty by design until the team collects them. Status: `NAMED` (source identified, image not yet captured) · `PLACED` (file in `art/refs/`) · `REJECTED`.

### 8.1 Siphon / Black Hole

| Slot | Reference | Study | Status |
|---|---|---|---|
| `[VISUAL REF — SIPHON-01]` | *Interstellar* (2014) — Gargantua over Miller's planet | the "hat" silhouette; disk-over-horizon geometry | NAMED |
| `[VISUAL REF — SIPHON-02]` | *Interstellar* (2014) — Endurance flyby | lensing starfield arcs; scale | NAMED |
| `[VISUAL REF — SIPHON-03]` | *The Expanse* (S5) — Eros/Protogenium visuals (black-hole drama) | how a showpiece is *used* in narrative beats | NAMED |
| `[VISUAL REF — SIPHON-04]` | Real: EHT M87* / Sgr A* images | photon-ring proportion honesty | NAMED |
| `[VISUAL REF — SIPHON-05]` | (Doppler-beamed disk renders, academic) | approaching/receding side brightness delta | TO PLACE |
| `[VISUAL REF — SIPHON-06]` | (Our own BALANCED-tier test renders) | the internal standard | TO PLACE (P5) |
| `[VISUAL REF — SIPHON-07]` | (LOW-tier fallback render — must read identical) | fallback QA | TO PLACE (P5) |
| `[VISUAL REF — SIPHON-08]` | (Volumetric HIGH-tier test) | volume feel | TO PLACE (P6) |

### 8.2 Xenothrix Phantasma

| Slot | Reference | Study | Status |
|---|---|---|---|
| `[VISUAL REF — PHANTOM-01]` | *Annihilation* (2018) — the Bird | "the thing you can't quite see" tension; edge glow | NAMED |
| `[VISUAL REF — PHANTOM-02]` | *Subnautica* — Reaver Leviathan | biolume scale silhouette; awe-not-jumpscare | NAMED |
| `[VISUAL REF — PHANTOM-03]` | *Annihilation* (2018) — refracted forest | *absence* as a visual material (our starfield cut-out) | NAMED |
| `[VISUAL REF — PHANTOM-04]` | Cherenkov radiation photography (reactor pools) | the blue-white edge color standard | NAMED |
| `[VISUAL REF — PHANTOM-05]` | (Phases: transparency flicker studies) | limb-phasing rhythm — must read "rag-doll almost-coherence" | TO PLACE |
| `[VISUAL REF — PHANTOM-06]` | (Eye-tracking study frames) | the 400 ms delay — it must feel *attentive*, not mechanical | TO PLACE |
| `[VISUAL REF — PHANTOM-07]` | (Encounter sequence timing board) | 60 s beat sheet as frames | TO PLACE (P5) |
| `[VISUAL REF — PHANTOM-08]` | (LOW-tier silhouette only) | fallback QA | TO PLACE (P5) |

### 8.3 Stations & Ships

| Slot | Reference | Study | Status |
|---|---|---|---|
| `[VISUAL REF — STATION-01]` | *No Man's Sky* — space stations | modular grammar at distance | NAMED |
| `[VISUAL REF — STATION-02]` | *The Expanse* — Canterbury-class / orbital stations | "grown not placed" human craft | NAMED |
| `[VISUAL REF — STATION-03]` | (Meridian Station hero render — 80k LOD0) | the internal standard: warm windows in cold void | TO PLACE (P3) |
| `[VISUAL REF — SHIP-01]` | (4-wing X-arrangement studies, concept) | stagger + silhouette dart-read | TO PLACE (P2) |
| `[VISUAL REF — SHIP-02]` | *No Man's Sky* — Atlas-class / fast interceptors | "quick, not powerful" register | NAMED |
| `[VISUAL REF — SHIP-03]` | (Astra hero render + 500 m silhouette test) | asset QA standard | TO PLACE (P3) |
| `[VISUAL REF — SHIP-04]` | (Harvester grammar contrast frame: human right-angles vs grown curves) | species legibility at a glance | TO PLACE (P5) |

### 8.4 Biomes & Surfaces

| Slot | Reference | Study | Status |
|---|---|---|---|
| `[VISUAL REF — BIOME-01]` | *No Man's Sky* — biome palette frames | saturation level for "bold" without "clown" | NAMED |
| `[VISUAL REF — BIOME-02]` | *Monument Valley* / *Journey* — palette restraint | accent-jewel discipline (10% rule) | NAMED |
| `[VISUAL REF — BIOME-03]` | *Slime Rancher* / *Deep Rock Galactic* — flat-shaded vertex-color worlds | the faceted style reference | NAMED |
| `[VISUAL REF — BIOME-04]` | (7 biome palette boards, hand-painted) | the artist's canonical color sheets | TO PLACE (P1) |
| `[VISUAL REF — BIOME-05]` | (Orbit-legibility test: 7 biomes at global-sphere LOD) | the 100 km legibility standard | TO PLACE (P3) |

### 8.5 Skies, Nebulae, Weather

| Slot | Reference | Study | Status |
|---|---|---|---|
| `[VISUAL REF — SKY-01]` | (12 sky-LUT contact sheet) | the LUT as an art deliverable | TO PLACE (P2) |
| `[VISUAL REF — SKY-02]` | *Interstellar* — Earth sunset frames | terminator color temperature | NAMED |
| `[VISUAL REF — NEB-01]` | Hubble M42 / Carina imagery | filament structure for vertex-noise slices | NAMED |
| `[VISUAL REF — NEB-02]` | (Billboard-slice stack test, 12 slices) | slice count / parallax feel | TO PLACE (P3) |
| `[VISUAL REF — WTHR-01]` | (Weather 4-primitive contact sheet) | rain/snow/storm/acid at budget | TO PLACE (P4) |
| `[VISUAL REF — WTHR-02]` | *Subnautica* — storm/rain readability | "readable before felt" standard | NAMED |

### 8.6 UI / Map

| Slot | Reference | Study | Status |
|---|---|---|---|
| `[VISUAL REF — UI-01]` | *Subnautica* HUD — minimal meter readability | margin-note HUD register | NAMED |
| `[VISUAL REF — UI-02]` | *Elite Dangerous* — galactic map icons | icon density at 50% zoom (the counter-example: too busy) | NAMED |
| `[VISUAL REF — UI-03]` | (HUD full-state mockup, 720p) | LOW-preset readability standard | TO PLACE (P3) |
| `[VISUAL REF — MAP-01]` | (Star Atlas Layer-1 mockup with 430 anchors placed) | gold/red emotional axis; arm light | TO PLACE (P3) |
| `[VISUAL REF — MENU-01]` | (Menu + cutscene still contact sheet) | plate / rule line / type grammar | TO PLACE (P4) |

**Adding references:** copy the template in [Appendix D](#appendix-d--visual-ref-slot-template), place the file at `art/refs/<ELEMENT>-NN.png`, set status `PLACED`, and log any resulting style decision as `A-###`.

---

## 9. Asset Pipeline, Naming & QA

### 9.1 Toolchain

| Stage | Tool | Output |
|---|---|---|
| Sculpt / model | **Blender 4.x** (hi-poly source, A-008) | `.blend` in `art/src/` |
| Retopo / reduce | Blender (Decimate + manual where faces matter) | `.glb` deliverable |
| **Vertex-color paint** | Blender (vertex paint + 64-color quantize node) | vertex color in `.glb` |
| Export | Python script (`tools/` — TDD §5.4 lineage) | `.astromesh` (ZSTD, TDD §2.4) into `assets/` |
| Sky/nebula precompute | Python (offline) | sky LUTs `.lut`, nebula slice vertex data |
| Cutscene stills | offline in-engine render (`astra --render-still`) | 4K → ZSTD into `assets/` |
| Font | JetBrains Mono (OFL) subset | `assets/ui.astropack` |

### 9.2 Naming

```
art/src/<category>/<name>-<variant>-<LOD>.blend      (e.g. art/src/ship/astra-01-LOD0.blend)
art/refs/<ELEMENT>-NN.<ext>                          (§8 slots)
assets: heroes.astropack / ui.astropack / audio.astropack (TDD §5.3 tree)
```

Categories: `ship · station · planet · alien · prop · ui`. Variants zero-padded. LOD0 = deliverable.

### 9.3 Asset QA Gates (every hero asset passes all five before merge)

1. **Silhouette test:** black-on-white, 500 m equivalent, 2 s identification (species/object class).
2. **Two-color test:** strip all but base + emissive — still reads.
3. **Color-blind pair test:** adjacent-hue confusables (Protanopia/Deuteranopia sim) on the palette board.
4. **Black-reservation audit:** no pure `#000000` outside Siphon horizon / Phantasma / true void (scripted scan of vertex colors).
5. **Budget audit:** tri + texture memory against §5.1 table (scripted against the manifest).
6. **iGPU spot check:** the asset renders at frame budget on the reference box at HIGH preset (per-asset, in scene).

---

## 10. Art Production Plan (by Phase — maps to GDD §13)

| Phase | Art deliverables | Gate |
|---|---|---|
| **P1 Pre-Production** | Full palette boards (Appendix A) approved; 7 biome mood one-liners; **all 39 reference slots in §8 defined with status, 20+ named**; 2 concept sketches (Siphon, Phantasma); font subset; icon glyph table (Appendix C) final | Palette sign-off = P1 exit item |
| **P2 Engine** | Vertex-color pipeline E2E (the lit cube → a painted asteroid); sky LUT v1 (4 atmosphere classes); starfield 3-layer look dev; SHIP-01 wing studies; BIOME-04 palette boards (PLACED) | A painted asteroid on the reference iGPU at 60 FPS |
| **P3 Exploration** | **Astra ship** (LOD0–3) + STATION-03 Meridian render; first 3 anchor planets (Thessaly, Proxima, Sirius); 7 biomes in-game + BIOME-05 orbit-legibility test; Star Atlas Layer-1 look (MAP-01); HUD full mockup (UI-03); nebula slices v1 (NEB-02) | A new player's first 3 hours is *beautiful* by playtest word-of-mouth |
| **P4 Survival** | Weather 4-primitives live (WTHR-01); UI meter states + FPS readout; menu/cutscene grammar + first 10 story stills (MENU-01); toxic/acid palette tuning | Playtesters report "the meters read at a glance" unprompted |
| **P5 Story/Aliens/Phantom** | **AAV-1 Siphon final** (all 3 tiers + SIPHON-06/07 renders); **AAV-2 Phantasma final** (PHANTOM-05/06/07); Watcher + 3 Harvester tiers + SHIP-04 grammar contrast; 5 remaining anchor planets; Exotic/Loom field look; Loom Gates | The two AAVs hold 2 ms post budget; χ² spawn + "blindest spot" playtest |
| **P6 Polish** | Full palette pass (every scene vs Appendix A); nebula/sky final; HIGH-tier volumetric + particles; cutscene stills complete (≤ 40); palette hex match screenshot-diff (GDD §13 P6 exit) | Blind playtest: top-2 visuals named unprompted |
| **P7 Multiplayer** | Presence icon (◦), P7 map layer art, Survey Ledger UI | presence reads at 50% zoom |
| **P8 QA/Launch** | Art regression suite (20 canonical scenes, screenshot-diffed per build); LOW-tier full pass on reference iGPU; reference library archived (all `PLACED` slots in `art/refs/`) | palette + AAV renders match the signed-off boards |

---

## 11. Art Decision Log

**Format:** `A-###` · date · decision · rationale · status. Append-only; design-affecting entries get a GDD §17 pointer.

| ID | Date | Decision | Rationale | Status |
|---|---|---|---|---|
| A-001 | 2026-09-27 | **No texture maps on world geometry, ever. Biome vertex colors, CPU-computed, flat-shaded.** | Affirms GDD §9.2 and the memory budget; the faceted look is the style, not a compromise. "The art is the algorithm" (§1.1 P4). | ACCEPTED |
| A-002 | 2026-09-27 | **Biome palettes extend GDD §9.3's master signal palette** (7 biomes + Exotic/Loom, Appendix A). Volcanic "black" = charcoal `#2A2226` to preserve the black-reservation rule. | The GDD locked the *signal* palette (UI/species); the *world* palette is this document's jurisdiction. Charcoal keeps Phantasma/void as the only pure black. | ACCEPTED |
| A-003 | 2026-09-27 | **Exotic purple/warp is reserved for Loom phenomena and warp space only** — no planet biome uses it. | Purple must be a story signal ("the Loom reached here"), not decoration. Also isolates the most shader-expensive effect (warp shimmer) to the moments that pay for it. | ACCEPTED |
| A-004 | 2026-09-27 | **Siphon lensing renders at ½ resolution offscreen + upscale; LOW fakes the "hat" with a bent disk strip.** | The bend is low-frequency — half-res loses nothing perceptible and fits the 2 ms post budget (TDD T-007) on Vega 8. Fallback must read identical to HIGH (GDD §3.5). | ACCEPTED |
| A-005 | 2026-09-27 | **Phantasma form: tall, thin humanoid (9:1), crouched many-jointed encounter pose; Cherenkov blue-white edge; independent limb phasing; void face with 2–7 tracking eye-points (400 ms delay; drift-when-unlooked-at).** | Refines GDD §8.4's "starfield silhouette body" into a buildable form; the humanoid proportion is what makes the scale *wrong* in the exact way the GDD's "unsettling" tone demands. GDD §8.4 pointer added. | ACCEPTED |
| A-006 | 2026-09-27 | **Skies = pre-computed scattering LUTs (256², 12 atmosphere classes × 8 sun angles, offline Python); nebulae = 6–12 crossed billboard slices with vertex-noise filaments (no textures).** | Zero runtime scattering cost on the iGPU; the LUT is an *art deliverable* (the sky is designed, not simulated). Slices keep nebulae inside the "no textures" rule while reading as gas. | ACCEPTED |
| A-007 | 2026-09-27 | **Weather = 4 primitive types (rain-lines, snow-slow-particles, storm-lightning-flashes, acid-green-particles) + 2 extensions (curtain, streaks), mapped per biome; budgets 200/400/800 live particles.** | The four primitives are the whole weather vocabulary — every biome's weather is a *recoloring* of a primitive, which keeps the particle system one draw per type and the art tunable per-biome via color tables only. | ACCEPTED |
| A-008 | 2026-09-27 | **Anchor planets: high-detail Blender sculpts as canonical source → reduced 512×256 hero sphere + 8 hand-placed landmarks each; the procedural generator is retuned to match the hand-paint on anchor planets.** | "Hero detail" is a *source* concept, not a runtime cost — the iGPU gets the reduced deliverable, the artist gets a sculpty source of truth, and the 430-anchor promise gets story-grade quality on the 8 that matter. | ACCEPTED |
| A-009 | 2026-09-27 | **UI: JetBrains Mono (OFL) only; HUD = speed/fuel/hull/O2/egy/temp + system name + FPS; meters color-by-state (cyan→amber→rust), 2 px, no labels until hover; ship *Astra* = four-wing X-staggered dart, Biolume Cyan spine strip, Star Amber engines.** | Monospace is the cartographer's register and one family halves the UI art scope. Color-by-state (not per-meter) keeps 5 meters at 500 px of HUD. The four-wing config is the player's identity — silhouette dart + one cyan line. | ACCEPTED |
| A-010 | 2026-09-27 | **Star Atlas art: top-down Milky Way with arm-light; gold ring = anchors, red ring = BH/Siphon (the map's emotional axis); fog = Indigo dot-grid (never black); player = cyan chevron + 10-ly promise circle.** | The map is the game's thesis rendered (GDD §3.1 "the map is a promise"): gold = the galaxy's memory, red = its wound. Dot-grid fog respects the black-reservation rule while reading as "chart awaiting ink". | ACCEPTED |
| A-011 | 2026-09-27 | **Menus/cutscenes: pre-rendered in-engine stills + slow scripted camera (Ken Burns ≤ 1.5%/s) + typewriter text; no real-time animation cutscenes at launch. ≤ 40 stills, ~8 MB.** | The iGPU budget, the 40 s rule, and the minimalist register all point one way; a beautiful still is honest about what the game is, and the audio (GDD §10) carries the drama. | ACCEPTED |
| A-012 | 2026-09-27 | **Reference images are internal mood boards only — never shipped, never in the zip; rights noted per slot in `art/refs/`.** | References (film stills, game captures, photos) carry copyright; the shipped product uses only original art. The placeholder library (§8) keeps the mood board honest without legal exposure. | ACCEPTED |
| A-013 | 2026-09-27 | **Phantasma encounter audio timing per AudioDesign AU-002 (pointer to GDD D-019): 2 s ambient/music fade to silence (replacing "0–10 s cut"), 18 Hz infrasonic bed from t=2 s, breathing at 3 s, sub-bass note at 45–55 s HRTF-positioned at the creature's face (replacing "behind the shoulder").** §4.2 sequence updated in place. | Audio design is owned by the audio document; the visual document keeps the 60 s beat sheet but defers all audio timing. The face-positioned note is a visual-true refinement (the player has been staring at the face — the sound should come from where the dread is). | ACCEPTED |

---

## Appendix A — Full Palette (hex)

**Master signal (GDD §9.3):** Void Indigo `#0B0E2A` · Star Amber `#FFB347` · Biolume Cyan `#4DF0E0` · Rust Red `#C0392B` · Architect Gold `#E8C547` · Phantom Void `#000000` (reserved).

**Biomes (base / secondary / accent / hazard / emissive):**
- Terrestrial: `#3FA86A` / `#7A5C3E` / `#2E9BD6` / `#8A94A8` / `#FFB347`
- Glacial: `#DFF3F7` / `#7FD4E8` / `#1E5E7A` / `#C8D8E0` / `#4DF0E0`
- Volcanic: `#2A2226` / `#3A2E33` / `#FF5A2A`→`#FFB347` / `#FF7A3D` / `#FF7A3D`
- Desert: `#E0A458` / `#B0684A` / `#4DA8A0` / `#C88A4A` / —
- Ocean: `#2E8FBF` / `#123E5E` / `#5AD9A0` / `#2A3A4A` / `#4DF0E0`
- Toxic: `#8FB83A` / `#4A5E2A` / `#C8F04C` / `#A8E83A` / `#C8F04C`
- Barren: `#6A6A72` / `#52525A` / `#E8C547` / `#E8E8F0` / —
- **Exotic/Loom:** `#4A2A6A` / `#7A3ABF` / `#B06AE8` / `#FFFFFF` / `#E8C547`

**Phantasma-only:** Cherenkov rim `#DDEBFF` → `#7FD4E8` · eye-points `#F4F8FF` · shard prism `#DDEBFF`.

## Appendix B — Star Class → Color

| Class | Hex | Note |
|---|---|---|
| O | `#9BB0FF` | blue-white, harsh; radiation-band hazard |
| B | `#AABEFF` | blue-white |
| A | `#CAD6FF` | white-blue |
| F | `#F6F7FF` | white |
| G | `#FFF4E8` | warm white (Sol, Tau Ceti home-light: `#FFB347`-tinted warm) |
| K | `#FFD2A8` | orange |
| M | `#FFB5A5` | deep red-orange; long shadows, cold red light |
| WD | `#E8F4FF` | pale blue-white; faint, dense |
| NS | *(no visible body)* + `#9BD8FF` X-ray corona glow | the "should not be visible" cue |
| **Siphon** | horizon `#000000` + disk `#FFB347`→`#FFFFFF` + **gold thread if story-gated** | AAV-1 |
| Anchor star (any class) | class color **+ gold 4-pt spike + 4× size tick** | the 430 promise (GDD §6.2) |

## Appendix C — Icon Glyph Table

| Glyph | Code | Meaning | Color | Min size |
|---|---|---|---|---|
| ◆ in ring | `ICON_ANCHOR` | Real anchor body | Architect Gold | 16 px |
| ● in ring | `ICON_BH` | Black hole / Siphon | Rust Red | 16 px |
| ● | `ICON_STAR` | Generic system | class color | 10 px |
| ◈ | `ICON_WATCHER` | Watcher nest | Biolume Cyan | 12 px |
| ▲ | `ICON_HARVESTER` | Harvester zone | Rust Red | 14 px |
| ◇ outlined | `ICON_LOOMGATE` | Loom Gate | Architect Gold | 18 px |
| ⬠ | `ICON_DERELICT` | Derelict | `#9A9AA4` | 12 px |
| ▽ | `ICON_SALVAGE` | Salvage beacon | Biolume Cyan | 12 px |
| ✝ thin | `ICON_GRAVE` | Grave marker | `#9A9AA4` | 12 px |
| ◦ | `ICON_PRESENCE` | Explorer presence (P7) | Biolume Cyan 40% | 10 px |
| ▸ | `ICON_PLAYER` | Player chevron + 10-ly circle | Biolume Cyan | 20 px |

All glyphs: 2 colors max, one shape family, renderable at 50% zoom on 720p (LOW).

## Appendix D — `[VISUAL REF]` Slot Template

```markdown
| `[VISUAL REF — <ELEMENT>-NN]` | <source: title (year), or internal render> | <what to study — one line> | NAMED / TO PLACE / PLACED / REJECTED |
```

File path when placed: `art/refs/<element>-NN.png` (lowercase, NN zero-padded). Rights note goes in `art/refs/NOTES.md` next to the file. A `PLACED` slot that changes a style decision must be logged as `A-###` in §11.

---

*End of Art Bible v1.1 (re-issued). Next review: Phase 1 exit (palette boards + §8 slots — 39 defined, 20+ named). Visual authority for all renders, palettes, and asset QA from this date.*
