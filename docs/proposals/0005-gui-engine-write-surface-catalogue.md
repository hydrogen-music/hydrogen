# Proposal 0005 — GUI→engine write-surface catalogue & sweep

**Status:** in progress (2026-06-18) — **re-audited 2026-06-23** against the
working tree. Companion to
[ADR 0027](/docs/decisions/0027-coreactioncontroller-single-write-surface.md).
Supports the phase inserted into
[proposal 0004 §8.5](/docs/proposals/0004-plugin-port-implementation-plan.md).

**Audit summary (2026-06-23).** Buckets A/B-Basics/C/D are landed and the CI guard
(`tools/check_write_surface`) is **green**, enforcing zero direct GUI mutation of
engine-owned `Basics`. Done: §2.1 Note/Pattern, §2.2 Instrument+ADSR, §2.3
Component/Layer, §2.4 Sample, §2.5 Drumkit properties, §2.7 song-grid, plus §2.6
pan-law and stop-notes; bucket C (config, incl. the audio/MIDI **driver** sweep of
[ADR 0029](/docs/decisions/0029-audio-driver-access-across-editor-split.md)) and
bucket D (peaks → telemetry). **Still open** (not yet covered, guard does not reach
them): the §2.6 sequencer / pattern-list verbs (`sequencerPlay`,
`setSelectedPatternNumber`→`selectPattern`, `toggleNextPattern`, `movePattern` /
`PatternList::replace`), the remaining hand-rolled lock sites in §5
(SampleEditor / SongEditorPositionRuler / SongEditorPatternList / ExportSongDialog),
and extending the guard beyond `Basics` to `AudioEngine`/`Sampler`/`Transport`.
Per-section status is marked inline below.

This document is the exhaustive map produced by reviewing `src/gui/src/`,
`src/core/Basics/` and `src/core/AudioEngine/`. It records, for every writable
piece of host-owned state, **where the GUI changes it today** and **where that
change must go** under the single-write-surface rule (ADR 0027). It is the work
list for the sweep and the checklist for "done."

The four destinations (ADR 0027):

* **A** — already routed through `CoreActionController` (CAC); no change.
* **B** — engine song/drumkit graph mutated directly → **new CAC entry point**.
* **C** — config/preferences → **layered config** (ADR 0022/0023), not CAC.
* **D** — display / editor-local → **never crosses IPC** (telemetry read or stays
  local).

Legend for the tables: **RT** = realtime-audio-sensitive (the CAC entry point
must hold the `AudioEngine` lock). **Undo?** = wrapped in a `QUndoCommand` today.
**CAC** = does an entry point already exist.

---

## 0. Totals

| | Count |
|---|---|
| Existing CAC entry points | ~70 |
| Bucket A — already CAC (undo commands) | ~25 |
| Bucket B — direct mutation sites → new CAC | ~110 |
| New CAC entry points to add (bucket B) | ~30–40 |
| Bucket C — reroute to config layer | ~12 |
| Bucket D — display/editor-local | ~12 |
| Hand-rolled `AudioEngine::lock()/unlock()` blocks to absorb | 16 |

---

## 1. Bucket A — already CAC-routed (template, no change)

Undo commands in `UndoActions.h` that already call only CAC — these define the
target pattern:

`setPattern` / `removePattern` / `setPatternProperties` / `setSongProperties`,
`addTempoMarker` / `deleteTempoMarker`, `addTag` / `deleteTag`, `setBpm`,
`moveInstrument`, `addInstrument` / `removeInstrument`, `replaceInstrument`,
`renameComponent`, `addToPlaylist` / `removeFromPlaylist` / `setPlaylist`,
`addAutomationPoint` / `removeAutomationPoint`.

---

## 2. Bucket B — the sweep (new CAC entry points)

