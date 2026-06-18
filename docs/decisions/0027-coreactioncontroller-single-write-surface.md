---
status: proposed
date: 2026-06-18
deciders: pm
---

# AD: CoreActionController is the single GUI→engine write surface

## Context and Problem Statement

The out-of-process editor ([ADR 0016](0016-out-of-process-plugin-ui.md)) assumes
the GUI reaches the engine through an injected handle, so the same `MainForm` can
be backed either by a local `Hydrogen` (standalone) or by an IPC proxy (editor
mode). For *reads* and for *commands routed through `CoreActionController`* (CAC)
this already holds: Phase 2 routed the GUI's state reads through `IEngineAccess`,
and a large set of edits flow through CAC.

A review of `src/gui/src/`, `src/core/Basics/` and `src/core/AudioEngine/`
(2026-06-18) found this assumption is **only half true**. The GUI also mutates
engine-owned `Basics` objects **directly**, bypassing CAC:

* ~110 sites call non-`const` setters straight on `Note`, `Pattern`,
  `PatternList`, `Instrument`, `InstrumentComponent`, `InstrumentLayer`, `Sample`,
  `Adsr`, `Song`, `Drumkit`, `AutomationPath` (e.g. `pNote->setVelocity(...)`,
  `pInstrument->setGain(...)`, `pComponent->setIsMuted(...)`,
  `pPattern->insertNote(...)`).
* ~16 of those sites take the **`AudioEngine` lock by hand**
  (`pAudioEngine->lock(RIGHT_HERE)` … `unlock()`) around the edit — the GUI is
  doing real-time safety itself.
* A subset of undo/redo commands (`UndoActions.h`) do **not** call CAC; they call
  back into GUI widget methods that interleave the core mutation with a view
  refresh (e.g. `PatternEditor::editNotePropertiesAction`,
  `SoundLibraryPanel::switchDrumkit`).

In editor mode the engine-owned graph lives in **another process**. A direct
`pNote->setVelocity()` mutates the editor's local mirror, not the engine — so
every one of those ~110 sites is exactly as much a cross-process problem as a CAC
command. Routing CAC over IPC while leaving direct mutations in place would
produce a GUI that *looks* converted but silently drops most editing across the
split. We must decide how the GUI is allowed to change engine state at all,
before building the IPC command path.

## Decision Drivers

* Editor mode needs a **single, complete, serialisable** GUI→engine write surface
  ([ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0018](0018-plugin-editor-ipc-transport.md)).
* Real-time safety: most `Note`/`Pattern`/`Sample`/`Adsr`/`AutomationPath`/
  `Instrument` edits touch objects the `Sampler`/`AudioEngine` reference live, so
  they need the engine lock. That discipline should live in one place, not be
  re-implemented at each call site.
* Don't make a hot core class (`CoreActionController`) polymorphic if it can be
  avoided; keep the standalone path byte-for-byte unchanged during the sweep.
* The change should be independently valuable (clean core/GUI boundary, full
  undo coverage) even apart from the plugin.

## Considered Options

1. **CAC as the single write surface** — forbid direct GUI mutation of
   engine-owned `Basics`; every such edit goes through a CAC entry point; editor
   mode then routes that one surface over IPC.
2. **Leave direct mutations; add an IPC-backed `CoreActionController`** (mark its
   ~80 methods virtual + a subclass) and separately intercept direct `Basics`
   edits — two seams, and the direct edits remain unencapsulated.
3. **Intercept at the undo-command boundary** — serialise each `QUndoCommand`'s
   effect for IPC. Rejected: the survey shows many undo commands call GUI widget
   methods that *interleave* mutation with view-refresh, so the boundary is not a
   clean edit description.

## Decision Outcome

Chosen option: **CoreActionController is the single GUI→engine write surface.**

