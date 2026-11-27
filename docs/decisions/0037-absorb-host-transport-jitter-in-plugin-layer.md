---
status: accepted
date: 2026-11-05
deciders: pm
---

# AD: Absorb host transport jitter in the plugin layer

## Context and Problem Statement

[ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md) made plugin transport
host-driven: every `process()` block the shim reads the host position and
publishes it via `PluginAudioDriver::setHostTransport()`, and the engine's
`followHostTransport` (`AudioEngine.cpp`) follows it. The engine contract is a
**contiguous clock**: while rolling, each block must present exactly
`previous frame + nFrames`.

The CLAP transport the shims receive does not meet that contract:

* CLAP conveys position as `clap_sectime` — seconds in 32.32 fixed point
  (`CLAP_SECTIME_FACTOR = 1<<31`, `extern/clap/include/clap/fixedpoint.h`).
  Converting to frames (`seconds × sampleRate`) is **lossy**: at 48 kHz the
  representable step is coarser than one frame, so the round-tripped frame
  alternates between `n` and `n±1` across blocks even when the host clock is
  perfect.
* The VST3 path (via clap-wrapper, [ADR 0014](0014-plugin-format-strategy.md))
  fills `song_pos_seconds` from the host's sample position **without** setting
  `CLAP_TRANSPORT_HAS_SECONDS_TIMELINE`
  (`extern/clap-wrapper/src/detail/vst3/process.cpp`), so the flag cannot be
  used to distinguish "real position" from "zero" there.

The engine follows transport with **zero tolerance**: any frame mismatch is
treated as a host relocation → `locateToFrame()` → unconditional `Relocation`
event + `resetOffsets()`, which drops the tick lookahead. With the ±1 frame
jitter this fired on roughly every other block: the lookahead was reset after
notes had already been enqueued, and the re-lookahead enqueued the same notes
again on top of the still-playing first copies — audible as doubled/chorused
hits whenever the plugin followed a rolling host.

## Decision Drivers

* Fix the double-trigger without touching the engine: the engine's
  zero-tolerance follow is correct for genuine relocations; the defect is the
  *input*, not the follower.
* Work identically for CLAP, VST3 (via clap-wrapper) and LV2, and stay
  testable without any plugin SDK (the SDKs are submodule-heavy and not
  linked into the unit-test binary).
* Keep genuine seeks, loop wraps, and stop/resume snaps working — those are
  real relocations and must still relocate.
* Do not regress realtime-safety: the fix lives on the audio thread
  (`process()`), and `lv2lint`'s syscall/memory gates instrument exactly that
  path.

## Considered Options

1. **Engine-side tolerance** — teach `followHostTransport` to ignore small
   mismatches. *Rejected:* the engine is shared by JACK/ALSA/software drivers
   whose clocks are exact; adding tolerance there weakens the contract for
   everyone and the correct tolerance is host-format-specific (a plugin-layer
   concern).
2. **Shim-side rounding tricks** — try to reconstruct the exact frame from
   sectime (e.g. track fractional residue). *Rejected:* the information is
   already lost in the fixed-point encoding; any reconstruction is guesswork
   that still drifts on long sessions.
3. **Tick-based plugin transport** — instead of forcing every driver to
   base its transport on frames, allow the plugin driver to be tick-based
   and drive the CLAP shim from `song_pos_beats`, which maps directly onto
   Hydrogen's tick grid. *Rejected:* Hydrogen does not support proper
   beat/bar/tick notation — the "columns" within a song do not translate to
   whole beats for all pattern lengths — so honouring a beat-based host
   position faithfully would require a larger rewrite of the audio engine,
   timeline, and song editor.
4. **Plugin-side latch** — a small SDK-free component between the shim and
   the engine that reconstructs the contiguous clock from the jittery
   readings. *(chosen)*

## Decision Outcome

### 1. `HostTransportLatch` (SDK-free, header-only)

`src/plugin/HostTransportLatch.h` owns the last *presented* reading and
reconciles each new host reading against what the clock should be:

* **Absorb:** if the host reading is within `knJitterToleranceFrames` (2) of
  the expected frame and the rolling state matches, present the expected
  frame (`latched + nFrames` while rolling, latched while parked) and keep
  the host's tempo. This is the jitter fix: the engine sees a perfect clock.
* **Adopt:** on first-ever transport, on a rolling-state change (host
  started/stopped), or on a mismatch beyond tolerance (seek, loop wrap,
  stop/resume at a different frame), take the host reading as the new truth.
* **Hold:** an invalid reading (host offered no usable position this block)
  keeps the latch — self-advancing while rolling, frozen while parked.
