---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Target native LV2 and CLAP, with VST3 via clap-wrapper

## Context and Problem Statement

Having decided to ship Hydrogen as a plugin ([ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md)),
we must choose which plugin format(s) to support. The formats differ in license,
per-OS host coverage, API style, and maintenance cost. Hydrogen is licensed
**GPLv2-or-later** (`COPYING`, "version 2 … or any later version"), which
constrains the SDKs we may link.

## Decision Drivers

* **License compatibility** with GPLv2-or-later.
* **Host coverage** across Linux, Windows, and macOS — Hydrogen's primary
  Linux hosts (Ardour, Qtractor, Carla) are LV2-first; commercial DAWs on Windows
  and macOS need VST3 and increasingly support CLAP.
* **Maintenance surface**: one well-built engine wrapper should yield as many
  formats as possible, not N hand-written codepaths.
* **API quality**: a clean threading contract that matches Hydrogen's
  lock-based engine.

## Considered Options

1. **Native LV2 + native CLAP, then CLAP → VST3 via `clap-wrapper`.**
2. Use a single abstraction framework (e.g. DPF / DISTRHO) emitting all formats.
3. Hand-write each format (LV2 + VST3 separately).
4. JUCE.

## Decision Outcome

Chosen option: **native LV2 + native CLAP, with VST3 produced from the
CLAP build via the official [`clap-wrapper`](https://github.com/free-audio/clap-wrapper).**

The target set is **LV2, CLAP, and VST3 only.** Apple's AU is explicitly **not**
a target (see consequences); Logic/GarageBand are out of scope.

Rationale per format:

| Format | License | Linux | Win | macOS | How we ship it |
|---|---|---|---|---|---|
| **LV2** | ISC (permissive) | ✅ native home | ✅ | ✅ | Native target |
| **CLAP** | MIT | ✅ | ✅ | ✅ | Native target (primary) |
| **VST3** | GPLv3 *or* proprietary (OK via "or later") | ✅ | ✅ | ✅ | Via clap-wrapper |
| AU | Apple SDK | ❌ | ❌ | ✅ | **Not targeted** |
| VST2 | SDK withdrawn 2018 | — | — | — | **Skipped** (no legal SDK) |
| AAX | Avid proprietary + PACE | ❌ | ✅ | ✅ | **Skipped** (GPL-incompatible) |

* **CLAP (MIT)** is the primary native target: header-only, trivially vendored
  into a GPL project, and with an explicit main-thread/audio-thread contract
  that maps onto Hydrogen's engine. `clap-wrapper` re-exposes a CLAP as VST3 with
  negligible extra code, giving all three target formats from one engine
  integration.
* **LV2 (ISC)** is shipped natively rather than via wrapper because it is
  Hydrogen's native ecosystem, has no GPL friction, and — importantly — its
  separate-binary UI model is exactly the out-of-process UI we want
  ([ADR 0016](0016-out-of-process-plugin-ui.md)).
* **VST3's** dual license is acceptable precisely because Hydrogen is "or
  later": we distribute the VST3 build under GPLv3.

Rejected:

* **DPF / single framework** — viable and lower-overhead in theory, but it
  imposes its own UI/state model that conflicts with our out-of-process Qt UI
  and embed-song-in-state decisions; we prefer to own the integration seam.
* **Hand-writing VST3** separately — duplicates the engine seam.
* **JUCE** — GPLv3-or-commercial, drags in its own GUI framework that clashes
  with Qt, and has no first-class CLAP.

### Consequences

* Two native integration seams to maintain (LV2 and CLAP); VST3 rides along.
* The engine↔host abstraction (audio buffers, MIDI, transport) must be format-
  neutral so LV2 and CLAP share it. This is the `PluginAudioDriver` /
  `PluginMidiDriver` work in [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md).
* `clap-wrapper` becomes a build dependency for the VST3 artifact.
* **No AU**, so **Logic Pro and GarageBand are not supported.** On macOS users
  reach Hydrogen through CLAP- or VST3-capable hosts (e.g. Reaper, Bitwig, Live).
  AU could be added later via `clap-wrapper` if demand warrants, with no engine
  changes — only build/packaging work.
* No VST2 and no AAX — communicated as a deliberate, license-driven choice.

## More Information

* CLAP: <https://github.com/free-audio/clap>
* clap-wrapper: <https://github.com/free-audio/clap-wrapper>
* LV2: <https://lv2plug.in/>
* Related: [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md),
  [ADR 0016](0016-out-of-process-plugin-ui.md)
