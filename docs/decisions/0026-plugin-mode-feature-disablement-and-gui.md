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

## Amendment (2026-09-11): the editor process holds no control-surface clients

The disablement table above makes OSC and NSM *inert* in plugin mode: the
objects exist but never bind or talk. For the IPC editor split
(`--connect-via-ipc`, [ADR 0016](0016-out-of-process-plugin-ui.md) /
[0032](0032-h2player-gui-connection-mode.md)) that is not strong enough —
the mirror is a *separate process* whose OSC/NSM state would be a lie even
when inert. The editor process therefore constructs **neither** an
`OscServer` **nor** an `NsmClient` at all:

1. **Null members, not gutted ones.** `Hydrogen` leaves `m_pOscServer` /
   `m_pNsmClient` null in `ProcessMode::Editor` (in-class `= nullptr`
   initializers; the constructor skips both `new` calls). A present-but-
   empty client had already corrupted logic silently: the mirror's empty
   NSM session folder made `QString::contains("")` match *every* song path,
   so `Hydrogen::setSong()` pinned each replacement song to the previous
   song's location. Null members turn that class of bug into a loud
   null-guard.
2. **Sink gates as defense in depth.** `Hydrogen::recreateOscServer()`
   early-returns in Editor mode (joining the existing gate in
   `toggleOscServer()`); `setSong()` / `setSongModified()` consult the
   client only when it exists. The engine re-applies NSM session policy
   (path pinning, dirty-state reporting) when changes sync across the
   split.
3. **Caller gates in the GUI.** The preferences dialog's OSC apply/restore
   paths and `onRejected()`'s driver restarts skip their local
   `toggleOscServer` / `recreateOscServer` / `restartAudioDriver` /
   `restartMidiDriver` calls in Editor mode — the forwarded
   `CoreActionController::setPreferences` lets the headless engine apply
   them ([ADR 0029 amendment](0029-audio-driver-access-across-editor-split.md)).
   The OSC temporary-port notes (preferences dialog, port-busy message box)
   resolve through the engine-access query (point 5) in both modes.
4. **Already-clean sites stay clean.** All seven
   `CoreActionController::send*Feedback()` functions and the
   `isUnderSessionManagement()` / `isUnderPluginHost()` accessors were
   already Editor-gated or IPC-cached; the startup NSM handshake
   (`createInitialClient`) never ran in the editor branch.

Point 3's caller gates left two holes once the editor dialog became the
*only* user-facing OSC surface: the engine's fallback port was invisible in
the editor, and the dialog's restart call bypassed the engine. Both are
closed by this amendment as well:

5. **Query.** `IEngineAccess::getOscTemporaryPort()` (ADR 0029 query
   pattern) reports the engine's fallback port — the one a remote control
   surface must dial — to the preferences dialog and the port-busy message
   box in both modes (`GetOscTemporaryPort` reply; `-1` = no fallback in
   effect).
6. **Restart command.** `CoreActionController::recreateOscServer()` is the
   single write surface for the dialog's restart ([ADR 0027]); in the
   editor split the `IpcCoreActionController` override forwards it
   (`RecreateOscServer`, [ADR 0030]) so the *engine's* server restarts —
   the mirror's local call is a no-op. The dialog issues it after the
   preferences forward, so a recreated engine server binds the new port
   and not a stale copy. `onRejected()` additionally forwards the restored
   preferences in Editor mode, closing the cancel-path hole: the engine
   previously kept the canceled OSC settings until the next OK.
7. **Dirty-state command.** NSM judges "unsaved changes" by the dirty
   state the engine's `NsmClient` reports, but modifications happen in
   the editor. `IEngineAccess::setSongModified()` therefore forwards the
   flip (`SetSongModified`, [ADR 0030]) and applies it to the mirror so
   the GUI title updates immediately; the engine's
   `Hydrogen::setSongModified()` then reports to the session manager.
   The `SongIsModified` event stays engine-side — no echo is needed for
   editor-originated flips. The GUI call sites go through
   `IEngineAccess` in both modes. The `gui/main.cpp` bootstrap reset is
   skipped entirely in the split: the flag pulled with the song is
   authoritative — the engine's NSM open callback re-marks a new
   session's fresh song after `CoreActionController::setSong()` resets
   it — and a local reset would clobber the pulled flag.
