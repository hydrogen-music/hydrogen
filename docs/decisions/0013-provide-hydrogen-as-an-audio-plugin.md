---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Provide Hydrogen as a groovebox audio plugin synced to host transport

## Context and Problem Statement

Users repeatedly ask to run Hydrogen inside a DAW as a plugin instead of as a
standalone application bridged over JACK. A plugin is fundamentally different
from the standalone app: the host owns the audio buffers, the MIDI stream, and
— crucially — the transport (play/stop, tempo, song position). Hydrogen, by
contrast, is built as a *master*: it owns its own audio driver
(`src/core/IO/AudioDriver.h`), runs its own sequencer, and can even be the JACK
Timebase master (`src/core/IO/JackDriver.h`).

Two product identities are possible:

* **Mode A — drum *instrument*.** The host sends MIDI notes; Hydrogen only
  plays samples. The pattern sequencer and song mode are dropped.
* **Mode B — *groovebox*.** Hydrogen keeps patterns and song mode, but its
  internal playback follows the host transport instead of driving it.

We must decide which identity the plugin presents, because it determines how the
audio engine's control flow is restructured.

## Decision Drivers

* The pattern sequencer and song mode *are* what makes Hydrogen Hydrogen;
  dropping them yields a generic drum sampler that competes poorly.
* DAW users expect a plugin to start/stop and follow tempo with the host
  timeline, sample-accurately.
* The audio engine already separates a "queuing" lookahead transport from the
  "playhead" transport (`AudioEngine.h`), which makes a follower model feasible.
* Minimising surprise: when the host loops or relocates, the groovebox must
  relocate too.

## Considered Options

1. **Mode B — groovebox plugin following host transport.**
2. **Mode A — drum instrument only (sequencer dropped).**
3. Ship both as separate plugin variants.

## Decision Outcome

Chosen option: **Mode B — groovebox plugin following host transport.**

The plugin retains patterns, song mode, the sampler, drumkits, and per-instrument
processing. The transport relationship is **inverted**: each processing block the
plugin reads the host's play-state, tempo, and timeline position and drives
`AudioEngine`'s playhead/queuing transports from it, rather than calling
`incrementPlayhead()` and acting as Timebase master.

Concretely:

* The host-provided audio buffers and MIDI replace the audio/MIDI driver layer
  (see [ADR 0014](0014-plugin-format-strategy.md) and the implementation plan,
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)).
* `Tempo::Jack`/`Tempo::Song`/JACK Timebase master logic is replaced inside the
  plugin by a new "host transport" tempo source. The standalone app is
  unaffected.
* Internal audio export (`DiskWriterDriver`) and NSM/JACK-session are **not**
  exposed in the plugin — the host owns rendering and sessions.
* The LADSPA/LRDF effect host is **removed from Hydrogen entirely** in 2.0
  (deprecated in 1.2.7) — see [ADR 0024](0024-remove-ladspa-lrdf-effect-hosting.md).
  As a plugin, Hydrogen lives next to other plugins and the host owns the effects
  chain; this is a broader removal than just the plugin build.

Mode A is rejected as too generic. Shipping both variants is rejected as needless
maintenance surface; Mode B can always be *used* as an instrument by simply not
programming patterns.

### Consequences

* The audio-engine control flow around `audioEngine_process()`
  (`AudioEngine.cpp:1743`) gains a host-transport path. This is additive for the
  standalone build, which keeps its existing driver-driven path.
* Timeline tempo markers (`src/core/Timeline.h`) become advisory only inside the
  plugin: the host tempo wins. This is documented as a known limitation.
* Features that defer to the host (bounce/export, sessions, effect inserts) are
  intentionally absent from the plugin and must be communicated to users.

## More Information

* Implementation plan and feature matrix:
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
* Related: [ADR 0014](0014-plugin-format-strategy.md),
  [ADR 0015](0015-per-instance-engine-context.md),
  [ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0017](0017-embed-song-in-plugin-state.md)
