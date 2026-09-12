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
12. **Engine-access commands without a CoreActionController surface.**
    `handleBeatCounter()`, `onTapTempoAccelEvent()`,
    `updateBeatCounterSettings()`, `setIsTimelineActivated()`,
    `setPatternMode()`, and `loadPlaybackTrack()` are Hydrogen-level
    commands the editor issues via `IEngineAccess`; until now their
    `IpcEngineAccess` implementations applied them to the mirror only.
    Taps are engine-authoritative — the mirror's handlers are designed
    no-ops in editor mode (`getTempoSource() == Tempo::Remote`) — so
    they cross as fire-and-forget commands carrying the absolute tap
    timestamp as a nanosecond epoch count (engine and editor share the
    host clock, so the `TimePoint` reconstructs losslessly). A
    default-constructed `TimePoint` is stamped at call time on the
    editor side — the engine must not stamp at bridge-thread processing
    time, or a tap queued behind a TapAndPlay lead-in sleep would
    inherit that delay as interval jitter.
    Beat-counter *config* is editor-owned: the BpmTap buttons write the
    mirror's Hydrogen members and the mode actions and preferences
    dialog write the mirror's preferences — none of which sync to the
    engine on their own, and the engine's TapAndPlay completion branch
    reads the mode from its (stale) preferences copy. So
    `updateBeatCounterSettings()` crosses as a *config snapshot* of the
    editor's current state (beat length, total beats, drift
    compensation, start offset, and Tap/TapAndPlay mode) instead of
    letting the engine re-read its own preferences. The engine's
    `BeatCounter` pushes carry their event count as the event value;
    `EditorStateMirror::applyEvent()` applies it to the mirror's count
    member, keeping the BpmTap "n/total" display in step without a
    blocking query (listeners never read the value, so stand-alone
    behavior is unchanged). Timeline activation, pattern mode, and the
    playback track are dual-apply: forwarded for the engine's
    authoritative copy, applied locally for immediate GUI reflection.
     The engine-side `setPatternMode()` originally flipped the dirty
     flag with the `Default` trigger so its echo re-pulled the song;
     point 13 moves it — like all forwarded commands — to `Suppress`;
     the *explicit* `setSongModified` command keeps `Suppress`
     throughout (bare flag flips are editor-known). Known residual:
     the TapAndPlay completion branch sleeps one beat in the bridge
     thread before starting playback — pre-existing semantics, kept
     synchronous so FIFO ordering against a subsequent Stop is
     preserved.
13. **Dirty-flip trigger split for forwarded commands.** Point 10 made
     engine-origin `SongIsModified` echoes the editor's re-pull signal,
     and the explicit `SetSongModified` opcode already applied with
     `Suppress`. But every *forwarded edit command* — the
     `IpcCoreActionController` overrides that marshal a command to the
     engine and also apply it on the mirror — flipped the engine's
     dirty flag with the `Default` trigger on the engine-side apply.
     Each such edit therefore echoed `SongIsModified` back to the
     editor, whose `handleRemoteEvent` re-pulled the full song:
     redundant (the mirror had already applied the identical edit) and
     damaging (the re-pull runs the mirror's `setSong`, which resets
     the selected pattern to 0, clears the undo stack, and churns the
     recent-files list — per edit). The fix is a trigger split along
     the dual-apply seam: the mirror-local apply keeps `Default` (an
     Editor-origin event, filtered by the origin gate, updating the
     local GUI), while the bridge applies forwarded commands on the
     engine with `Suppress` (flag flips, the engine's NsmClient
     reports to the session manager, no echo). `Event::Trigger` is
     threaded through every `CoreActionController` method that reaches
     a `setSongModified()`/`setDrumkitModified()`/`setPatternModified()`
     flip, through `Hydrogen::setPatternMode()`, and through the
     drumkit/pattern/pattern-editor-lock wrappers, so engine-local
     callers (OSC/MIDI on the engine, NSM open re-marks) keep the
     `Default` argument and remain the editor's only echo source. Two
     accompanying semantics: (a) on the headless engine
     `setSongModified()` fires even when the flag is unchanged (unless
     `Suppress`) — the transition-only early-return swallowed every
     engine-local edit after the first while dirty, and the editor
     would never have re-pulled them; stand-alone GUI and the editor
     mirror keep transition-only behavior (the local UI already
     knows), and the CLI player drains the extra events in its main
     loop; (b) the `SongIsModified` re-pull branch in
     `HydrogenApp::handleRemoteEvent` re-syncs the selected pattern
     and instrument after `ipcSyncSong()` — the mirror's `setSong`
     resets the pattern selection with new-song semantics, and the
     surviving engine-local echoes must not cost the editor its
     editing context (the undo-stack clear that used to ride along was
     later reserved for new documents — point 15). The former residual —
     mirror-only flips never reaching the engine, so NSM under-reported
     editor-side drumkit/pattern edits — is resolved by point 14's
     Class C routing.
