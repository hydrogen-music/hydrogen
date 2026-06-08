---
status: proposed
date: 2026-05-20
deciders: phil (theGreatWhiteShark)
---

# AD: Progress reporting for online import

## Context and Problem Statement

The `OnlineImporter` (in `src/core/`) will download potentially large artifacts
(drumkits can be tens of MiB). The GUI needs to display a progress bar during
these downloads. The core→GUI communication in Hydrogen uses the `EventQueue`
system with typed `Event`s dispatched to `EventListener` implementations.

How do we report download progress from core to GUI?

## Decision Drivers

- Must follow the established `Event`/`EventQueue`/`EventListener` pattern.
- Must not create tight coupling between core and GUI.
- The existing `Event::Type::Progress` is used by `DiskWriterDriver` for audio
  export (0–100 range, -1 = failure). We should not overload its semantics.
- Must support reporting progress for multiple sequential downloads in a batch.
- The CLI tool should also be able to consume progress (for a text-based
  progress indicator).

## Considered Options

1. **Reuse `Event::Type::Progress`** — Multiplex by checking context (who's
   currently "exporting"?).
2. **New `Event::Type::OnlineImportProgress`** — Dedicated event type with clear
   semantics.
3. **Qt signals directly** — Make `OnlineImporter` a `QObject` and emit signals;
   bypass EventQueue entirely.

## Decision Outcome

Chosen option: **2 — New `Event::Type::OnlineImportProgress`**.

**Rationale:**

- Reusing `Progress` (option 1) would create ambiguity: a listener receiving
  `progressEvent(50)` cannot know if it's an audio export or a download without
  external state tracking. This violates the principle that events should be
  self-describing.
- Option 3 bypasses the established architecture. While `QObject` signals work
  fine for direct connections, the `EventQueue` provides thread-safe decoupling
  and is the established idiom here. The GUI's timer-based event polling
  (`HydrogenApp`) already handles dispatching efficiently.

**However**, the `OnlineImporter` will **also** be a `QObject` emitting Qt
signals for its async API (see ADR 0009). The `OnlineImportProgress` event
serves as the bridge: the GUI's `OnlineImportDialog` can connect directly to
signals when it holds a reference, while other parts of the system (status bar,
CLI) can listen via the EventQueue.

### Event Semantics

```
Event::Type::OnlineImportProgress
```

**Named constants** (defined in `OnlineImporter.h`):
```cpp
static constexpr int nProgressError = -1;
static constexpr int nProgressComplete = 101;
```

**Value encoding** (in `m_nValue`):

| Value    | Meaning                                            |
|----------|----------------------------------------------------|
| 0–100    | Overall batch progress percentage                  |
| -1       | Error occurred (download failed or hash mismatch)  |
| 101      | Batch completed successfully                       |

This mirrors the `Progress` convention (0–100, -1 = error) but adds 101 for
explicit completion signaling, which is useful because the dialog needs to know
when to re-enable its controls.

**Error reporting detail:** The event value `-1` only signals "an error
happened" for progress bar purposes. Detailed per-artifact error information
(which artifact failed, why) is communicated via the `OnlineImporter`'s Qt
signal `downloadFinished(artifactName, success, error)`. The dialog, which holds
a direct reference to the `OnlineImporter`, uses signals for specifics and the
event for progress bar state.

### EventListener Addition

```cpp
// In src/gui/src/EventListener.h
virtual void onlineImportProgressEvent( int nValue ) { UNUSED( nValue ); }
```

### Who Fires the Event

`OnlineImporter::downloadArtifacts(...)` enqueues `OnlineImportProgress` events
via `EventQueue::get_instance()->pushEvent(...)` as downloads proceed. The
progress is computed as:

```
progress = (completedBytes across all artifacts) / (totalBytes across all artifacts) * 100
```

## Consequences

- `Event::Type` enum gains one new member: `OnlineImportProgress`.
- `Event.cpp` `TypeToQString()` gains the corresponding string mapping.
- `EventListener.h` gains `onlineImportProgressEvent()`.
- `HydrogenApp.cpp`'s event dispatch switch gains the new case.
- Minimal footprint: one new enum value, one new virtual method, one new switch
  case.
