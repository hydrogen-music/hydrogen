---
status: accepted
date: 2026-06-23
deciders: pm
---

# AD: Audio driver access across the editor↔engine split

## Context and Problem Statement

The out-of-process editor ([ADR 0016](0016-out-of-process-plugin-ui.md)) reaches
the engine through an injected handle: state *reads* go through `IEngineAccess`
(Phase 2), *writes* through `CoreActionController`
([ADR 0027](0027-coreactioncontroller-single-write-surface.md)), continuous
*telemetry* (peaks/playhead/process-time) through a shared-memory block, and
discrete *events* through the control socket
([ADR 0018](0018-plugin-editor-ipc-transport.md)).

The **audio driver** (`src/core/IO/AudioDriver.h`) does not fit any of those
seams. A review of `src/gui/src/` (2026-06-23) shows the GUI reaches it by holding
a live `std::shared_ptr<AudioDriver>` — exposed on the read surface as
`IEngineAccess::getAudioDriver()` (`IEngineAccess.h:63`), forwarded by
`IpcEngineAccess` to the **mirror's** driver (`IpcEngineAccess.h:60-61`) — and
then reads scalars off it, enumerates devices, *and casts it to its concrete
subclass*:

* **Scalars** — `getSampleRate()` (10 sites, e.g. `AudioFileBrowser.cpp:594`,
  `PlaybackTrackWaveDisplay.cpp:124`, `SampleEditor.cpp:1362` — used in *resample
  math*, not just display), `getBufferSize()` and `getLatency()`
  (`AudioEngineInfoForm.cpp:143,148`; `PreferencesDialog.cpp:1571`), `getXRuns()`
  (`HydrogenApp.cpp:835`, on the `Xrun` event).
* **Enumeration** — `getDevices()` / `getDevices(hostAPI)` / `getHostAPIs()`
  (`PreferencesDialog.cpp:75-92`) and the static `AlsaAudioDriver::getAlsaDevices()`
  populate the device/host-API combos.
* **Concrete-type introspection** — ~24 `std::dynamic_pointer_cast<…>` on the
  driver pointer: `NullDriver` ("is a real driver running?" — `MainForm.cpp:216,2224`,
  `MainToolBar.cpp:743,780`, `PreferencesDialog.cpp:935,1960`) and
  `JackDriver`/`AlsaAudioDriver`/`PortAudioDriver`/`CoreAudioDriver`/
  `PulseAudioDriver`/`OssDriver` to pick the right device list, show driver-
  specific options (PortAudio host-API + latency target, JACK connect-defaults/
  track-outs) and read driver-specific fields (e.g.
  `pAlsaDriver->m_sAlsaAudioDevice`, `PreferencesDialog.cpp:1240-1340`).
* **Export** — `dynamic_pointer_cast<DiskWriterDriver>` then `->m_bWritingFailed`
  (`ExportSongDialog.cpp:880`); during export the active driver *is* the disk
  writer.

The audio driver is an engine-owned, hardware- and RT-bound object living in the
host process in editor mode. A `getAudioDriver()` handed across the split returns
the mirror's headless driver: `getSampleRate()` is wrong, `getDevices()` is empty,
every `dynamic_pointer_cast` fails (the mirror has no concrete driver), and the
`NullDriver` gates misfire. As with the direct `Basics` mutations of
[ADR 0027](0027-coreactioncontroller-single-write-surface.md), the code compiles
against `IEngineAccess` yet silently produces wrong results across the split. The
[ADR 0027](0027-coreactioncontroller-single-write-surface.md) sweep was scoped to
engine-owned `Basics`/`AudioEngine`/`Transport` and deliberately did **not**
classify the drivers; this ADR does so for the **audio driver** (the MIDI driver
is the analogous case — see the closing note).

The distinctive problem the audio driver adds over any other engine state is
**`dynamic_pointer_cast` on a live pointer**: RTTI cannot cross a process
boundary, so "branch on the concrete driver type" has no meaning in editor mode
and needs an explicit replacement.

## Decision Drivers

* The driver object must **not cross IPC**: it is RT-bound, holds OS handles, and
  carries platform `#ifdef`-gated subclasses. The editor needs the *data and the
  type*, not the object.
* `getSampleRate()`/`getBufferSize()` feed **local DSP math** (on-the-fly resample
  ratio in the wave displays), so they must be available **synchronously and
  locally** in the editor — a round trip per repaint is unacceptable.
* "Branch on concrete driver type" (`dynamic_pointer_cast`) must survive the split
  as a value, not as RTTI on a remote pointer.
* The host owns audio I/O in plugin mode
  ([ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md)); driver/device
  *selection* is already settled config and must not be reinvented here.
* Standalone behaviour stays unchanged, as in
  [ADR 0027](0027-coreactioncontroller-single-write-surface.md).

## Considered Options

1. **Classify each access into config / query / event, and replace
   `dynamic_pointer_cast` with a value *driver descriptor*; remove
   `getAudioDriver()` from the read surface.**
