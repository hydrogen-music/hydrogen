---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Make Hydrogen a per-instance context; require multi-instance from day one

## Context and Problem Statement

A DAW loads several plugin instances **in one process**. Hydrogen's core is built
around process-global singletons that forbid this:

* `Hydrogen::__instance` is a single static pointer; the constructor *throws* if
  one already exists (`Hydrogen.cpp:115`, `:171`).
* `Preferences` (`Preferences.h:183`), `EventQueue` (`EventQueue.h:74`), and
  `Logger` are process singletons. `SoundLibraryDatabase` is **already** a
  per-`Hydrogen` leaf (reached via `Hydrogen::get_instance()->getSoundLibraryDatabase()`),
  not a singleton.

There are roughly **681 `get_instance()` call sites in `src/core` and 1,292 in
`src/gui`**. To run two groovebox instances side by side, all per-instance state
must be per-instance — **nothing engine-related stays shared between instances.**
We must choose how call sites reach their state once it is no longer global, and
when multi-instance support is required.

## Decision Drivers

* True multi-instance is non-negotiable for a DAW plugin — single-instance
  breaks in most common hosts.
* The refactor must be tractable: ~2,000 call sites is a lot of churn risk.
* The standalone application must keep working throughout.
* **No cross-instance state leakage.** Anything mutable that differs per
  instance — including the sound library view and the log stream — must be
  per-instance, not shared.

## Considered Options

1. **Hybrid — make the existing `Hydrogen` god-object the per-instance context**
   (back-pointers on hub classes, explicit references for new leaf classes;
   everything per-instance, nothing shared).
2. **Pure `EngineContext`** — a new container type owning all subsystems, with
   `Hydrogen` demoted to a member.
3. **Pure dependency injection** — thread explicit handles through every
   constructor and signature.

(Code sketches for all three are reproduced in
[proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md).)

## Decision Outcome

Chosen option: **Option 1 — Hydrogen as the per-instance context**, and
**multi-instance is a hard requirement from day one** (no single-instance
shipping milestone).

`Hydrogen` already aggregates `AudioEngine`, `MidiActionManager`, the current
`Song`, etc., so it is the natural per-instance container:

```cpp
// Hydrogen.h — static __instance / get_instance() removed; constructable N times
class Hydrogen {
public:
    Hydrogen(std::shared_ptr<Preferences> pPref, int nOscPort);   // was create_instance()
    AudioEngine*                 getAudioEngine() const { return m_pAudioEngine; }
    std::shared_ptr<Preferences> getPreferences() const { return m_pPreferences; }
    EventQueue*                  getEventQueue()  const { return m_pEventQueue; }
    Logger*                      getLogger()      const { return m_pLogger; }
    // SoundLibraryDatabase is already a per-Hydrogen leaf — kept that way.
private:
    std::shared_ptr<Preferences> m_pPreferences;   // per-instance, owned here
    EventQueue*                  m_pEventQueue;     // per-instance, owned here
    Logger*                      m_pLogger;         // per-instance, owned here
    AudioEngine*                 m_pAudioEngine;
};
```

* Hub classes (`AudioEngine`, `Sampler`, drivers) receive and store a
  back-pointer to their owning `Hydrogen` at construction.
* New leaf classes take explicit references to the dependencies they use
  (DI-style), so they stay testable.
* `X::get_instance()` lookups for `Hydrogen`/`Preferences`/`EventQueue` become
  `pHydrogen->getX()`.
* **`SoundLibraryDatabase` stays per-instance** — it is already a leaf of
  `Hydrogen`, so this is "leave it alone, do not promote it to a singleton."
  Promoting it would leak one instance's library edits/scan results into another.
* **`Logger` becomes a per-instance leaf of `Hydrogen` too** — but reached
  differently (ambient context), see its dedicated subsection below.

#### How instances reach objects