14. **Class C routing, echo hygiene, and re-pull debouncing.** Closes
      the follow-ups from point 13's review. (a) *Class C routing*: the
      drumkit/pattern modified flags and the pattern-editor lock are
      song state the GUI writes through `IEngineAccess`; three opcodes
      (`SetDrumkitModified`, `SetPatternModified`,
      `SetIsPatternEditorLocked`, appended at the enum tail for wire
      compatibility) cross them. The bridge applies them on the engine
      `Hydrogen`-direct with `Suppress` (no echo — the editor initiated
      and already applied the flip), `IpcEngineAccess` dual-applies
      (forward + mirror), and `setIsPatternEditorLocked()` joins the
      `IEngineAccess` command surface (`LocalEngineAccess` passes
      through). The remaining GUI bypasses — `Modifier::modify()`,
      `MainForm::onFixMidiSetup()`, and the SongEditorPanel lock
      button — migrated from `HydrogenApp::pHydrogen()` to `pEngine()`.
      (b) *Wrapper guards*: `Hydrogen::setDrumkitModified()` and
      `setPatternModified()` forward every dirty flip to
      `setSongModified()` — not just clean→dirty — so engine-local
      instrument/note edits while already dirty still echo (the
      editor's re-pull signal); stand-alone and the editor mirror are
      unchanged (`setSongModified()`'s transition-only early-return
      absorbs the redundant calls). On the headless engine under NSM,
      unchanged flips also re-send the dirty state to the session
      manager — idempotent (a boolean flag server-side) and one small
      OSC send per edit, the same order as the edit traffic itself.
      (c) *Force*: `setSongModified()`
      honors `Event::Trigger::Force` in every process mode — bypassing
      the transition-only early-return — matching the `Event.h`
      contract ("queued regardless whether there are changes or not");
      no caller uses it yet. (d) *Debounced re-pull*: engine-origin
      `SongIsModified` echoes no longer re-pull inline per echo — an
      OSC fader sweep would re-pull (and clear the undo stack) per
      change. `HydrogenApp::scheduleSongModifiedResync()` pulls
      immediately on the first echo of a burst (single edits keep zero
      latency), restarts a 200 ms debounce on successive echoes, and
      forces at least one pull per 1000 ms during continuous streams. A
      full `UpdateSong` sync cancels a pending resync (it supersedes);
      IPC connection loss drops it. (e) *Error re-anchor (deferred)*:
      re-pulling on engine-origin `Error` events was considered and
      dropped: no dispatched command emits `Error` events today —
      command failures are `ERRORLOG`-only (`EngineSession` discards
      the `dispatchCommand` result) — while the actual `Error` sources
      are infrastructure (JACK port/activate/shutdown, driver start,
      invalid playback track, OSC port). A re-pull on those would clear
      the editor's undo stack and block the GUI on request timeouts
      precisely when the engine is distressed — net-negative versus
      the popup-only baseline. The hook lands together with the
      upstream fix: propagate command-failure results in
      `EngineSession` and push an `Error` event on failure; then the
      debounced re-anchor becomes correct (the mirror may have applied
      an edit the engine refused). Known residuals: engine-side
      failures that only `ERRORLOG` (no `Error` event) still pass
      unnoticed;
      `saveSong`/`saveSongAs` stay forward-only by design (post-save
      state sync rides the `UpdateSong(1)` echo); and the GUI-side
      debounce is not covered by the core test harness (`HydrogenApp`
      is GUI-only) — it is review-covered instead.
 15. **Undo resets only on new documents.** Supersedes the undo-clear
      behavior conceded in point 13 and motivating point 14(d): the
      undo stack is reset exclusively when a new song is loaded or set
      — never when the same document re-syncs.
      `HydrogenApp::updateSongEvent(0)` keeps the reset only for
      standalone (and a detached editor on local fallback, where every
      `UpdateSong(0)` is a load/set); while attached via IPC the reset
      moves to the two new-document origins: the remote
      `UpdateSong(0)` (the engine loaded a song) in
      `handleRemoteEvent()` and the attach in `syncViaIpc()` (on
      re-attach the engine's song may have changed while detached —
      the editor cannot know; a no-op on the first attach, where the
      stack is still empty). Dirty-echo re-pulls and post-save syncs
      keep the history. Relatedly, `CAC::saveSong()` with discarded
      missing samples now pushes `DrumkitLoaded` alongside the regular
      `UpdateSong(1)` — and never `UpdateSong(0)`: the discard is
      confined to the instruments of the current drumkit (removed from
      the in-memory song by `InstrumentComponent::saveTo`), so the
      instrument-facing widgets refresh while the undo history
      survives a same-document save. In editor mode the save is
      engine-only (`IpcCoreActionController::saveSong` forwards
      without a mirror write), so the mirror relies on the
      `UpdateSong(1)` echo: `handleRemoteEvent` re-pulls the song
      (undo preserved — the pull's local `UpdateSong(0)` is guarded
      above), and the queued `DrumkitLoaded` is consumed after that
      pull, so the widgets re-read an already-discarded mirror. The
      saveSong event change is harness-covered
      (`CoreActionControllerTest::testSaveSongDiscardEvent`); the
      HydrogenApp routing is review-covered (GUI-only). Residuals: the
      read-only advisory (`UpdateSong(2)`) still rides the setSong
      install path, so re-syncs of a read-only song re-show the popup;
      and engine-origin `DrumkitLoaded` from engine-side drumkit loads
      (OSC) has no `handleRemoteEvent` case — the mirror is not
      re-synced for those (pre-existing).
