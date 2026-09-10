---
status: accepted
date: 2026-06-08
amended: 2026-09-10
deciders: pm
---

# AD: Editor↔engine IPC uses a control socket plus shared-memory telemetry

## Context and Problem Statement

The plugin editor runs out-of-process ([ADR 0016](0016-out-of-process-plugin-ui.md)),
but the engine lives in the host process. They must exchange three kinds of data
that the existing in-process code already treats differently:

* **Discrete state events** (engine→editor) — today `EventQueue`
  (`src/core/EventQueue.h`), `Event::Type` in `Event.h:56`, polled by the GUI at
  **20 Hz** (`HydrogenApp.cpp:95`, `QUEUE_TIMER_PERIOD = 50` ms), max 1024
  events. Bursty; must not drop or reorder (`PatternChanged`, `DrumkitLoaded`…).
* **Commands** (editor→engine) — today direct `CoreActionController::*` calls
  (`setBpm`, `setDrumkit`, `handleNote`, `addInstrument`…). Reliable, ordered,
  low-rate.
* **Continuous telemetry** (engine→editor) — today read directly from live
  members: `Instrument::getPeak_L/R()`, `AudioEngine::getPlayhead()`,
  `getMasterPeak_*`, `getProcessTime()`. "Latest value wins", lossy-tolerant,
  wants smooth ~30–60 Hz refresh.

The engine half runs in the host's real-time context: **nothing on the IPC path
may block the audio thread.** Forcing reliable ordered messaging and high-rate
lossy telemetry through one mechanism is the trap — a single reliable stream
makes meters chatty and can starve commands; a single lock-free region makes
variable-length ordered messages painful.

## Decision Drivers

* Audio thread must never block on IPC.
* Events/commands need reliability + ordering; telemetry needs low latency and is
  loss-tolerant.
* Reuse the existing split (events via queue, meters/playhead via direct reads)
  so editor-side code barely changes.
* Cross-platform (Linux/Windows/macOS) and already-Qt where possible.

## Considered Options

1. **Hybrid: `QLocalSocket` control/event channel + shared-memory telemetry block.**
2. Single local socket carrying events, commands, *and* telemetry as messages.
3. Pure shared memory: two SPSC ring buffers + eventfd/semaphore wakeup, no socket.
4. Qt Remote Objects / an RPC framework.

## Decision Outcome

Chosen option: **Hybrid (option 1).**

* A **`QLocalSocket`** (cross-platform, already-Qt, reliable, ordered) carries
  discrete events engine→editor and commands editor→engine. Commands map 1:1
  onto the existing `CoreActionController` vocabulary — each method becomes one
  message type. Large payloads (song / drumkit) reuse the existing XML
  serialisation rather than a bespoke codec.
* A **versioned shared-memory struct** holds the latest telemetry: playhead frame
  + BBT, master peaks, per-instrument peaks (fixed-size array + live count),
  process time. The engine writes it lock-free each audio buffer; the editor
  reads it at its own refresh rate. This mirrors how the GUI reads peaks/playhead
  directly today.
* An **IPC bridge thread** on the plugin side drains `EventQueue` and forwards to
  the socket, keeping the audio thread untouched.

**Event classification (not every event crosses IPC).** `EventQueue` events split
into *engine-origin* events (transport, song, instruments, …) that are marshalled
engine→editor, and *editor-internal* events that originate and are consumed within
the editor process and must **not** be marshalled. `OnlineImportProgress` is the
worked example: online import is editor-side library management
(`OnlineImporter`, also used headless by the CLI), so its progress events — and
its Qt signals to the dialog — stay entirely in the editor process. Each event
type must be tagged with its origin so the bridge thread forwards only
engine-origin events.

Option 2 (single socket) is rejected: meter traffic at 30–60 Hz × N instances is
chatty and adds jitter to commands. Option 3 (pure shm rings) is rejected as
hand-rolled framing/wakeup that a socket gives for free — overkill for a drum
editor. Option 4 (QtRO/RPC) is rejected: heavier dependency, pulls more Qt onto
the thin engine side, and the telemetry-rate problem remains.

#### Telemetry shared-memory layout

The engine writes the block every audio buffer while the editor reads
asynchronously, so reads must be **tear-free**. Use a **seqlock**: an atomic
`seq` bumped before and after each write; the reader samples `seq`, copies the
struct, re-checks, and retries if `seq` changed or is odd — no mutex on the audio
thread. The block is a POD struct, attached once and validated by a
`formatVersion` constant:

```cpp
struct EngineTelemetry {            // single-writer (engine) / single-reader (editor)
    uint32_t          formatVersion;     // checked once at attach
    std::atomic<uint32_t> seq;           // seqlock
    int64_t           frame;             // playhead
    int32_t           bar, beat, tick;   float bpm;
    uint8_t           playing;
    float             masterPeakL, masterPeakR;
    float             procTimeCur, procTimeMax;
    float             playbackTrackPeakL, playbackTrackPeakR;  // playback-track meters
    uint16_t          instPeakCount;     // valid entries below
    float             peakL[256], peakR[256];   // per-instrument meters
};
```

The `looping` field was present in the original layout but has been removed:
loop mode is seldom-changing, already carried on the Song object, and
event-forwarded over the control socket — duplicating it in the high-rate
telemetry block was unnecessary. The `playbackTrackPeakL/R` fields were added
so the editor can render the playback-track waveform display from telemetry
instead of accessing the engine's instrument layer directly.