`get_instance()` is not uniformly distributed: in `src/core` it is ~95 % in
**hub/controller/engine** classes (`CoreActionController` 177, `AudioEngine` 79,
`MidiActionManager` 73, `OscServer` 54, `Hydrogen` 44, `JackDriver` 39, `Sampler`
29 …) and only ~26 calls across the **pure-data leaves** in `Basics/`
(`Note`, `Pattern`, `Sample`). So there are three mechanisms, by object kind:

1. **Hubs — constructor back-pointer, threaded down the ownership tree.** Each
   subsystem is created by its owner and handed the `Hydrogen*`, top-down from
   `Hydrogen`'s constructor (`Hydrogen` → `AudioEngine(this)` → `Sampler(this)` →
   …). Safe because these objects are owned by, and live as long as, their
   `Hydrogen`. They then use `m_pHydrogen->getPreferences()/getEventQueue()/…`.

2. **Data leaves — call-time parameters, not stored members.** The few leaf
   methods that call `get_instance()` today need something *specific*, not
   "Hydrogen" in general — e.g. `Note::humanize()`/`swing()` need the current
   `Song`; `Sample`'s rubberband paths need `Preferences`. These become explicit
   **method parameters**, supplied by the hub caller that already holds the
   context:

   ```cpp
   // before:  void Note::humanize();                 // Hydrogen::get_instance()->getSong()
   // after:   void Note::humanize( std::shared_ptr<Song> pSong );
   // caller (AudioEngine/Sampler — has the back-pointer):
   note->humanize( m_pHydrogen->getSong() );
   ```

   A `Note`/`Pattern`/`Sample` does **not** store a `Hydrogen`/`Preferences`/
   `EventQueue` — that would bloat the many data objects and hide the dependency;
   passing it keeps the dependency visible (testable) and uses the *current* value
   rather than a cached one.

3. **Most objects — nothing.** The overwhelming majority of data objects need
   none of these; the refactor doesn't touch them.

`EventQueue` pushes are almost all in the controller/engine tier (back-pointer);
a rare leaf push takes the queue as a parameter, or the emission moves up to the
controller where it belongs. The GUI's ~1,292 calls resolve against a single
injected `IEngineAccess` handle held by `HydrogenApp` (local engine in standalone,
IPC proxy in editor mode — [ADR 0016](0016-out-of-process-plugin-ui.md)), not
per-widget back-pointers.

#### Per-instance Logger (the special case)