2. **Marshal the driver object** behind a serialised proxy the editor
   reconstructs and casts. Rejected: the proxy must mirror live hardware-bound
   state and re-expose every platform subclass; it reinvents the
   telemetry/event/query split as one bespoke object and drags driver internals
   and `#ifdef`s onto the thin engine side.
3. **Keep `getAudioDriver()`, make the driver itself IPC-aware** (a remote-driver
   subclass behind the mirror). Rejected: pushes socket/RT concerns into every
   driver backend, the opposite of
   [ADR 0018](0018-plugin-editor-ipc-transport.md)'s thin-engine goal, and still
   hands the GUI a castable pointer that means nothing remotely.

## Decision Outcome

Chosen option: **option 1 — audio-driver state crosses the split as config,
query, or event, and the concrete-type branch becomes a value descriptor; never a
driver pointer.**

The invariant, parallel to [ADR 0027](0027-coreactioncontroller-single-write-surface.md):
**GUI code MUST NOT obtain, dereference, or `dynamic_cast` an `AudioDriver`
object.** `IEngineAccess::getAudioDriver()` is **removed** from the read surface.
Every current use maps to exactly one of the following.

### 1. Config — host-owned values via the layered config (override layer)

`getSampleRate()` and `getBufferSize()`, the active-driver *selection*, and the
driver-specific device/option fields (`m_sAlsaAudioDevice`, `m_sOSSDevice`,
`m_sPortAudioDevice`/`m_sPortAudioHostAPI`, `m_nLatencyTarget`, the JACK
`m_bJackConnectDefaults`/`m_bJackTrackOuts`) are the override subset of
[ADR 0022](0022-layered-plugin-configuration.md) (`m_nSampleRate`, `m_nBufferSize`,
`m_audioDriver`, …). The GUI reads them from `Preferences`; the
`writeAudioDriverPreferences()` write path stays config (in plugin mode the I/O
controls are hidden — [ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md)).
This is what makes `getAudioDriver()->getSampleRate()` a **local** `Preferences`
read, satisfying the DSP-math sites without a round trip.

### 2. Query — live enumeration and status that is *not* config

`getDevices()`/`getDevices(hostAPI)`/`getHostAPIs()`/`getAlsaDevices()` (available
devices and host APIs) and `getLatency()` (live, possibly variable, frames) are
on-demand, low-rate reads that need a round trip. They become typed, data-
returning `IEngineAccess` accessors (e.g. `getAudioDevices(...)` →
`QStringList`, `getAudioLatencyFrames()` → `int`), backed in editor mode by a
request/response message pair on the existing `QLocalSocket`. They are not
telemetry (not periodic) and not events (pulled when a dialog opens or on an
`AudioDriverChanged` event). In plugin mode the device-selection UI is hidden
([ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md)), so these are mostly
a standalone concern; the mechanism is defined for the status the editor still
shows.

### 3. Event — runtime counters and one-shot status

`getXRuns()` is read in `HydrogenApp::XRunEvent()`, already driven by the
engine-origin `Event::Type::Xrun`; the xrun count rides in that event's payload,
so the editor never touches the driver. The export write-failure
(`DiskWriterDriver::m_bWritingFailed`) is reported through the existing export
progress/completion events, **not** a `dynamic_pointer_cast<DiskWriterDriver>` —
export is an engine-side offline flow whose result already crosses as events.

### 4. Driver descriptor — the replacement for `dynamic_pointer_cast`

Every `dynamic_pointer_cast<ConcreteDriver>` and every `NullDriver` "is a real
driver running?" gate is replaced by a **value descriptor** the engine fills and
the editor reads, e.g.:

```cpp
struct AudioDriverInfo {            // value type; crosses IPC as plain fields
    Preferences::AudioDriver kind;  // Jack / Alsa / PortAudio / CoreAudio /
                                    // PulseAudio / Oss / Disk / Null / Fake / None
    bool      isRunning;            // replaces the NullDriver "is real?" gate
    bool      isCompiledIn;         // replaces the per-driver #ifdef NOT-compiled note
    // driver-specific scalars already mirrored in config are read from Preferences;
    // anything genuinely live (e.g. connected device name) is filled here.
};
```

The GUI branches on `info.kind` instead of the runtime type of a pointer; the
`NullDriver`/`nullptr` checks become `! info.isRunning`. Each driver reports its
own `kind` via an enum (it already maps to `Preferences::AudioDriver`), so no RTTI
crosses the wire. `LocalEngineAccess` fills the descriptor from the live driver
(`dynamic_pointer_cast` stays, but **inside core**, on the engine side);
`IpcEngineAccess` fills it from the query response. This single descriptor
subsumes the ~24 cast sites.

### Seam summary

| Current GUI access | Category | New seam |
|---|---|---|
| `getSampleRate()`, `getBufferSize()`; driver/device *selection*, JACK/PortAudio options | Config | `Preferences` override layer ([ADR 0022](0022-layered-plugin-configuration.md)); UI hidden in plugin mode ([ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md)) |
| `getDevices()/getHostAPIs()/getAlsaDevices()`, `getLatency()` | Query | request/response → typed `IEngineAccess` accessors |
| `getXRuns()`; `DiskWriterDriver::m_bWritingFailed` | Event | `Xrun` event payload; export progress/completion events |
| `dynamic_pointer_cast<ConcreteDriver>`, `NullDriver` "is real?" gates | Descriptor | `AudioDriverInfo` value struct (kind + isRunning + isCompiledIn) |
| `getOut_L/R()`, `init/connect/disconnect()` | — | engine/RT-only; never GUI or IPC |