The invariant: **GUI code MUST NOT call a non-`const`/mutating method on an
engine-owned `Basics`, `AudioEngine` or `Transport` object directly. All such
changes go through a `CoreActionController` entry point.** Once this holds, editor
mode is "route CAC over IPC" and the direct-mutation problem disappears by
construction; the choice of *how* to carry CAC across the wire (command-sink vs
virtualisation) becomes a minor, well-informed implementation detail decided
afterwards.

Not every GUI mutation belongs in CAC. Each writable member is assigned to **one
of four destinations** ("the four-bucket taxonomy"); only bucket B is the CAC
sweep:

* **A — already CAC-routed.** ~25 undo commands already call only CAC
  (`setPattern`, `setSongProperties`, `addTempoMarker`, `moveInstrument`,
  `replaceInstrument`, `addAutomationPoint`, playlist ops, …). These are the
  template; no change.
* **B — engine song/drumkit graph, currently mutated directly → new CAC entry
  points.** ~110 sites: note-property edits, pattern size, the full `Instrument`
  parameter set, `InstrumentComponent`/`InstrumentLayer` edits, drumkit
  properties, a few `Song`/sequencer/pattern-selection ops. This is the sweep
  (~30–40 new CAC methods).
* **C — config / preferences → layered config, NOT CAC.** ~12 sites:
  `Sampler::setInterpolateMode` (Export + Preferences dialogs), metronome volume,
  MIDI action-table edits. These belong to the layered-config mechanism
  ([ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0023](0023-concurrency-safe-config-persistence.md)).
* **D — display / editor-local → never crosses IPC.** ~12 sites:
  `AudioEngine::setMasterPeak_L/R`, `Instrument::setPeak_L/R` (read from the
  telemetry block instead — [ADR 0018](0018-plugin-editor-ipc-transport.md)),
  `setCurrentlyExported`, `GridPoint`, selection and `SampleEditor` staging
  buffers. These stay local and must be explicitly marked so the sweep does not
  over-route them.

The exhaustive per-entry catalogue (every writable member × current GUI sites ×
target bucket × new-or-existing CAC method × lock-sensitivity × undoable-today)
is maintained in
[proposal 0005](/docs/proposals/0005-gui-engine-write-surface-catalogue.md).

### Three invariants every bucket-B entry point must honour

1. **CAC owns the AudioEngine lock.** The ~16 hand-rolled
   `lock()/…/unlock()` blocks in the GUI are the evidence that these edits are
   real-time-sensitive. That responsibility moves *into* the CAC method; GUI
   call sites never lock.

2. **CAC entry points are granular parameter setters, not whole-object
   replacements.** Each entry point takes the minimal delta — e.g.
   `setLayerGain(nInstrument, nComponent, nLayer, fGain)`,
   `setInstrumentFilterCutoff(nInstrument, f)`, `setDrumkitAuthor(QString)` — not
   a serialized `Instrument`/`Drumkit`. This is what makes the entry point a
   cheap IPC opcode (bytes, not the object); see invariant 3 and the cost note
   below.

3. **Whole-instrument/drumkit replacement is the engine-side *implementation* of
   those setters, never the IPC payload.** Because each `Note` holds a
   `shared_ptr<Instrument>` referenced by the live `Sampler` queue, a
   *structural* change (add/move/remove `InstrumentComponent` or
   `InstrumentLayer`, replace a `Sample`) must swap the instrument atomically —
   the established pattern (`SE_replaceInstrumentAction`, documented at
   `UndoActions.h:1055`). The engine performs that clone-and-replace **locally**
   from the delta it received, reusing its own already-loaded `Sample` objects.
   The wire never carries the object. For purely *scalar* edits (gain, mute,
   filter, ADSR, pan) the engine may skip replacement entirely and do a
   lock-guarded in-place set. Undo state stays editor-side (the editor keeps its
   old/new copies for its local mirror); undo/redo just re-send the delta.

   *Addressing:* editor-facing entry points name engine objects by **stable
   value identity, never by pointer** — by the `Object`-level instance id
   ([ADR 0028](0028-object-instance-identity.md)) and/or by kit slot/index
   (patterns, components, layers, playlist entries). A review found the handlers
   currently compare by raw pointer (`InstrumentList::index`,
   `removeInstrumentFromDeathRow`, `clearNoteQueues`,
   `Sampler::releasePlayingNotes`, `Playlist::remove`, `Pattern::removeNote`),
   which is why Phase 4.4 (ADR 0028) re-bases all instance identity on the
   `Object` id — one uniform rule — before this sweep. The engine's internal
   identity machinery (death row, note-queue, sampler matching) keeps working
   because the engine resolves the value identity to *its own* objects.