* **Absent transport:** if the host *never* offered transport, present
  invalid; the driver's transport stays invalid and the engine is left in
  its own state. The playhead stays parked — it advances only on a rolling
  host ([ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md)) — while
  realtime (MIDI/keyboard) notes keep triggering on
  `AudioEngine::m_nRealtimeFrame`, the engine-internal clock that
  `AudioEngine::getCurrentFrame()` substitutes for the playhead whenever
  the engine is not playing. That note path is independent of transport
  *and* of any audio driver, and predates this ADR; the latch neither
  enables nor touches it.
* **Clamp:** negative host frames present as 0 (hosts legitimately report
  pre-roll positions; the engine has no negative timeline).
* **Reset:** `activate()` resets the latch — a new sample rate rescales the
  timeline, so latched frame expectations from a previous activation are
  void.

Tempo gating: a non-positive BPM in an otherwise valid reading keeps the
latched tempo (or 0 if none) — hosts may omit tempo while providing position.

### 2. `HostTransportReading` as the shim boundary type

`{ bool bValid; bool bRolling; double fBpm; long long nFrame; }` —
`bValid` means "the shim has a usable position", not merely "transport
exists". `HydrogenPlugin::process()` takes it by const reference; each shim
builds it:

* **CLAP:** valid only with a usable seconds-timeline position. Under
  `H2_CLAP_FOR_VST3` (clap-wrapper fills `song_pos_seconds` unflagged) the
  position is always trusted; native CLAP requires
  `CLAP_TRANSPORT_HAS_SECONDS_TIMELINE`. Frames via
  `std::llround(seconds × sampleRate)`. The previous `steady_time` fallback
  is dropped — it is a wall-clock monotonic counter, never a song position.
* **LV2:** valid only when the host actually sent a `time:Position` this
  block (`Lv2TransportLatch::bHavePosition`, set in `read()`).

`process()` publishes to the driver **only when the presented reading is
valid**; an invalid presentation is not published at all.

### 3. Per-block RT logging is compile-time gated

The debugging `___DEBUGLOG` of host/presented transport in `process()` is
wrapped in `#if PLUGIN_TRANSPORT_DEBUG` (0 by default), mirroring the
engine's `AUDIO_ENGINE_DEBUG` (`AudioEngine.cpp`). Reason: the plugin's
debug-build logger bootstrap enables `Debug`, so a runtime-gated log here
takes the logger lock (futex) and drives log-file writes (newfstatat) from
the realtime thread every block — exactly what lv2lint's `Plugin Syscall`
gate rejects (verified: baseline passes, runtime-gated version fails 5/5,
compile-gated passes 5/5).

### Consequences

Positive:
* The double-trigger/chorus artifact when following a rolling host is gone;
  the engine sees the contiguous clock its contract requires.
* Genuine seeks, loop wraps and stop/resume still relocate (adopt path), and
  the tolerance is small enough (2 frames) that a real seek is never
  mistaken for jitter.
* The latch is SDK-free and unit-testable; `PluginTransportTest` (12 cases)
  pins the policy: absorb rolling/parked jitter, hold over absent transport,
  absent-transport presentation, clamp negatives, tempo gating, adopt-on-seek,
  and the end-to-end "jitter does not relocate the engine" regression.
* Under hosts that never publish transport the plugin no longer synthesizes
  a parked transport at frame 0 (the old shims called
  `setHostTransport(false, 0, 0)` every block, which `bValid = true`-pinned
  the engine to `Ready`). Playhead behaviour is unchanged — parked either
  way — and realtime notes triggered all along via `m_nRealtimeFrame`. The
  visible delta: transport actions that remain reachable in plugin mode
  (e.g. a MIDI-mapped Play action) are no longer vetoed block after block
  by that phantom parked transport; with no host transport published, such
  playback free-runs on the engine's own clock — exactly the engine's own
  semantics for an unpublished transport (`followHostTransport`: "free-run
  from our own state").

Negative / risks:
* Behaviour change under hosts that never publish transport (above): a
  MIDI-mapped Play action now starts free-running playback instead of being
  silently ignored. Realtime note triggering is unaffected — it never
  depended on transport.
* Between a genuine sub-tolerance host nudge and the latch there is no
  distinction by design: hosts that intentionally micro-nudge the playhead
  by ≤2 frames per block will be ignored. No known host does this.
* The VST3 path trusts `song_pos_seconds` unconditionally (wrapper
  limitation); a VST3 host that reports garbage seconds will be followed as
  a genuine position.

## More Information

* Builds on [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md)
  (host-driven transport), [ADR 0014](0014-plugin-format-strategy.md)
  (CLAP-first, VST3 via clap-wrapper), [ADR 0031]
  (0031-decouple-engine-clock-from-audio-driver.md) (the engine clock
  contract the latch restores).
* Engine side is deliberately untouched: `followHostTransport`,
  `locateToFrame`, `resetOffsets` keep their zero-tolerance semantics.
