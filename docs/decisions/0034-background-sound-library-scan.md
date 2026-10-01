---
status: accepted
date: 2026-10-01
deciders: phil (theGreatWhiteShark)
---

# AD: Move the sound library scan into a background thread with progress events

## Context and Problem Statement

The `SoundLibraryDatabase` constructor ran a full `update()` synchronously:
it listed and loaded every drumkit, pattern, song, and playlist on disk
before any Hydrogen instance finished constructing. Every startup mode —
GUI, `h2player`, CLI, plugin host ([ADR
0013](0013-provide-hydrogen-as-an-audio-plugin.md)) — paid the full disk
I/O latency up front, and the GUI additionally froze on it.

The synchronous scan also sat inside a web of unsynchronized concurrent
access:

* The GUI thread can add or remove a custom sound library directory
  (`CoreActionController::addCustomSoundLibraryDir` /
  `removeCustomSoundLibraryDir`) — a read-modify-write on
  `Preferences::m_customSoundLibraryDirs` — while the scan reads the same
  list via `Filesystem::targetDirs(Custom)`.
* A scan triggered while lazily registered custom drumkits exist could
  drop their database entries: the scan built a fresh snapshot from disk
  and replaced the published one wholesale.
* Teardown during a scan was impossible to interrupt: the scan loop had
  no cancellation point, so a shutting-down instance kept loading
  drumkits nobody would read.

The snapshot store itself — copy-on-write publication under
`m_writerMutex`, readers grabbing an immutable `std::shared_ptr` — was
already in place (commit `3644c1236`); this decision is about *when* the
scan runs and how concurrent actors coordinate with it.

## Decision Drivers

* Startup must not block on sound library disk I/O in any process mode
* Concurrent registrations and scans must not lose updates
* Teardown must be able to interrupt a running scan promptly
* Users should see scan progress instead of an unresponsive UI
* Correctness over convenience: no partial or torn publications

## Considered Options

1. **Keep the synchronous constructor scan** — status quo. Simple, but
   every startup mode keeps paying the full latency and the races remain.
2. **Lazy scan on first database access** — defers the cost out of the
   constructor, but moves the full stall to an arbitrary first access
   (e.g. the first drumkit change), where a blocking wait is far harder
   to reason about and to cancel.
3. **Background worker thread with progress events and explicit wait
   gates** — the scan runs off-thread; consumers that need a populated
   database wait explicitly; the scan is interruptible and reports
   progress.

## Decision Outcome

I chose **option 3**: a dedicated scan worker with an explicit
coordination surface.

* **Worker start at the end of the `Hydrogen` constructor**
  (`SoundLibraryDatabase::startInitialScan()`), not in the database
  constructor: mid-ctor the member initialization order is fragile and
  the `EventQueue` is not yet in a usable state. Nothing in the
  constructor tail reads the database anymore — `getEmptySong()` loads
  its default kit from disk directly (commit `88646f628`).
* **Progress reporting** via a new `Event::Type::SoundLibraryScanProgress`
  (0–100, throttled to 5% steps) pushed through the `EventQueue`. Events
  pushed before `setFullyOperational()` are dropped by the queue — the
  earliest progress reports are lost, which matches the pre-existing
  behavior for all non-Error events and is accepted.
* **Explicit wait gates**: `waitForInitialScan()` (condition variable)
  blocks only where a populated database is actually required —
  `getNextDrumkit()` / `getPreviousDrumkit()` (GUI thread and the MIDI
  action worker; never the audio thread) and the test harness. The GUI
  shows progress instead of waiting: the footer's status message display
  (`HydrogenApp::showStatusBarMessage`) in `MainForm`.
* **Terminal interrupt**: `std::atomic<bool> m_bStopScan`, set only by
  `shutdown()` (flag + join, idempotent), checked at the entry of
  `update()` / `updatePatterns()` / `updateSongs()` / `updateDrumkits()`,
  at every loop top, and between phases. An interrupted scan publishes
  nothing partial — the previous snapshot stays. `shutdown()` runs first
  in `~Hydrogen`, with the database destructor as a safety net.
* **Entry merge in `publish()`**: custom drumkit paths registered
  concurrently with a scan keep their entries — for a merged path
  missing in the new snapshot, the entry, its `DrumkitInfo`, and its
  unique label are carried over. Entries win over disk state.
* **Closed Preferences race**: `getCustomSoundLibraryDirs()` returns a
  copy under a new `QMutex` in the `Preferences` wrapper class (following
  the `m_persistedFootprintMutex` precedent). The mutex deliberately
  does *not* live in `PreferencesData`: that base is copied, and its
  member list is guarded by a structured-binding canary — a mutex member
  would break both.
* **Per-thread logger scope**: the worker opens its own
  `Logger::Scope` on the owning instance's logger — the scope is
  `thread_local` and not inherited from the spawning thread ([ADR
  0015](0015-per-instance-engine-context.md), T1.6). Without it the
  worker's logs pollute the process-default logger.

### Positive consequences

* All startup modes construct without sound library disk I/O
* Scan progress is visible in the GUI; the MIDI action path and the GUI
  drumkit navigation degrade to a bounded wait instead of reading a
  half-populated database
* Concurrent registrations and scans no longer lose updates
* Teardown interrupts a running scan at the next loop iteration

### Negative consequences

* GUI and MIDI action threads can block for the duration of a scan on
  huge or network-mounted libraries — accepted for now; a bounded wait
  with a "library still scanning" fallback is a possible follow-up
* There is still no unregister API for custom drumkit paths: a
  registered kit whose files are deleted stays listed (and re-logs a
  load error on every rescan) until process restart — follow-up ticket
* `m_bIsFullyOperational` remains a plain `bool` read cross-thread by the
  scan worker — pre-existing (the MIDI worker already read it), but an
  `std::atomic<bool>` would make it honest

## Pros and Cons of the Options

### Option 1 — synchronous constructor scan

* `+` No coordination surface at all; trivially deterministic
* `−` Every startup mode blocks on the full scan
* `−` The concurrent-access races remain unaddressed

### Option 2 — lazy scan on first access

* `+` Constructor stays cheap without introducing a thread
* `−` Moves an unbounded stall to an arbitrary first access, where the
  calling thread is unknown (potentially the audio path)
* `−` Cancellation and progress reporting get harder, not easier

### Option 3 — background worker with wait gates (chosen)

* `+` Startup unblocked in every mode; progress visible; waits are
  explicit, bounded to the scan duration, and confined to threads that
  can afford them
* `+` Interruptible teardown; no partial publications
* `−` A real coordination surface to maintain: worker lifecycle, wait
  gates, entry merge, and per-thread logger scopes (each verified by the
  unit suite: 484 tests, plus the static plugin suite)