8. **Session-folder query.** `IEngineAccess::getSessionFolderPath()`
   ([ADR 0029] query pattern, `GetSessionFolderPath` reply; empty = no
   NSM session in effect) serves the engine's NSM session folder to the
   editor's drumkit-export path. `DrumkitPropertiesDialog` replaced its
   local null-client guard with the query, so NSM-session export works
   identically in both modes and needs no OSC `#ifdef`.
9. **Error events: runtime forward + boot replay.** Runtime errors
   cross via the generic event forward (`isEngineOriginEvent` exempts
   only editor-internal types) and reach the editor's popup path. Two
   silent drops are closed: `EventQueue::pushEvent` no longer gates
   `Event::Type::Error` on `isFullyOperational()` — constructor-time
   failures (OSC port conflict at server start, driver start) must
   survive — and `EngineSession` retains the last 16 error codes while
   no editor is attached (instead of discarding them with the rest of
   the queue) and replays them on accept, ahead of the live queue, so
   the editor user still sees e.g. the boot-time OSC port-busy popup.
10. **Engine-origin dirty-state sync.** `CoreActionController::setSong()`
   preserves the incoming song's `isModified` flag instead of resetting
   it: a song pulled over IPC (`ipcSyncSong`) carries the engine's
   authoritative dirty state, and the installer no longer wipes it
   (making point 7's pulled-flag claim true by construction). Engine-
   origin flips (OSC/MIDI-reachable `CoreActionController` commands,
   the NSM open re-mark) push `SongIsModified`, which crosses tagged
   engine-origin; `handleRemoteEvent` answers it with the same full
   re-pull the remote `UpdateSong` handler performs — flag and content
   follow the authoritative engine. Editor-originated flips forward
   with `Event::Trigger::Suppress` (no echo: the editor already applied
   them locally, and an echo would trigger a redundant re-pull per
   edit). The re-pull clears the editor's undo stack — intended, since
   the engine-side edit is not part of the editor's undo history.
11. **Ad-hoc instrument preview.** The file browser, sound library, and
    sample editor audition instruments that are not part of the current
    song's kit — the number-based `PreviewInstrument` command
    ([ADR 0030] batch 2b) can not address them, and the sites' direct
    `Sampler::previewInstrument()` calls hit the mirror's engine, which
    can not render audio. `CoreActionController::previewInstrument(
    instrument, note)` closes the gap: in the editor split both objects
    cross as XML (`PreviewInstrumentSerialized`; instrument payload,
    note arg) and the engine reconstructs them — the note crosses
    without an instrument (an ad-hoc instrument resolves in no kit), so
    the bridge attaches it via `Note::mapToInstrument()` before the
    base implementation loads the samples and hands the pair to the
    sampler. Stand-alone instruments carry no kit context the engine
    could resolve a bare sample filename against, so
    `InstrumentLayer::saveTo()` writes the absolute sample path under
    `Xml::Flag::Ipc` (which `loadFrom()` accepts directly); on-disk
    formats are unaffected. The sample editor's edits survive the
    crossing: they are parametric (loop, velocity, and pan settings plus
    the `ismodified` flag) and ride the layer XML, so the engine
    re-bakes them on load — the preview is faithful. Residual limits: a
    rubberband-modified sample re-bakes at the engine's playhead bpm
    while the editor's in-memory copy was baked at the (frozen) mirror
    bpm — audible only for rubberband edits after a tempo change — and
    the browser/editor playhead animations read the mirror's frozen
    realtime frames (pre-existing).