> **Granular-delta rule (ADR 0027 invariants 2–3).** Every entry point below
> takes the **minimal parameter delta** (e.g. `setLayerGain(nInstr, nComp,
> nLayer, f)`), never a serialized `Instrument`/`Drumkit`. Where a column says
> "via replace", that means the **engine** clones-and-replaces the instrument
> *locally* (queue safety; reuses its own loaded samples) — the wire still
> carries only the delta. Scalar edits (gain/mute/filter/ADSR/pan) may skip
> replacement and do a lock-guarded in-place set. This matters because the
> in-process clone already `memcpy`s all sample PCM (`Sample`'s copy ctor); if a
> whole object were put on the wire instead, a single-parameter edit would cost
> MB of transfer or a full engine-side disk re-decode (hundreds of ms–seconds
> for a drumkit-property edit). Whole-object payloads are reserved for genuine
> bulk loads (`SetSong`, `SetDrumkit`, drop-in instrument).

### 2.1 Note / Pattern (PatternEditor) — ✅ DONE

CAC entry points `editNoteProperty`, `addOrRemoveNote`, `setPatternSize` landed;
the GUI routes through them (guard-clean). The hand-rolled locks in
`PatternEditorPanel`/`PatternEditorRuler` were absorbed into CAC.

| Core mutator | GUI sites (file:line) | RT | Undo? | CAC | New entry point |
|---|---|---|---|---|---|
| `Note::set{Velocity,Pan,LeadLag,Key,Octave,Probability,Length}` | PatternEditor.cpp 446–506; NotePropertiesRuler.cpp 261–300, 1068–1125; mouseEditUpdate 2346–2389 | yes | yes (via `SE_editNotePropertiesAction` → GUI static) | no | `editNoteProperty(...)` (untangle from `PatternEditor::editNotePropertiesAction`) |
| `Note::set{InstrumentId,Type}` | PatternEditor.cpp 465–485 | yes | yes | no | covered by the same note-edit entry |
| `Pattern::insertNote` / `removeNote` | PatternEditor.cpp 194, 261, 322, 355 | yes | yes (`SE_addOrRemoveNoteAction`/`SE_deselectAndOverwriteNotesAction` → GUI static) | partial (`handleNote`, `toggleGridCell`) | `addOrRemoveNote(...)` covering selection overwrite |
| `Pattern::set{Length,Denominator}` | PatternEditorPanel.cpp 1726–1727 | yes | yes (`SE_patternSizeChangedAction` → GUI method) | no | `setPatternSize(nLength, fDenominator, nPattern)` |

Notes: all four rows currently self-lock (`PatternEditorPanel.cpp:1506/1528`,
`1724/1729`; `PatternEditorRuler.cpp:117/127`). The CAC entries absorb the lock.

### 2.2 Instrument parameters (Rack/InstrumentEditor) — ✅ DONE

CAC gained `setInstrumentGain/Pitch/RandomPitch/FilterActive/FilterCutoff/
FilterResonance/MuteGroup/StopNotes/ApplyVelocity/HihatGroup/LowerCc/HigherCc`
and `setInstrumentAttack/Decay/Sustain/Release` (ADSR, scalar in-place under
lock); volume/mute/solo route through the pre-existing `setStripVolume/
setStripIsMuted/setStripIsSoloed` (bucket A). Guard-clean; undo coverage added.

| Core mutator | GUI sites | RT | Undo? | CAC | New entry point |
|---|---|---|---|---|---|
| `Instrument::set{Gain,Volume,Muted}` | InstrumentEditor.cpp 348; SongEditorPanel.cpp 274, 1189 | yes | no | no | `setInstrumentGain/Volume/Muted(nInstr, …)` |
| `Instrument::set{FilterActive,FilterCutoff,FilterResonance}` | InstrumentEditor.cpp 244, 258, 269 | yes | no | no | `setInstrumentFilter*(…)` |
| `Instrument::set{PitchOffset,RandomPitchFactor}` | InstrumentEditor.cpp 196, 214, 227 | yes | no | partial (`setInstrumentPitch`) | extend pitch entry |
| `Instrument::set{MuteGroup,StopNotes,ApplyVelocity,HihatGrp,LowerCc,HigherCc}` | InstrumentEditor.cpp 363–450 | mixed | no | no | `setInstrument*(…)` group |
| `Adsr::set{Attack,Decay,Sustain,Release}` | InstrumentEditor.cpp 286–327 | yes | no | no | `setInstrumentAdsr(nInstr, …)` (scalar → in-place set under lock) |

### 2.3 Component / Layer (Rack/ComponentView, LayerPreview) — ✅ DONE

CAC gained `setComponentIsMuted/IsSoloed/Gain/Selection` and `setLayerIsMuted/
IsSoloed/Gain/PitchOffset/StartVelocity/EndVelocity` (scalar in-place under lock).
Guard-clean; undo coverage added. (The transient `setSelectedLayerInfo` preview
state is bucket D, allowlisted in the guard.)

Granular setters per the rule above. These are scalar component/layer params, so
the engine can set them **in place under lock**; whole-instrument replacement is
only needed for *structural* changes (add/move/remove component or layer, replace
sample — §2.4), and even then it is the engine's local implementation, never the
wire payload (ADR 0027 invariants 2–3).

| Core mutator | GUI sites | RT | Undo? | New entry point |
|---|---|---|---|---|
| `InstrumentComponent::set{IsMuted,IsSoloed,Gain}` | ComponentView.cpp 277, 295, 307 | yes | no | `setComponent*(nInstr, nComp, …)` — in-place |
| `InstrumentLayer::set{IsMuted,IsSoloed,Gain,PitchOffset}` | ComponentView.cpp 428, 448, 464, 577, 608 | yes | no | `setLayer*(nInstr, nComp, nLayer, …)` — in-place |
| `InstrumentLayer::set{StartVelocity,EndVelocity}` | LayerPreview.cpp 838, 845 | no | no | `setLayerVelocityRange(nInstr, nComp, nLayer, …)` — in-place |

### 2.4 Sample (SampleEditor) — ✅ DONE (verified)

Verified: no direct `Sample::set*` / `Instrument::setSample` path escapes the
`SE_replaceInstrumentAction` → `replaceInstrument` commit (guard-clean — `pSample`
and `pInstrument` mutators are in the guard's pattern). The in-memory-only-sample
PCM transfer remains the sole warranted `QByteArray` payload (a deferred editor-IPC
concern, not a standalone gap).

`Sample::set{Loops,Rubberband,VelocityEnvelope,PanEnvelope}` and
`Instrument::setSample` (SampleEditor.cpp 692–713) already commit through
`SE_replaceInstrumentAction` → `replaceInstrument` (bucket A). These *are*
structural (the layer's sample is swapped), so the engine does the
whole-instrument replacement locally — but the **IPC delta** is still minimal:
the loop/rubberband/envelope values + the layer coordinates (metadata the engine
re-applies to its loaded sample), or a **file path** when a new sample is loaded
(engine reloads from disk). The one genuine bulk-transfer case is a sample that
exists **only in editor memory** (freshly recorded/imported, not yet on disk):
there the PCM must cross IPC once — acceptable as a one-off, and the only place a
`QByteArray` sample payload is warranted. **Action: verify no direct path escapes
the replace commit; confirm the in-memory-only-sample case is the sole PCM
transfer; no new scalar CAC expected.**

### 2.5 Drumkit properties (DrumkitPropertiesDialog) — ✅ DONE

Covered via the **detached working-copy** path: `DrumkitPropertiesDialog` edits a
`std::make_shared<Drumkit>` copy (the `ppInstrument->setType` line is allowlisted in
the guard as a working-copy edit) and commits through `SE_switchDrumkitAction` →
`CoreActionController::setDrumkit` (bucket A). Guard-clean. (This kept the
whole-kit-replace commit rather than adding the granular `setDrumkit*` setters the
row below proposed — the cross-split re-decode cost noted there is an editor-IPC
optimisation to revisit when the wire payload matters, not a standalone gap.)

| Core mutator | GUI sites | Undo? | New entry point |
|---|---|---|---|
| `Drumkit::set{Name,Author,Version,Info,License,Tags,Image,ImageLicense}` | DrumkitPropertiesDialog.cpp 1034–1097 | yes (`SE_switchDrumkitAction` → `SoundLibraryPanel::switchDrumkit` GUI static) | granular `setDrumkit{Name,Author,…}(value)` setters — **scalar metadata, in-place on the engine's kit, no whole-kit payload**. (Untangle from the GUI static; do **not** route via `SE_switchDrumkitAction`/whole-drumkit replacement for a property edit — that path would re-decode the entire kit across the split.) |

### 2.6 Song-level & sequencer/pattern (SongEditor, MainForm, MainToolBar) — ⚠ PARTIAL

**Done:** `setPanLaw` (CAC, guard-clean); the MixerLine "stop samples"
(`Note::setNoteOff`) is now `CoreActionController::previewInstrument(nLine, true)`.
**Still open** (these are `Hydrogen`/`PatternList` calls, not `Basics`, so the guard
does not reach them and they remain direct in the GUI):
* `Hydrogen::sequencerPlay` — `MainForm.cpp:2867`, `PlaylistEditor.cpp:954`,
  `MainToolBar.cpp:759`. Not yet on `IEngineAccess`/CAC (`sequencerStop` is).
* `Hydrogen::setSelectedPatternNumber` — `SongEditorPatternList.cpp:537` (should call
  the existing CAC `selectPattern`).
* `Hydrogen::toggleNextPattern` — `SongEditorPatternList.cpp:785` (no CAC entry yet).
* `PatternList::replace` in `movePatternLine` — `SongEditorPatternList.cpp:521–530`
  (needs CAC `movePattern(nFrom,nTo)`).
* `updateVirtualPatterns` / `updateSongSize` reactions remain direct — best fired
  from the CAC mutation via `EventListener` rather than called by the GUI.

| Mutator | GUI sites | RT | Undo? | CAC | New entry point |
|---|---|---|---|---|---|
| `Song::set{PanLawType,PanLawKNorm}` | MixerSettingsDialog.cpp 132, 149 | yes | no | no | `setPanLaw*(…)` |
| `Note::setNoteOff(true)` ("stop samples") | MixerLine.cpp 88 | yes | no | no | fold into a stop-notes command |
| `Hydrogen::sequencerPlay` / `sequencerStop` | PlaylistEditor 147/954/957; MainForm 234/674/945/2132/2872; MainToolBar 761/766/791 | n/a | no | `sequencerStop` in `IEngineAccess`; play not | add `sequencerPlay` to the surface |
| `Hydrogen::setSelectedPatternNumber` | SongEditorPatternList.cpp 537 | no | (move cmd) | partial (`selectPattern`) | use `selectPattern` |
| `Hydrogen::toggleNextPattern` | SongEditorPatternList.cpp 785 | yes | no | no (engine has it) | CAC `toggleNextPattern` |
| `Hydrogen::{updateSongSize,updateVirtualPatterns,updateSelectedPattern}` | PatternEditorPanel.cpp 1728; SongEditorPatternList.cpp 325, 534 | yes | no | no | these are *reactions* — prefer firing from CAC mutations + EventListener, not new commands |
| `PatternList::replace` | SongEditorPatternList.cpp 521–530 | yes | yes (`SE_movePatternListItemAction` → GUI method) | no | `movePattern(nFrom, nTo)` (untangle from `movePatternLine`) |

### 2.7 Pattern-cell / song-grid (SongEditor) — ✅ DONE

`SongEditor::addOrRemovePatternCellAction` now calls
`CoreActionController::toggleGridCell(gridPoint)` (`SongEditor.cpp:144`); CAC mutates
the pattern group and fires the event, the view reacts. Guard-clean.

---

## 3. Bucket C — reroute to the layered config (NOT CAC) — ✅ DONE

Interpolation mode: `PreferencesDialog` writes `Preferences::m_interpolateMode`;
`ExportSongDialog` uses the transient `Hydrogen::setInterpolateModeOverride` for the
render (no direct `Sampler::setInterpolateMode` left). MIDI action-table /
instrument-map edits persist via `Preferences::save()` (ADR 0023 path). The
audio/MIDI **driver** config crosses as override-layer config per
[ADR 0029](/docs/decisions/0029-audio-driver-access-across-editor-split.md). Metronome
volume lives in `Preferences` and is applied to the live metronome instrument as a
`preferencesChanged` reaction (`HydrogenApp.cpp:1661`).

| Mutator | GUI sites | Target |
|---|---|---|
| `Sampler::setInterpolateMode` | ExportSongDialog.cpp 940–952; PreferencesDialog.cpp 964–976 (10 total) | Preferences → layered config (ADR 0022) |
| metronome `Instrument::setVolume` from prefs | HydrogenApp.cpp 1658 | Preferences |
| MIDI action-table edits | `SE_addOrRemoveMidiEventsAction`, `SE_replaceMidiEventsAction` (→ MIDI table); `SE_modifyCustomLibraryDirsAction` | `MidiEventMap` / custom dirs = config, persisted via ADR 0023 |

No other `MidiEventMap`/`MidiInstrumentMap` direct mutations were found (read-only
elsewhere).

---

## 4. Bucket D — display / editor-local (never crosses IPC) — ✅ DONE

The GUI no longer writes peaks into the engine model: `Instrument::setPeak_L/R` and
`AudioEngine::setMasterPeak_L/R` are gone from `src/gui/src` (the remaining
`setPeak_L/R` calls are on the `Fader` widget — display only). Export/selection/grid
flags stay editor-local as designed.

| Mutator | GUI sites | Disposition |
|---|---|---|
| `AudioEngine::setMasterPeak_L/R` | Mixer/MasterLine.cpp 235–236 | telemetry read in editor mode (ADR 0018) |
| `Instrument::setPeak_L/R` | SongEditorPanel.cpp 711, 714 | telemetry read; stop writing model from GUI |
| `Instrument::setCurrentlyExported` | ExportSongDialog.cpp 568, 725, 728 | export runs engine-side; flag stays there |
| `GridPoint::setColumn/setRow` | editor cursor state | pure UI, local |
| selection / `SampleEditor` staging buffers | various | local until a bucket-B commit |

---

## 5. The 16 hand-rolled lock sites (absorb into CAC) — ⚠ PARTIAL

**Absorbed** (lock moved into the CAC entry point, GUI block deleted):
`SongEditor.cpp` 207/217; `PatternEditorRuler.cpp` 117/127;
`PatternEditorPanel.cpp` 1506/1528, 1724/1729.

**Still present** (their edit is not yet a CAC call — tracks the open §2.6 items and
the editor's own staging): `SampleEditor.cpp` 1535/1543, 1556/1566;
`SongEditorPositionRuler.cpp` 838/883; `SongEditorPatternList.cpp` 966/996; plus
`ExportSongDialog.cpp` 762/765 (export render). These are not caught by the current
guard (it does not enforce hand-rolled `lock()`); folding them in is part of closing
§2.6 and the guard-scope extension.

---

## 6. Sweep order (each step keeps the suite green)

1. ✅ **PatternEditor** (note edits, pattern size, cell toggles).
2. ⚠ **SongEditor / pattern list** — song-grid cell ✅; **move/select/toggle pattern
   still open** (see §2.6).
3. ✅ **Rack/InstrumentEditor** (instrument params + ADSR; missing undo added).
4. ✅ **Rack/ComponentView + LayerPreview** (component/layer; undo added).
5. ✅ **Dialogs** (Drumkit/Song/Pattern properties; MixerSettings pan-law).
6. ✅ **Config reroutes** (bucket C, incl. ADR 0029 drivers) and **display
   cleanups** (bucket D).
7. ⚠ **CI guard** — `tools/check_write_surface` is in CI and **green** for
   engine-owned `Basics`; **extending it to `AudioEngine`/`Sampler`/`Transport`
   (and hand-rolled `lock()`) is still pending** (see the guard header's "NOT yet
   enforced" note).

**Remaining to finish the phase:** the §2.6 sequencer/pattern-list verbs
(`sequencerPlay` onto the surface, `selectPattern`/`toggleNextPattern`/`movePattern`),
absorbing the §5 locks that wrap them, and the step-7 guard-scope extension. Editor
mode then resumes as CAC-over-IPC against the (then-complete) CAC surface.