Sharing one `Logger` across instances was rejected: all plugin instances **and** a
standalone app would write into one log file (serialised by the Logger's queue),
which (a) interleaves unrelated logs into an unreadable stream and (b) couples
their lifetimes — one shared queue must flush every instance's backlog at
shutdown, adding a bottleneck and event→log latency. Each `Hydrogen` therefore
owns its own `Logger` (own queue, own worker thread, own **per-instance log file**
— the path must be made unique per instance, e.g. by pid + an instance counter).

The catch: logging is wired **process-globally** today. `Base::__logger` is a
*static* `Logger*` (`Object.h:155`), set once via `Base::bootstrap()`, and every
`INFOLOG`/`DEBUGLOG` resolves through it (or via `Logger::get_instance()` in the
`__LOG_STATIC` path).

**Chosen mechanism — an ambient context, resolved at log time, used for logging
only:**

* A **thread-local "current `Hydrogen`" context**, pushed/popped by an RAII scope
  guard at the handful of instance-entry points (the audio `process()` callback,
  command dispatch, song/kit load, per-instance timers/workers). Setting the
  context is a few lines at those entry points — **not** a per-constructor change.
* The log macros change their logger source from the static `Base::__logger` to
  **`currentContext().logger()`**, resolved **at log time** — so there is **no new
  `Base` data member and no constructor churn**. A **process-default fallback
  `Logger`** (the role `Base::bootstrap()` already fills) covers static /
  pre-instance / unscoped contexts (`__LOG_STATIC`, early startup).
* The ambient context is used for the **logger only**. Behavioral dependencies
  (`Preferences`, `EventQueue`, `AudioEngine`) remain **explicit** (hub
  back-pointers + leaf DI, above). This deliberately confines the ambient
  ("service-locator") pattern to the one cross-cutting concern where it is
  idiomatic: a wrong or absent context can only misroute a *log line* to the
  fallback — it can never change behaviour or read another instance's state.

Rejected alternatives: capturing the context in a per-object `Base` member at
construction (more robust for cross-thread logging, but adds a pointer per object
and an always-on construction cost on hot paths like `Note`); and exposing
`context()` for general per-instance access (uniform and less boilerplate, but a
process-wide service locator — hidden dependencies, harder testing, and
wrong-context becomes a behavioural bug rather than a cosmetic log misroute).

Caveats: `Hydrogen` must outlive the objects that log through its context (or hold
the logger as `shared_ptr`); a worker thread servicing several instances wraps
each work item in its own scope; tests establish a scope or rely on the fallback.

Option 2 is rejected because it creates two overlapping container concepts
(`Hydrogen` *and* `EngineContext`) with an ambiguous boundary. Option 3 is
rejected for v1 as the largest and riskiest diff; individual leaf classes may
still adopt DI opportunistically.

"Multi-instance from day one" means the de-singletoning lands **before** host
integration is considered complete — there is no interim release that assumes a
single instance per process.

### Consequences

* A large, mostly-mechanical sweep across `src/core` and `src/gui` converting
  `get_instance()` chains to instance access. This is the dominant cost of the
  whole port and is shared by every plugin format.
* Per-instance lifetime: constructing/destructing a `Hydrogen` must fully set up
  and tear down `Preferences`/`EventQueue`/`AudioEngine` with no leaked global
  state. Multi-instance state-isolation testing is required (two instances must
  not cross-contaminate song, tempo, or events).
* The standalone app simply constructs exactly one `Hydrogen`, preserving current
  behaviour.
* Each instance gets its own `Logger` (own file) and its own
  `SoundLibraryDatabase`; nothing engine-related is shared between instances, so
  there is no cross-instance leakage to reason about. The cost is the
  `Base::__logger` rework (above) plus a process-default fallback logger for
  instance-less contexts.

## Amendment (2026-09-26): instance log routing, append mode, and process-default file content

Implementing the per-instance logger refined the mechanism above in three
ways:

* **Construction is a scope entry point — in practice the biggest one.** The
  illustrative entry-point list above is runtime-shaped; the largest misrouted
  chunk was instance *construction*: the `SoundLibraryDatabase` scan and the
  audio/MIDI driver startup inside `Hydrogen`'s constructor. The constructor
  now pushes a scope right after spawning the instance logger (covering the
  scan, OSC/NSM setup, and driver startup), and `EngineSession::serve()` —
  the command-dispatch site of the editor split
  ([ADR 0016](0016-out-of-process-plugin-ui.md)) — pushes one around its
  serve loop, so IPC dispatch, song/kit loads, and telemetry publishing land
  in the engine instance's file.
* **Plugin-mode log files open append-only, with a per-write file lock.** A
  pid + instance-counter path is unique within a session but not *across*
  sessions: hosts and the OS reuse PIDs, so a later run can reopen a prior
  run's path — truncating there would destroy the earlier session's log.
  Plugin-mode loggers (the per-instance files and the process-default
  `hydrogen_<pid>_plugin.log` alike) therefore open with
  `QIODevice::Append`, and each write takes a best-effort whole-file lock
  (`flock`/`LockFileEx`) so concurrent appenders cannot tear each other's
  lines. The lock is coupled to append mode: an exclusive (truncating)
  logger — the standalone app — keeps lock-free writes.
* **The process-default file carries process-level lines only.** With the
  construction and serve-loop scopes in place, all instance work lands in
  instance files; the shared per-process file keeps process-level messages
  plus the unscoped `Spawning instance logger` announcements, so it stays
  small and doubles as an index of the session's instance files. Mirroring
  instance work into the shared file as well was rejected — it would
  re-create the interleaved, unreadable stream the per-instance split exists
  to avoid.

## Amendment (2026-09-27): full routing coverage and the `[unscoped]` audit surface

Closing the remaining routing gaps and making future ones visible:

* **Thread bodies and object announcements route through the ambient scope.**
  The `__LOG_OBJ` family (thread-body macros) and `Object`'s
  constructor/destructor announcements previously resolved through the static
  `Base::__logger` — the process default — so work running *inside* an
  instance scope on a fresh thread bypassed it. They now resolve through
  `currentLogger()` like every other macro.
* **Teardown is scoped.** `~Hydrogen` pushes a scope around its whole
  teardown body (member resets inside it, the logger delete outside), so
  destructor-time work — including release-build `Filesystem::rm` logging —
  lands in the instance file instead of leaking to the process default after
  the constructor's scope popped.
* **The host entry surface is scoped.** `HydrogenPlugin`'s public methods —
  the surface the format shims (CLAP, LV2, VST3) forward host calls into —
  push a scope at entry, so main-thread engine work (state load, transport,
  MIDI queuing) routes to the instance logger. The test harness
  (`FakePluginHost`) mirrors the same per-method scopes.
* **Every per-instance thread is scoped at entry.** Threads serving one
  instance for their whole life (TimeHelper burn-in, MidiActionManager
  worker, export plan, all audio/MIDI driver threads, NSM) push a single
  scope at thread-body entry; externally-dispatched callbacks (OSC handlers,
  JACK non-process callbacks) push one per invocation. The process-level
  threads — each logger's own writer, the IpcServer accept loop — stay on
  the process default by design.
* **`[unscoped]` audit surface.** While any per-instance logger is alive,
  `Logger::log()` marks lines reaching the process default `[unscoped] `, so
  routing gaps stay greppable instead of silently blending into
  process-level output. Two tripwire tests (harness lifecycle and
  `HydrogenPlugin` lifecycle) assert the spawning announcement is the *only*
  marked line of a full lifecycle. During development this surface caught
  two real gaps (the harness's main-thread driving window; unscoped
  `HydrogenPlugin` host-entry lines) and one latent `Filesystem` quirk: the
  suite must read the process default via `Logger::getLogFile()` because
  `Filesystem::logFilePath()`'s lazy first-call resolution can clobber the
  bootstrapped custom path.

**Base-capture re-examined and held as a named escalation.** Capturing the
logger per object at construction (so even unscoped calls route correctly)
was re-examined under the multi-instance premise. The original rejection's
"misroute is only cosmetic" assumption no longer holds — with several
instances in one process a misroute writes *another instance's* log — but
the cost half stands (a per-object pointer on hot paths like `Note`). It is
recorded as an escalation with prerequisites, not the default:

* **F4 — construction order:** `Hydrogen` itself, the `EventQueue`, and
  `Preferences` are born before any scope exists; Base-capture would freeze
  the process-default pin on them. Escalating requires constructor
  reordering or setter discipline for those objects.
* **F5 — teardown:** the owning `delete` of an instance logger would turn
  late misroutes into use-after-free. Escalating requires a zombie protocol
  (the logger stays allocated and flag-drains its queue) — *not*
  `shared_ptr`, whose final release could happen on a real-time thread
  mid-flight.
* **Trigger:** a plugin host multiplexing multiple instances on one OS
  thread via stack-switching fibers, which breaks TLS-based ambient routing.
  No known host does this (Linux threading is strictly 1:1; Windows fibers
  are cooperative; CLAP's symbolic audio thread may migrate between OS
  threads, which the per-call scopes above already handle).

## More Information

* Call-site sketches and migration steps:
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
* Related: [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md),
  [ADR 0016](0016-out-of-process-plugin-ui.md)