The driver object stays entirely engine-side. `IpcEngineAccess` no longer forwards
a pointer; it answers scalars from override config, enumeration/latency from
queries, the descriptor from a query, and counters from events.
`LocalEngineAccess` backs the same accessors off the real driver, so the
standalone path is behaviourally unchanged.

`AudioEngineInfoForm` — the diagnostics window that drove much of this — is then a
pure consumer of these seams: process time / playhead / realtime-frame / state
come from the telemetry block and events
([ADR 0018](0018-plugin-editor-ipc-transport.md)); buffer size / sample rate from
config; latency and driver name/kind from the query + descriptor.

### Consequences

* **`IEngineAccess` loses `getAudioDriver()`** and gains a small typed set:
  scalar/status accessors, a device/host-API query, and the `AudioDriverInfo`
  descriptor. Every call site (`AudioEngineInfoForm`, `PreferencesDialog`,
  `MainForm`, `MainToolBar`, `ExportSongDialog`, the wave displays) migrates to
  them; this is a precondition for those dialogs working in editor mode.
* **The ~24 `dynamic_pointer_cast` sites collapse** onto one descriptor — a net
  simplification even in standalone, and the platform `#ifdef`s move out of the
  GUI into the engine that owns them.
* **New IPC surface, all on existing transports:** one request/response pair for
  device/latency/descriptor queries, and reuse of the existing `Xrun` and export
  events. No new shared-memory region; the telemetry block is untouched.
* **The thin engine side stays thin:** no driver backend learns about IPC; no
  driver object is serialised.
* **Standalone is unchanged:** `LocalEngineAccess` answers from the live driver,
  the casts simply move behind it.
* **Deferred editor-mode work, not part of the
  [ADR 0027](0027-coreactioncontroller-single-write-surface.md) sweep.** The audio
  driver is not `Basics`, so the write-surface CI guard neither covers nor flags
  it. Sequenced with the editor-mode IPC tasks (T5.2-cont/T5.3) in
  [proposal 0004](/docs/proposals/0004-plugin-port-implementation-plan.md), after
  the live socket exists.
* **No instance-identity translation** ([ADR 0028](0028-object-instance-identity.md))
  is needed: every payload here is a value (scalars, strings, the descriptor
  enum), carrying no engine pointers.

### The MIDI driver is the analogous case — implemented here too

`MidiBaseDriver` follows the same model under the same invariant (no driver
pointer in the GUI), and was swept alongside the audio driver (kept folded into
this ADR rather than split out):

* port *selection* is override config (unchanged);
* `getExternalPortList()` → **query** `IEngineAccess::getMidiPorts(PortType)`;
* the `getHandledInputs()/getHandledOutputs()` activity log → snapshot accessors
  `getHandledMidiInputs()/getHandledMidiOutputs()` (an ordered, variable-length
  **event** stream — not the fixed-size lossy telemetry block; the IPC event
  feed is the deferred editor-mode step, like the audio queries);
* `clearHandledInput()/clearHandledOutput()` → **commands**
  `CoreActionController::clearMidiInputLog()/clearMidiOutputLog()`;
* presence + input/output-active status → a small `MidiDriverInfo` value struct
  (`{isPresent, isInputActive, isOutputActive}`) — no *kind* field is needed, as
  the MIDI side has no GUI-visible concrete-subclass branching (no
  `dynamic_pointer_cast`).

`IEngineAccess::getMidiDriver()` was removed; `LocalEngineAccess` backs the
accessors off the live driver, `IpcEngineAccess` returns deferred stubs.

## More Information

* Code: `src/core/IO/AudioDriver.h` (`getSampleRate`/`getBufferSize`/`getLatency`/
  `getXRuns`/`getDevices`); `src/core/IEngineAccess.h`,
  `src/core/LocalEngineAccess.h`, `src/core/IPC/IpcEngineAccess.h`;
  `src/gui/src/AudioEngineInfoForm.cpp`,
  `src/gui/src/PreferencesDialog/PreferencesDialog.cpp`,
  `src/gui/src/MainForm.cpp`, `src/gui/src/MainToolBar/MainToolBar.cpp`,
  `src/gui/src/ExportSongDialog.cpp`,
  `src/gui/src/SongEditor/PlaybackTrackWaveDisplay.cpp`,
  `src/gui/src/AudioFileBrowser/AudioFileBrowser.cpp`,
  `src/gui/src/SampleEditor/SampleEditor.cpp`.
* Related: [ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0018](0018-plugin-editor-ipc-transport.md),
  [ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md),
  [ADR 0027](0027-coreactioncontroller-single-write-surface.md),
  [ADR 0028](0028-object-instance-identity.md),
  [proposal 0004](/docs/proposals/0004-plugin-port-implementation-plan.md).