#### Why invariants 2–3 matter (cost across the split)

The current undo design clones the whole object and keeps the old copy on the
undo stack. This was assumed cheap, but `Sample`'s copy constructor
`memcpy`s the full decoded PCM (`InstrumentLayer` deep-copies its `Sample`), so
every `SE_replaceInstrumentAction` already duplicates all of the instrument's
audio (sub-ms `memcpy`; modest retained RAM — fine in-process). Across the
editor/host split this stops being free **if the serialized object becomes the
IPC payload**:

* whole object as XML (samples by path) → engine **re-decodes samples from disk**
  per edit: ~5–30 ms for an instrument, **hundreds of ms to seconds for a whole
  drumkit** (e.g. editing a kit's author name), and it silently drops in-memory-
  only sample edits;
* whole object embedded (PCM in payload) → **MB to tens of MB across the socket
  per keystroke**.

A granular delta (invariants 2–3) keeps the per-edit cost at the in-process
baseline or below (bytes on the wire; the engine clones locally reusing loaded
samples, or sets in place for scalars). Whole-object payloads remain reserved for
genuine bulk loads (`SetSong`, `SetDrumkit`, drop-in instrument from another kit),
where the samples must be resolved anyway.

### Consequences

* **A dedicated phase precedes editor mode.** The sweep is larger than the
  remaining editor-mode tasks; it is sequenced *before* T5.2-cont/T5.3/T5.4 in
  [proposal 0004](/docs/proposals/0004-plugin-port-implementation-plan.md).
* **Per class, the work is:** add the CAC entry point(s) (owning the lock /
  replacing the instrument as needed); redirect the GUI mutation and any undo
  command to call *only* CAC; move the view-refresh out of the mutation path into
  an `EventQueue`/`EventListener` reaction; test; keep the suite green.
* **Undo coverage improves as a side effect.** The ~17 `InstrumentEditor` edits
  and the ~10 `ComponentView`/`LayerPreview` edits are **not undoable today**;
  routing them through CAC + an undo command fixes that.
* **CAC grows** from ~70 to ~100–110 methods. This is not bloat — it is the
  honest size of the GUI→engine write surface, now made explicit and testable.
* **Display-state-in-model is cleaned up.** Peak meters written by the GUI into
  core objects become telemetry reads, which editor mode required anyway.
* **Done-when:** zero direct mutations of engine-owned `Basics`/`AudioEngine`/
  `Transport` remain in `src/gui/src` — enforceable by a grep guard in CI. Editor
  mode then resumes as pure CAC-over-IPC.
* The standalone application is behaviourally unchanged throughout: a local CAC
  call does exactly what the former direct mutation did (plus, where it was
  missing, the lock and an undo step).

## More Information

* Exhaustive catalogue and phased sweep:
  [proposal 0005](/docs/proposals/0005-gui-engine-write-surface-catalogue.md)
* Editor architecture and the injected-handle premise:
  [ADR 0016](0016-out-of-process-plugin-ui.md)
* IPC command/event transport and telemetry:
  [ADR 0018](0018-plugin-editor-ipc-transport.md)
* Config buckets: [ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0023](0023-concurrency-safe-config-persistence.md)
* Implementation plan: [proposal 0004](/docs/proposals/0004-plugin-port-implementation-plan.md)
