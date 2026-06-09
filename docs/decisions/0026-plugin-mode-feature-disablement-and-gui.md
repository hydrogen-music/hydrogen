---
status: accepted
date: 2026-06-09
deciders: pm
---

# AD: Plugin-mode feature disablement (core) and GUI adaptation

## Context and Problem Statement

As a Mode B groovebox plugin ([ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md))
Hydrogen runs under the host's transport, audio, MIDI, and session management.
Several core features then **conflict** with the host (they try to own something
the host owns), and several GUI affordances become **meaningless or misleading**.
[ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md) covered the obvious
driver/transport/session cases; this ADR is the **definitive enumeration** of (A)
which core features are disabled or bypassed in plugin mode — done in the **early**
porting phases — and (B) the GUI changes required for a clean integration — a
**late-stage** task.

## Decision Drivers

* No core feature may fight the host for tempo, transport, sessions, or I/O.
* The editor must not present controls that do nothing or contradict the host.
* Consistency: when in doubt, disable rather than half-support.
* The editor is the *same* `hydrogen` GUI in editor mode
  ([ADR 0016](0016-out-of-process-plugin-ui.md)), so adaptation = conditional
  hiding/disabling, not a separate UI.

## Decision Outcome

### A. Core features disabled / bypassed in plugin mode (early phases)

These extend the driver/transport/session items already in
[ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md) (hardware audio/MIDI
drivers, JACK transport + Timebase master, `DiskWriter` export, `Tempo::*`
sources). The LADSPA/LRDF effect host is **removed entirely**, not merely
disabled ([ADR 0024](0024-remove-ladspa-lrdf-effect-hosting.md)).

| Core feature | Action in plugin mode | Rationale |
|---|---|---|
| **OSC server** (`OscServer`) | **Disabled** | Per-instance network port can't be bound by N instances; the host controls the plugin. |
| **NSM support** (`NsmClient`) | **Disabled** | The host owns session management. |
| **Timeline** (`Timeline`, tempo markers) | **Disabled** | The host owns tempo and the timeline; markers can't drive tempo here. |
| **Playlist MIDI actions & commands** (`MidiActionManager::playlistNextSong`/`playlistPreviousSong`/`playlistSong`, `PlaylistNextSong`/`PlaylistPrevSong`/`PlaylistSong`) | **Disabled** | No playlist in plugin mode — the host arranges songs. |
| **Tap Tempo** | **Disabled** | The host owns tempo. |
| **Beat Counter** | **Disabled** | The host owns tempo. |
| **Loop state** | **Forced ON** (not user-togglable) | Plugin transport is continuously looped under the host; variable loop is meaningless. |
| **MIDI clock** (in *and* out; `Tempo::Midi`, MIDI-clock stream) | **Disabled** | The host owns transport/clock; emitting or following MIDI clock would conflict. |

Bypassed (handled by the host, per [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md)):
hardware audio/MIDI drivers, JACK transport / Timebase, audio export, all
non-host tempo sources.

### B. GUI adaptation in editor mode (late stage)

| GUI element                                | Change in editor mode                                                                                                                                                                                             |
|--------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **PreferencesDialog**                      | Hide conflicting options (audio/MIDI driver, JACK, OSC, NSM, MIDI clock); show host-provided audio settings (sample rate, buffer size) **read-only** (cf. [ADR 0022](0022-layered-plugin-configuration.md)).      |
| **PlaylistEditor**                         | **Removed/hidden** — no playlist in plugin mode.                                                                                                                                                                  |
| **Timeline / Tempo Markers** (song editor) | **Disabled.** Timeline tags *could* be useful, but are likely covered by the DAW and are **disabled too for consistency.**                                                                                        |
| **Main-menu transport actions**            | Hide most transport actions; **keep play/pause and loop** as **read-only state indicators** — they reflect the host transport state, with loop shown permanently on to emphasise that transport is always looped. |
| **Main-toolbar widgets**                   | Hide the **BPM**, **tap-tempo**, and **beat-counter** widgets.                                                                                                                                                    |
| **Song-editor & pattern-editor rulers**    | **No relocation** by clicking the ruler (the host owns playhead position); also **drop the mouse-hover affordance** that advertises ruler relocation.                                                             |
| **MidiActionTable**                        | **Filter** `MidiAction`s discarded by `MidiActionManager`                                                                                                                                                         |

Notable non-obvious choices, recorded explicitly:

* **Loop is always on** (core forced; GUI loop button is an always-on indicator).
* **Timeline tags are disabled for consistency** despite being arguably useful,
  because the DAW already provides arrangement markers.
* **Play/pause and loop remain visible** — not as controls Hydrogen owns but as
  indicators of the host transport state.

### Phasing

* **Core disablement (A)** lands in the **early** porting phases (the host-seam
  work), so the engine never fights the host even before the GUI is adapted.
* **GUI adaptation (B)** is a **late-stage** task after editor mode exists
  ([ADR 0016](0016-out-of-process-plugin-ui.md)). Until then a stale control is
  harmless (the underlying core feature is already disabled by A) but confusing —
  which is why B is required for a *proper* integration, not optional polish.

### Consequences

* A single, queryable "running as plugin" predicate gates both the core disables
  (A) and the GUI conditionals (B); it must be available to core and to the GUI
  (via the engine-access handle, [ADR 0015](0015-per-instance-engine-context.md)).
* The GUI changes are conditional hides/disables on the *same* widgets the
  standalone app uses — no forked UI ([ADR 0016](0016-out-of-process-plugin-ui.md)).
* Disabled MIDI actions (playlist, tap tempo, beat counter, MIDI clock) must be
  filtered from `MidiActionManager` mapping/handling in plugin mode, not just
  hidden in the GUI.
* Forcing loop on is a transport-follower behaviour ([ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md));
  the song/pattern simply wraps under the host position.

## More Information

* `src/core/OscServer.*`, `src/core/NsmClient.*`, `src/core/Timeline.h`,
  `src/core/Midi/MidiActionManager.*` (playlist actions), beat-counter in
  `src/core/Hydrogen.*`, MIDI clock in `CoreActionController`/`AudioEngine`/
  `MidiInput`; `src/gui/src/PlaylistEditor/`, main toolbar/menu, song & pattern
  editor rulers.
* Related: [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md),
  [ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0024](0024-remove-ladspa-lrdf-effect-hosting.md),
  [proposal 0004](/docs/proposals/0004-plugin-port-implementation-plan.md)