The per-instrument peak array is a **fixed cap of 256** (there is no
`MAX_INSTRUMENTS`; at 8 bytes/entry this is ~2 KB — negligible). Instruments
beyond the cap get no meter (logged once). This cap concerns *instruments shown in
the mixer* and is independent of `H2_PLUGIN_OUTPUT_BUSES`
([ADR 0019](0019-plugin-output-bus-layout.md)). A `formatVersion` mismatch between
editor and plugin disables telemetry (editor falls back to events-only) rather
than crashing.

#### Control-channel wire format

Length-prefixed framed messages over `QLocalSocket` using **`QDataStream` with a
pinned version** (Qt-native, already a dependency, handles `QString`/containers,
explicitly versionable). A message is `[u32 length][u16 opcode][payload]`; opcodes
cover the `CoreActionController` commands, forwarded engine events, the
rescan-sound-library command ([ADR 0016](0016-out-of-process-plugin-ui.md)), and a
`hello` version handshake. Small messages carry compact typed fields — events are
trivial (`type` + `int nValue` + `long nId`, `Event.h`), commands carry their
typed args. **Large structured payloads (Song, Drumkit, full state) ride as a
`QByteArray` holding the existing XML serialisation** — the battle-tested
`Song`/`Drumkit` serializers are reused, not reinvented in binary. The protocol
version is negotiated in `hello`; a mismatch fails gracefully.

### Consequences

* Two mechanisms to build and version: the `QDataStream` message protocol and the
  seqlock'd shared-memory layout (both above). Each carries a version checked at
  connect/attach; a mismatch degrades gracefully (events-only telemetry / refused
  handshake), never crashes.
* Shared-memory segment lifecycle must be managed across processes (create with
  the instance, name per instance, clean up on teardown/crash).
* The `CoreActionController` surface effectively becomes the IPC command schema;
  changes to it must be reflected in the protocol.

## Amendment (2026-09-10): full telemetry payload, bridge-thread publish, editor-side apply

Since acceptance, the telemetry pipeline was implemented end-to-end for the
`--connect-via-ipc` editor mode. Three points refine the accepted text above
(which is kept as the baseline):

1. **Publishing thread — bridge thread, not the audio thread.** The accepted
   text says the engine writes the block "each audio buffer". The
   implementation instead publishes from the engine's IPC bridge thread
   (`EngineSession::publishTelemetry`) at a **~50 ms cadence** — enough for
   meters, and it keeps even the seqlock write off the audio thread. The
   bridge thread builds the snapshot under a short `tryLockFor` on the engine
   mutex; the audio thread only ever *try*-locks that mutex, so neither
   thread can block the other. On lock contention the publish degrades to a
   transport-only snapshot for that cycle (meter fields zero) — the editor
   simply sees a stale meter for 50 ms.

2. **Peaks are read consume-style (ADR 0027).** `buildTelemetrySnapshot`
   reads master / per-instrument / playback-track peaks with the existing
   consume (read + reset) accessors rather than plain reads. In the headless
   engine process no GUI drains these accumulators, so a plain read would
   latch at the running maximum — the pre-existing playback-track meter bug.
   Per-instrument peaks are capped at the 256-entry array in drumkit order,
   with `instPeakCount` reporting the valid entries. Process time is a plain
   read (it is not an accumulator).

3. **Editor-side apply — GUI-thread-only writes.** Transport fields stay on
   the accepted hybrid path (events + 5 s resync, ADR 0031). The meter
   payload is applied by a new **50 ms GUI-thread timer** in
   `EditorStateMirror` (`syncMetersFromTelemetry` → `applyMeterSnapshot`)
   which **max-merges** the snapshot peaks onto the mirror's existing peak
   members. The mirror is the sole writer of those members in editor mode
   (its render path is gated on `!= ProcessMode::Editor`), so all writes
   happen on the GUI thread — race-free by the same argument as
   [ADR 0029](0029-value-views-over-live-pointers.md). The shm block may not
   exist yet when the editor learns the endpoint (`serve()` advertises it
   first), so the mirror retries the attach lazily on each transport sync
   until it succeeds.

   **Process time is deliberately *not* written into the mirror's
   `AudioEngine` process-time members**: those size the mirror's own
   `tryLockFor` slack budget and must reflect the mirror's local render cost.
   Widgets (`Footer` CPU label, `AudioEngineInfoForm`) read
   `procTimeCur`/`procTimeMax` from the state mirror's telemetry snapshot
   instead (`HydrogenApp::getEditorSession()->getStateMirror()->getTelemetry()`).

4. **The mirror's sample rate follows the engine.** Frame↔tick conversion
   (`Transport::computeTickFromFrame` via the local driver rate) turns every
   telemetry frame into a tick/BBT position, so a rate mismatch between the
   authoritative engine (e.g. JACK dictating 44100) and the mirror (local
   preferences) skews the whole transport display. The `GetAudioDriverInfo`
   reply therefore carries the engine's actual rate, and
   `Hydrogen::setCachedAudioDriverInfo()` re-rates the mirror's
   `SoftwareDriver` (`setSampleRate()`: the clock thread is joined around the
   update, since it reads rate and process interval unsynchronized). The
   buffer size deliberately stays local — it only paces the mirror's own
   clock loop; the engine's buffer size and latency ride along in the cached
   info for display only.

## More Information

* `src/core/EventQueue.h`, `src/core/Basics/Event.h:56`,
  `src/core/CoreActionController.h`,
  `src/core/AudioEngine/AudioEngine.h` (playhead/peak/process-time getters),
  `src/core/Basics/Instrument.h` (`getPeak_L/R`).
* Related: [ADR 0016](0016-out-of-process-plugin-ui.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
