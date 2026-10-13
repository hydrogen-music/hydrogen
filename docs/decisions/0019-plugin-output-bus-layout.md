---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Plugin outputs are a master bus plus a compile-time-fixed bank of N stereo buses

## Context and Problem Statement

Hydrogen can route each instrument to its own output today, but only over JACK:
per-instrument stereo pairs are gated by `Preferences::m_bJackTrackOuts`, created
per `Instrument::Id` when a kit loads (`AudioEngine::createPerTrackJackAudioPorts`),
and the sampler mixes each note to **both** its per-track buffer (pre-fader) and
the master (post-fader) in one pass (`Sampler.cpp:1720`). The master stereo bus
(`Sampler::m_pMainOut_L/R`) always exists. Instrument count is **dynamic and
unbounded** (`InstrumentList` is a `std::vector`).

We must expose per-instrument routing in the plugin, but plugin formats do not
share JACK's freedom to add/remove ports at runtime:

* **LV2 ports are fully static** — declared in the `.ttl` at discovery; a plugin
  instance cannot add audio ports.
* **VST3** buses are fixed at creation; **CLAP** can rescan audio ports but host
  support varies.

So "one port pair per instrument, created on load" cannot map onto a static
plugin port count, while the kit's instrument count varies and the user may
add/remove/move instruments at runtime.

## Decision Drivers

* Preserve per-drum routing — a real Hydrogen feature.
* A **static, host-friendly** port count that works in LV2/VST3/CLAP.
* Stable routing across add/remove/move/reload of instruments.
* A user who ignores the extra buses must still get a normal stereo mix.

## Considered Options

1. **Master + a fixed bank of N stereo buses**, N chosen at **compile time**.
2. Stereo master only (v1).
3. Per-format configurable port counts (CLAP `audio-ports-config`, fixed LV2
   variants).

## Decision Outcome

Chosen option: **master + a fixed bank of N stereo buses, with N a compile-time
CMake option** (`H2_PLUGIN_OUTPUT_BUSES`, default **32**).

* The **master bus always carries the full mix**, so ignoring the extra buses
  yields ordinary stereo.
* **Default mapping is 1-to-1:** the first N instruments (by list order) feed
  buses 1…N; per-instrument audio is **pre-fader**, matching today's JACK
  track-out behaviour. This default ships as-is and needs no routing UI — it is
  already a good mapping.
* **No *automatic* wrapping.** When a kit has more than N instruments, the
  surplus (and any unmapped instrument) routes to **master only** — it simply
  gets no individual out. The engine never *silently* sums two instruments onto
  one individual bus.
* **Manual sharing is allowed.** A user *may* deliberately route more than one
  instrument to the same bus through the remapping widget below; what is rejected
  is doing so *automatically*. The (custom) mapping is persisted in plugin state
  and survives add/remove/move and reload.
* Buses with no instrument routed to them are silent.

#### Remapping widget — late-stage convenience

A GUI widget lets the user change the default mapping (reassign instruments to
buses, including assigning several instruments to one bus). This is a **convenience
feature that can land at a late stage** of the implementation — the 1-to-1 default
is sufficient to ship without it. It also gives LV2 users a way to control routing
despite the static, generic bus names (below). Until the widget exists, the
mapping is the implicit 1-to-1 default and no custom mapping is persisted.

#### Bus naming

Where the format permits, the N buses are **named after the assigned instrument**,
which is what makes a large bank usable. Capability differs by format (verified
against the SDKs):

* **CLAP** — `clap_audio_port_info.name`; names can be updated **while active** via
  `host.audio_ports.rescan(CLAP_AUDIO_PORTS_RESCAN_NAMES)`. **We target dynamic
  renaming to the instrument names** as the kit/mapping changes.
* **VST3** — `BusInfo::name`; update then `restartComponent(kIoTitlesChanged)` so
  the host re-queries `getBusInfo`. **We target dynamic renaming to the instrument
  names** (inherited through `clap-wrapper`).
* **LV2** — `lv2:name` is **static**, fixed in the `.ttl` at discovery; there is no
  clean runtime rename (Dynamic Manifest regenerates the whole manifest and is not
  suitable). LV2 therefore uses **fixed generic labels** (`Out 1`…`Out N`) that
  cannot reflect instrument names — a documented LV2-only limitation, and the
  reason the remapping widget matters there.

Because LV2 ports are static, the LV2 target's `.ttl` is **generated at build time
from N** (with a `stereo` variant alongside the `multi` variant). VST3 inherits the
layout through `clap-wrapper`.

N is compile-time (not runtime) because LV2 cannot vary port count per instance;
making it a build option lets distributors choose 16/32/64 without code changes,
while keeping every produced binary static and host-friendly. (CLAP and VST3 *can*
change port count at runtime — CLAP `RESCAN_LIST` while inactive, VST3 dynamic I/O —
so growing the bank beyond N on those two formats is a possible later enhancement;
LV2 keeps N as the portable baseline.)

Stereo-only (option 2) is rejected as dropping a real feature. Fully per-format
configurable (option 3) is rejected as much more work for little gain given LV2
must stay static anyway — though CLAP `audio-ports-config` presets remain a
possible later enhancement.

### Consequences

* The `.ttl` generation and the plugin's declared bus layout are driven by
  `H2_PLUGIN_OUTPUT_BUSES`; build tooling must template the `.ttl`.
* Kits with more than N instruments are fully supported by the default 1-to-1
  mapping: the first N get individual outs, the rest play through the master only.
  No bus is *automatically* shared.
* v1 ships with the implicit 1-to-1 default and **dynamic bus renaming on
  CLAP/VST3**. The **remapping widget** and the **persisted custom mapping** it
  produces (which may include several instruments on one bus) are a **late-stage**
  addition; when added, the custom mapping must round-trip in the embedded song
  state ([ADR 0017](0017-embed-song-in-plugin-state.md)) under format versioning.
* On CLAP/VST3 the plugin renames buses to the instrument names when the kit or
  mapping changes (rescan / `kIoTitlesChanged`); on LV2 names are fixed generic
  labels at build time.
* The sampler's existing dual-write (per-track + master) path is reused; the
  plugin supplies bus buffers where the JACK driver supplied track buffers.

## More Information

* `src/core/IO/JackDriver.h` (`InstrumentPorts`, `getTrackBuffer`),
  `src/core/AudioEngine/AudioEngine.h` (`createPerTrackJackAudioPorts`),
  `src/core/Sampler/Sampler.cpp:1720`, `src/core/Basics/InstrumentList.h`.
* Related: [ADR 0014](0014-plugin-format-strategy.md),
  [ADR 0017](0017-embed-song-in-plugin-state.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
