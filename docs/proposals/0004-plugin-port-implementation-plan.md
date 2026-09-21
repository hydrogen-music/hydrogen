# Proposal 0004 — Plugin port: test-driven implementation plan

Status: accepted (plan) — 2026-06-08 — author: pm

This is the detailed, **test-driven** implementation plan for the plugin port
designed in [proposal 0003](0003-hydrogen-as-an-audio-plugin.md). It refines
proposal 0003 §6 into concrete, sequenced tasks, each gated by tests written
*before* the implementation. Decisions are recorded in ADRs
[0013](/docs/decisions/0013-provide-hydrogen-as-an-audio-plugin.md)–[0023](/docs/decisions/0023-concurrency-safe-config-persistence.md);
the test catalogue is proposal 0003 §11.

---

## 1. How we work (TDD methodology)

Every task follows **red → green → refactor**:

1. **Red** — write or extend the CppUnit test(s) that specify the behaviour
   (`src/tests/`, registered in `registeredTests.h`). It must fail (or fail to
   compile against the intended API) for the right reason.
2. **Green** — write the minimum production code to pass.
3. **Refactor** — clean up with the test as a safety net.

Rules that make this work for a port of this size:

* **The existing ~30-suite test set is the regression net.** It must stay green
  at *every* commit. For the large mechanical refactor (Phase 1–2) this is the
  primary safety mechanism; the new spec tests describe the *added* behaviour.
* **Tests run on all four CI platforms** via `src/tests/tests --appveyor`
  (proposal 0003 §10–§11). Core-level tests need no host. Process/host-level tests
  go behind `WANT_INTEGRATION_TESTS` (Linux + debug).
* **A task is "done" only when its spec test passes AND the full suite is green**
  on the CI matrix.
* New tests are added as a `*Test.h/.cpp` pair + one
  `CPPUNIT_TEST_SUITE_REGISTRATION` line.
* Shared test infrastructure (`FakePluginHost`, see Phase 0) is built first so
  later behaviour can be specified before any real plugin binary exists.

**Definition of Done (per phase):** spec tests green on all platforms (integration
tests green on Linux); existing suite green; standalone app still builds and runs;
ADRs/proposals updated if the design shifted.

---

## 2. Dependency graph & critical path

```
PR Remove LADSPA/LRDF/FX ─► P0 Test harness ─┐
   (ADR 0024, prerequisite)                  ├─► P1 De-singleton ─► P2 GUI per-instance ─┐
                                             │        (ADR 0015)      (regression gate)  │
                                             │                                           ▼
                                             └──────────────────────► P3 Host seams ─► P4 Buses + State + CLAP/LV2
                                                                        (ADR 0013)        (ADR 0019/0017/0020/0014)
                                                                                                │
                                                                                                ▼
                                                         P5 Editor mode + IPC + layered config
                                                            (ADR 0016/0018/0022/0023)
                                                                                                │
                                                                                                ▼
                                                         P6 VST3 + packaging ─► P7 Hardening (+ T7.1 remap widget)
                                                            (ADR 0014, §9)        (ADR 0015/0019)
```

**Critical path: PR → P1 → P2 → P3 → P4 → P5.** PR (removing LADSPA/LRDF/FX) runs
**first** so later phases never refactor code that is being deleted; P1
(de-singletoning) gates everything multi-instance; P5 (editor/IPC/config) is the
second-largest block. P6 is marginal on top of P4's CLAP. Estimates mirror
proposal 0003 §6 (~5–7 months total); PR is net-negative effort (deletion that
shrinks every later phase).

---

## 3. Phase R — Remove LADSPA/LRDF/effect hosting (prerequisite) — ADR 0024

**Status: ✅ DONE** (2026-06-11)

*Objective:* delete the in-app effect-hosting subsystem before the port begins, so
no later phase refactors code that is being removed. Net-negative effort.

> **1.2.7 (separate, on the maintenance branch):** mark LADSPA/LRDF effect hosting
> deprecated — release notes + a user-facing hint, no code removal. This precedes
> and is outside the 2.0 plan; the removal below is the 2.0 half.

**Tests first**
* The **existing suite is the spec**: after removal it must stay green. Remove or
  adjust any FX-touching expectations (e.g. in `AudioEngineTest`), and add an
  `XmlTest` case that an old song containing `<fx>` nodes **loads cleanly with the
  section ignored** (formatVersion bumped, per
  [ADR 0001](/docs/decisions/0001-introduce-formatVersion-to-xml-files.md)).

**Tasks**
* **TR.1** Delete core FX: `src/core/FX/` (`Effects`, `LadspaFX`); remove
  `Song::getEffects()` and `<fx>` serialization (bump `formatVersion`; ignore
  `<fx>` on load).
* **TR.2** Remove the `#ifdef H2CORE_HAVE_LADSPA` paths from the audio engine
  (`AudioEngine::setupLadspaFX`, `m_fLadspaTime`, the `processAudio` FX loop) and
  `Sampler.cpp:1828`.
* **TR.3** Delete GUI FX: `LadspaFXProperties`, `LadspaFXSelector` (+ `.ui`) and
  their mixer/menu entry points.
* **TR.4** Remove `WANT_LADSPA`/`WANT_LRDF`, `FindLadspa`, the LRDF probe, and the
  status-list entries from CMake; drop `ladspa-sdk`/`liblrdf-dev` and
  `-DWANT_LADSPA`/`-DWANT_LRDF` from `.appveyor.yml`.

**Done when:** the standalone app builds and runs with no LADSPA/LRDF/FX code,
options, or CI deps; full suite green (incl. the old-song `<fx>`-ignored case); no
`H2CORE_HAVE_LADSPA`/`LRDF` symbols remain.

*(Placement note: PR may instead be done last, but as a prerequisite it shrinks
the de-singletoning sweep and removes two build deps before the plugin toolchain
is added — the recommended order.)*

---

## 4. Phase 0 — Test harness & build scaffolding

**Status: ✅ DONE** (2026-06-11)

*Objective:* be able to specify engine/plugin behaviour in tests before plugins
exist; stand up the build options without changing the standalone build.

**Tests first**
* `FakePluginHostTest` — trivial: construct the new fixture, run zero frames,
  assert clean teardown.

**Tasks**
* **T0.1 `FakePluginHost` fixture** (`src/tests/utils/`). A non-audio harness
  modelled on `FakeAudioDriver` that supplies: output buffers, a settable
  transport state (frame/BBT/tempo/playing), a MIDI event list with sample
  offsets, and a configurable sample rate / block size. It drives one engine
  instance's process callback directly. *Grows* through later phases.
* **T0.2 CMake scaffolding** — add options `WANT_LV2`, `WANT_CLAP`, `WANT_VST3`
  (all default `OFF`) and `H2_PLUGIN_OUTPUT_BUSES` (default `32`) to the top-level
  `CMakeLists.txt`; create `src/plugin/` with a placeholder; vendor `clap` and
  `lv2` headers and add `clap-wrapper` as a submodule (not yet built).
* **T0.3 CI toggle** — keep all plugin builds off by default so per-commit CI is
  unchanged; document how to enable them.

**Done when:** `FakePluginHostTest` passes on all platforms; a default build is
byte-for-byte the same standalone app; enabling the new options configures
cleanly (even if targets are stubs).

---

## 5. Phase 1 — Per-instance engine (de-singletoning) — ADR 0015

*Objective:* multiple `Hydrogen` instances coexist in one process.
*This is the critical path and the largest single block.*

**Status: ✅ DONE** (2026-06-15) — all of T1.1–T1.6 landed and the Phase-1
"Done when" gate is met: `MultiInstanceTest`, `PluginLifecycleTest`, and
`LoggerInstanceTest` pass; full suite green (`OK (227 tests)` + the GUI startup
smoke test); no `Hydrogen/Preferences/EventQueue/Logger::get_instance()` remain
in `src/core` except the documented process-default logger fallback. The
standalone app builds, runs, and tears down leak-free. **`OscServer` and
`NsmClient` were also converted from process-global singletons to per-instance
members owned by `Hydrogen`** (back-pointer + `getOscServer()`/`getNsmClient()`,
created in the ctor / freed in the dtor): the static liblo callbacks reach their
owning instance via the registration binding (a `this`-capturing `addMethod`
helper) and liblo `user_data` (the catch-all `generic_handler`, the NSM
open/save callbacks). Two live instances therefore each own an independent OSC
server / NSM client; only one binds an OSC port at a time, which is a runtime
config concern (port/`getOscServerEnabled`), not a shared-ownership one. This
brings forward the OSC/NSM part of ADR 0026; the remaining plugin-mode feature
disablement stays Phase 3. `MultiInstanceTest` constructs two real instances and
asserts independent song/tempo/`Preferences`/`EventQueue`.

**Tests first (the spec)**
* `MultiInstanceTest` — construct two `Hydrogen` instances with the target API
  (`Hydrogen(std::shared_ptr<Preferences>, oscPort)`, `getAudioEngine()`,
  `getPreferences()`, `getEventQueue()`); assert independent song/tempo/transport
  and `EventQueue`; mutating one never affects the other. *Will not compile until
  the API exists — that is the red.*
* `PluginLifecycleTest` — construct → use → destruct, repeated N times in one
  process, with no residual global state (pairs with `MemoryLeakageTest`).

**Tasks**
* **T1.1** Make `Preferences` instance-ownable (`Preferences.h:182`); retain a
  transitional `get_instance()` returning a process-current pointer so unconverted
  code keeps compiling.
* **T1.2** Make `EventQueue` instance-ownable (`EventQueue.h:74`), same transition
  shim.
* **T1.3** Remove the `Hydrogen` "already running" throw (`Hydrogen.cpp:115`); add
  the instance constructor; have `Hydrogen` own its `Preferences`+`EventQueue`
  (+`Logger`, T1.6); `create_instance()` becomes a thin standalone helper that
  constructs one.
* **T1.4** Mechanical sweep of `src/core` (~681 `get_instance()` sites), by object
  kind ([ADR 0015](/docs/decisions/0015-per-instance-engine-context.md) "How
  instances reach objects"):
  * **Hubs/controllers/engine/drivers** (~95% of calls — `CoreActionController`,
    `AudioEngine`, `MidiActionManager`, `OscServer`, `JackDriver`, `Sampler`, …):
    give each a **back-pointer to its owning `Hydrogen`**, set at construction and
    threaded top-down from `Hydrogen`'s ctor; convert lookups to
    `m_pHydrogen->getX()`.
  * **Pure-data leaves** (`Basics/` — `Note`, `Pattern`, `Sample`; ~26 calls): the
    few methods that call `get_instance()` take the **specific dependency as a
    method parameter** (e.g. `Note::humanize(std::shared_ptr<Song>)`,
    `Sample` rubberband paths take `Preferences&`), supplied by the hub caller.
    **Do not** add a stored `Hydrogen`/`Preferences`/`EventQueue` member to data
    classes — keep them plain data and the dependency visible in the signature.
  * **Everything else**: untouched (needs none of these).
  * `EventQueue` pushes live in the controller/engine tier (back-pointer); a rare
    leaf push takes the queue as a parameter or moves up to the controller.
  * **`SoundLibraryDatabase` stays per-instance** (already a `Hydrogen` leaf — do
    *not* promote it to a singleton; that would leak library state between
    instances).
  * **Status: ✅ DONE** (2026-06-12) — `src/core` production sweep complete,
    full suite green throughout; **637 → ~51** `get_instance()`. Hubs/controllers/
    engine/drivers carry an owning `m_pHydrogen` back-pointer; pure-data leaves
    take the specific dependency as a parameter; the static deserialization/load
    tree, `Transport` frame↔tick math, and `Filesystem` context helpers are
    threaded. The audio **process callback** now receives its owning `Hydrogen`
    through the driver's `void*` arg (all drivers that invoke it pass their
    instance), so each instance's callback drives its own engine — a defensive
    `get_instance()` fallback remains for unforeseen/direct calls, dropped in
    T1.5. The remaining `src/core` `get_instance()` are the **transitional shims
    (removed in T1.5)**, the per-method `AudioEngineTests` seams (await the
    test-instance strategy), and the cosmetic `AE_*LOG` driver-name prefix.
* **T1.5** Remove the transitional shims once the sweep is complete; the build
  failing on a lingering `get_instance()` is the proof it is gone.
  * **Status: ✅ DONE** (2026-06-15) — the `Hydrogen` / `Preferences` /
    `EventQueue` registry (`__instance` + `get_instance()` + `setInstance()` +
    `replaceInstance()` + the registering `create_instance()`) is deleted;
    whole-tree `get_instance()` for those three is **0**. The de-singletoning
    was finished in coordinated groups: static-audio math now threads the
    instance (`Transport::compute*`, `getLeadLagInFrames`, Jack timebase);
    `AE_*LOG`/`assertLocked` route through `this`/a thread-local locked-engine;
    `audioEngine_process` takes the owning `Hydrogen` via its callback arg (P3);
    the load tree (`Song`/`Drumkit`/`Instrument`/`InstrumentLayer`/`Sample`/
    `Pattern`/`Playlist`/`Legacy`/`Filesystem`/`getEmptySong`/`from`/the
    `*Info` loaders) makes the instance a required parameter; `AudioEngineTests`
    takes a harness-injected fixture pointer; `Preferences::load`/`Shortcuts`
    thread it; the standalone factory is now `Hydrogen::create_instance(nOscPort,
    pPreferences)` returning a caller-owned instance; the GUI injects the engine
    + preferences via `HydrogenApp::setBootstrap()` (single-instance, GUI-local,
    not a core singleton). The instance-mechanism tests that exercised the shim
    (`EventQueueTest::testProcessCurrent`, `PreferencesInstanceTest::
    testProcessCurrent`) were removed; the independence/ownership tests remain.
    Green via `build.sh t`: `OK (225 tests)` + `GuiStartup` passed.
* **T1.6 Per-instance `Logger`** via an **ambient context, resolved at log time,
  logging-only** ([ADR 0015](/docs/decisions/0015-per-instance-engine-context.md)).
  Make `Logger` instance-ownable (owned by `Hydrogen`), each with its own queue,
  worker thread, and **unique per-instance log file** (pid + instance counter).
  Add a **thread-local "current `Hydrogen`" context** with an RAII scope guard set
  at the instance-entry points (audio `process()`, command dispatch, song/kit
  load, per-instance timers/workers). Redefine the log macros to read
  `currentContext().logger()` instead of the static `Base::__logger`
  (`Object.h:155`), with a **process-default fallback** for unscoped/static
  contexts — **no `Base` data member, no constructor changes**. The ambient
  context is used for the logger only; behavioral deps stay explicit (T1.4).
  *Tests first:* `LoggerInstanceTest` — within distinct context scopes two
  instances log to two distinct files with no cross-writing; an unscoped log call
  hits the fallback; teardown of one instance flushes only its own queue.
  * **Status: ✅ DONE** (2026-06-12) — `Logger` is instance-ownable
    (`Logger::createInstanceLogger()`: own queue, worker thread, and per-instance
    file `hydrogen_<pid>_<counter>.log`; does not touch the singleton); `Hydrogen`
    owns one via `getLogger()`, mirroring the default logger's stdout/colours. The
    ambient context is `Logger::currentLogger()` + an RAII `Logger::Scope`
    (thread-local), with the bootstrap logger as the process-default fallback; the
    log macros (`Object.h`) resolve through it at log time — no `Base` data member,
    no ctor churn. `Logger::Scope` is set at the audio `process()` entry point
    (instance reached via the callback arg); the remaining entry-point scopes
    (command dispatch, GUI workers) land with Phase 2. `LoggerInstanceTest` passes
    (3 cases), and **`Logger::get_instance()` is eliminated from `src/core`**. Also
    fixed a latent worker-loop race that skipped draining a short-lived logger's
    queue on teardown.

**Risk control:** land T1.1–T1.6 incrementally, suite green at each commit. The
shim lets the refactor proceed file-by-file. T1.6 is well-contained by the
resolve-at-log-time choice (macro redefinition + an RAII scope at ~5 entry points,
not a per-class change), but verify it broadly since the macros touch every class.

**Done when:** `MultiInstanceTest` + `PluginLifecycleTest` + `LoggerInstanceTest`
pass; full suite green; no `Hydrogen/Preferences/EventQueue/Logger::get_instance()`
remain in `src/core` except the documented process-default logger fallback.

---

## 6. Phase 2 — GUI per-instance & standalone regression

*Objective:* the existing GUI drives a *specific* engine instance via an injected
handle — the seam that later lets editor mode swap a local engine for an IPC proxy.

**Status: ✅ DONE** (2026-06-16) — `EngineAccessTest` passes; full suite green
(`OK (226 tests)`); standalone GUI builds, starts, and tears down unchanged
(GUI startup smoke test + leak check clean). Delivered: **`IEngineAccess`**
(`src/core/IEngineAccess.h`) — the surface the GUI reads state and issues commands
through, mirroring the `Hydrogen` methods the GUI fans out from (commands via
`CoreActionController`, events via `EventQueue`, live state via getters); and
**`LocalEngineAccess`** (`src/core/LocalEngineAccess.h`) wrapping a local
`Hydrogen`. `HydrogenApp` owns the handle (created in `setBootstrap()` once the
engine exists) and exposes it via `HydrogenApp::pEngine()`. The GUI's **366
state/command call sites** (`pHydrogen()->…`) now go through `pEngine()->…`
(`IEngineAccess`). **Scope boundary (deferred to P5/ADR 0016+0018):** ~200 GUI
sites still pass the concrete `Hydrogen*` to core file-load/utility APIs
(`Song::load`, `Filesystem::DetermineContext`, `Instrument::from`, …); these are
direct-engine operations that editor mode routes through IPC commands, so they
remain on `HydrogenApp::pHydrogen()` — which still returns the GUI's single
*injected* per-instance engine (P1 removed the global), not a singleton. Since
the engine is already injected, P2's "GUI drives a specific instance, not a
global" objective holds for every site; the `IEngineAccess` indirection is what
P5 needs to swap a local engine for an IPC proxy. **Regression gate cleared.**

**Tests first**
* Existing GUI-touching tests (`CliTest`, etc.) remain the spec; add
  `EngineAccessTest` — a `LocalEngineAccess` wrapping a `Hydrogen` exposes the
  state/command surface the GUI needs (transport, song, peaks, events).

**Tasks**
* **T2.1** Define an **`IEngineAccess`** interface (the surface the GUI reads
  state and sends commands through — backed by `CoreActionController` +
  `EventQueue` + live getters). Implement `LocalEngineAccess` over a local
  `Hydrogen`.
* **T2.2** Inject the access handle into `HydrogenApp`/`MainForm`; convert the
  ~1,292 GUI `get_instance()` sites to go through it.
* **T2.3** Standalone `main.cpp` constructs one `Hydrogen` + `LocalEngineAccess`
  and injects it (replacing the implicit singleton wiring at `main.cpp:424`).

**Done when:** standalone app builds and runs unchanged on the refactored core;
`EngineAccessTest` passes; full suite green. **Regression gate** before any plugin
UI work.

---

## 7. Phase 3 — Plugin host seams (audio / MIDI / transport) — ADR 0013

**Status: ✅ DONE** (2026-06-16) — `PluginAudioDriver`, `PluginMidiDriver`, the
host-transport follower, and the plugin-mode feature gate all landed with their
test groups (`PluginProcessTest`, `PluginMidiTest`, extended `TransportTest`,
`PluginFeatureGateTest`). Full CppUnit suite green (`OK (251 tests)`); standalone
unaffected. T3.4's build-level source exclusion is recorded in
`src/plugin/CMakeLists.txt` for the Phase-4 plugin target (the features are
already runtime-inert under a plugin host).

*Objective:* the engine runs from host-provided buffers, MIDI, and transport.

**Tests first**
* `PluginProcessTest` (`FakePluginHost`): arbitrary/odd `nframes`, mid-stream
  block-size and sample-rate changes (tick size recomputed), render identical
  across block boundaries; silence in → exact silence out; no NaN/denormals.
* extend `TransportTest`: playhead tracks host position sample-accurately; tempo,
  start/stop, relocate, loop all follow the host; with a timeline present, host
  tempo wins; relocate/loop neither hangs nor wrongly cuts voices.
* `PluginMidiTest` (+ extend `MidiNoteTest`/`MidiActionTest`): host note-on/off →
  right instrument/velocity; CC/program → mapped actions; events land at their
  sample offset.

**Tasks**
* **T3.1 `PluginAudioDriver : AudioDriver`** (`src/core/IO/AudioDriver.h:37`):
  `getOut_L/R()` point at host buffers; `init/connect` no-ops; host `process()`
  drives `audioEngine_process()` (`AudioEngine.cpp:1743`).
* **T3.2 `PluginMidiDriver : MidiBaseDriver`** (`MidiBaseDriver.h:45`): inject
  host MIDI events (with sample offsets) into the existing input queue.
* **T3.3 Host-transport follower:** add a host-transport tempo source; in the
  process path drive `m_pPlayhead`/`m_pQueuing` from host position/tempo/state
  instead of `incrementPlayhead()` / JACK Timebase-master logic. Reuse the
  existing frame↔tick math.
* **T3.4** Exclude `DiskWriterDriver` and NSM/JACK-session from the plugin build
  path. (The LADSPA/LRDF effect host is already gone — removed in Phase R.)
* **T3.5 Disable conflicting core features in plugin mode** ([ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)).
  Introduce a single **"running as plugin" predicate** (reachable from core and,
  via the engine-access handle, the GUI) and gate off: **OSC server**, **NSM**,
  **Timeline**, **playlist MIDI actions/commands** (filtered from
  `MidiActionManager`), **Tap Tempo**, **Beat Counter**, **MIDI clock** (in/out);
  and **force loop state ON** (non-togglable). *Tests first:* `PluginFeatureGateTest`
  — with the predicate set, these features are inert (no OSC bind, playlist MIDI
  actions are no-ops, loop reads on and cannot be turned off), and standalone is
  unaffected when the predicate is clear; extend `MidiActionTest` for the filtered
  actions and `TransportTest` for forced-loop.

**Done when:** the three test groups + `PluginFeatureGateTest` pass on all
platforms; standalone unaffected.

---

## 8. Phase 4 — Buses, state/samples, native CLAP & LV2

Three sub-blocks; each is an independent red→green loop. ADRs 0019, 0017, 0020,
0014.

### 8a. Output buses — ADR 0019
**Status: ✅ DONE** (2026-06-16) — `OutputBusTest` (4 cases) green; full suite
`OK (255 tests)`. Host bus buffers live on `PluginAudioDriver`
(`setBusBuffers`/`getBusBuffer_L/R`/`clearBusBuffers`); the sampler routes each
instrument pre-fader to the bus matching its kit index (surplus → master only),
master keeps the full post-fader sum.

**Tests first:** `OutputBusTest` — the **default 1-to-1 mapping** sends the first
N instruments to buses 1…N (pre-fader); master carries the full sum;
surplus/unmapped instruments (kits with > N) route to **master only**; nothing is
*automatically* summed onto a shared bus; active bus count equals
`H2_PLUGIN_OUTPUT_BUSES`; on CLAP/VST3 the bus label tracks the assigned
instrument name and the rename hook fires on kit change.
**Tasks:**
* **T4a.1** Bus-buffer provider analogous to `JackDriver::getTrackBuffer`, fed by
  the host's bus buffers; reuse the sampler's existing dual-write (per-track +
  master) at `Sampler.cpp:1720`.
* **T4a.2** Default 1-to-1 mapping (first N instruments → buses by list order);
  surplus/unmapped instruments to master only; **no automatic sharing**. No custom
  mapping or routing UI yet (that is late-stage, §11/T7.1).
* **T4a.3** Bus naming on CLAP/VST3: name each bus after its assigned instrument
  and emit the per-format rename on kit change — CLAP
  `host.audio_ports.rescan(RESCAN_NAMES)` (allowed while active), VST3
  `restartComponent(kIoTitlesChanged)`. LV2 keeps build-time `Out 1…N` labels in
  the generated `.ttl`.

### 8b. State, samples & `.h2project` — ADR 0017 / 0020 / 0025
**Status: ✅ DONE (core)** (2026-06-16) — T4b.1/2/3/5 landed and tested; full
suite `OK (262 tests)`. T4b.4 (standalone *menu* action + Open-dialog filter) is
the only remainder and is pure GUI wiring on top of the finished codec —
deferred to a GUI session (not unit-testable here).
* **T4b.1** `Sample::loadFromMemory()` decodes encoded bytes via libsndfile
  virtual I/O, sharing the decode back end with `load()`
  (`SampleTest::testLoadFromMemory`: in-memory decode bit-identical to disk).
* **T4b.2** `H2Project` codec (`src/core/Helpers/H2Project.{h,cpp}`): in-memory
  libarchive bundle (song XML + content-hash-deduped sample blobs + an
  ordinal manifest), built and reconstructed entirely in memory; file
  save/load too. `H2ProjectTest` (buffer + file round-trip, container detect).
* **T4b.3** Unified open `H2Project::openSong()` (peeks the container, defers to
  `Song::load` or `H2Project::load`) + `H2ProjectTest::testUnifiedOpen`.
* **T4b.5** Plugin embed toggle `H2Project::toState(embed)/fromState()`
  (ON → portable bundle, OFF → song-only XML) + `PluginStateTest`.

**Tests first:** `H2ProjectTest` (song+kit → `.h2project` bundle → reconstructs
identically, incl. bus mapping); extend `SampleTest` (memory-decoded sample
bit-identical to file-loaded; content-hash dedup); **unified open** (one endpoint
loads both `.h2song` and `.h2project`, detected by container — extend `XmlTest`);
`PluginStateTest` (embed toggle: ON → portable `.h2project` state, OFF → song-only
state, both reload); state `formatVersion` compatibility; save/load entirely off
the audio thread.
**Tasks:**
* **T4b.1** In-memory `Sample` load via libsndfile virtual I/O (`sf_open_virtual`),
  alongside the existing file path (`Sample.cpp:215`).
* **T4b.2** `.h2project` codec ([ADR 0025](/docs/decisions/0025-h2project-self-contained-format.md)):
  assemble the libarchive bundle (song XML + unique, hash-deduped samples + image)
  and reconstruct from memory; reuse `Drumkit` libarchive code (`Drumkit.cpp:737`).
* **T4b.3** Unified open: one endpoint detects `.h2song` (XML) vs `.h2project`
  (archive) and loads accordingly; the plugin state reader accepts both song-only
  and full-bundle states.
* **T4b.4** Standalone menu action to create a `.h2project`; Open dialog accepts it.
  (Standalone-only; depends on the codec, not on the plugin — can land as soon as
  T4b.2 is in.)
* **T4b.5** Plugin "store drumkit samples in plugin state" preference toggle
  (default ON): ON writes a `.h2project` bundle, OFF writes song-only state.

### 8c. Native CLAP + LV2 — ADR 0014
**Status: ✅ DONE** (2026-06-17) — SDKs vendored under `extern/` (`clap`, `lv2`)
and both native plugins build and pass conformance:
* A format-agnostic `HydrogenPlugin` core (`src/plugin/HydrogenPlugin.{h,cpp}`,
  always built) wraps a headless engine on the Phase-3 seam (host buffers +
  transport + MIDI), the 8a output buses, and the 8b `.h2project` state codec.
  Verified in-process by `PluginLifecycleTest` (process silence/notes into
  master + buses with no NaN; state save/load round-trip; repeated
  construct/destruct leaves no residual objects) — runs in the normal suite.
* **CLAP** (`src/plugin/clap/HydrogenClap.cpp`, `WANT_CLAP`): master + N bus
  audio ports, MIDI note port, transport following, state save/load, no params
  (ADR 0021). **`clap-validator validate` passes: 11 passed / 0 failed** (10
  `clap.params` tests skipped by design). Wired as the `clap-validate` CTest.
* **LV2** (`src/plugin/lv2/HydrogenLv2.cpp`, `WANT_LV2`): same seam; the
  `hydrogen.ttl` (master + N bus ports + atom MIDI in) is generated from
  `H2_PLUGIN_OUTPUT_BUSES` at configure time. Verified by an in-process LV2 host
  (`lv2_smoke`, the `lv2-smoke` CTest); `lv2lint` is wired conditionally for CI
  where meson/lilv are present.

**Tests first:** per-format validator smoke step in CI (`clap-validator`,
`lv2lint`/`lv2_validate`): instantiate, activate, process silence, no crash;
extend `PluginLifecycleTest`.
**Tasks:**
* **T4c.1** Native CLAP plugin over the Phase 3 seam: audio ports (master + N
  buses), note ports, transport, state save/load via the 4b bundle codec, no
  params (ADR 0021).
* **T4c.2** Native LV2 wrapper sharing the same seam; generate the `.ttl` from
  `H2_PLUGIN_OUTPUT_BUSES` at build time, plus a `stereo` variant.

**Done when:** all 7a/7b tests green on all platforms; CLAP & LV2 pass their
validators in CI.

---

## 8.4 Phase 4.4 — Object-level instance identity (baseline for the write-surface sweep)

[ADR 0028](/docs/decisions/0028-object-instance-identity.md).

**Status: ✅ DONE** (2026-06-19) — T4.4a/b/c landed; full CppUnit suite green
(`OK (299 tests)`, +7 `ObjectUuidTest`) and the ctest gate (GuiStartup +
CLAP/LV2 conformance) passes. `Object`/`Base` now carries an immutable
`Uuid m_uuid` (`{epoch, counter}`: epoch a one-off off-RT `random_device` draw
via a Meyers singleton, counter a wait-free `fetch_add`), minted in every
constructor incl. copy, preserved under assignment; a `static_assert` pins the
counter lock-free for the note path. Identity comparisons route through one
`sameObject(a,b)` helper (death row, note queues in `AudioEngine`/`Sampler`,
`Hydrogen` MIDI-record, `Pattern::removeNote`/`references`/`purgeInstrument`/
`virtualPatternsDel`, `InstrumentList::add`/`insert`); `index`/`del` gained
uuid overloads (positional return unchanged) on `InstrumentList`/`PatternList`/
`Instrument`/`InstrumentComponent` with the pointer overloads delegating. The
latent cross-kit `Id` match in `Sampler::midiKeyboardNoteOff` is fixed to use
identity. **Two ADR-listed sites were left unchanged on inspection — they are
not pointer-identity:** `Drumkit::addInstrument` (`Drumkit.cpp:597`) is a
deliberate `Instrument::Id`-*uniqueness* check (uuid would defeat duplicate-id
detection), and `Playlist::remove` matches by a *value* `operator==`
(song/script path), not by pointer. (ADR 0028 updated accordingly.)

**Original problem statement.** Core objects are identified by raw `shared_ptr`
pointer comparison across many `Basics` classes (note queue, death row,
`InstrumentList`/`PatternList`/`Instrument`/`InstrumentComponent` `index`,
`Playlist::remove`, `Pattern::removeNote`/`references`/`purgeInstrument`).
Pointers don't cross the process split, and the only value fallback today —
`Instrument::Id` — is kit-unique (zero-based), so the site already matching
by `getId()` (`Sampler.cpp:427`) is a **latent bug** (old-kit
death-row instruments mis-match the new kit during a drumkit switch). Deciding
*per site* whether a comparison is index-safe or needs a value id is a footgun.

This phase adds **one uniform instance identity on the `Object` base** —
immutable `m_uuid` + `getUuid()` for every derived class — and routes all
identity comparisons through it. Backing is a **process-tagged atomic counter**
(`{epoch, counter}`), **not** `QUuid::createUuid()`: `Note` is an `Object` copied
on the audio thread (`AudioEngine.cpp:1693,3023,3137,3188`, `Sampler.cpp:1987`),
and `createUuid()`'s shared synchronised generator is an RT/xrun hazard, whereas
a wait-free `fetch_add` is RT-safe (~ns; ~16 B/object, <0.1 % of sample PCM).
Minted on **every construction including copy**, so each object (incl. queued
note copies and instrument clones) is a distinct identity — exactly pointer
semantics as a value, which also gives instruments their distinct-clone identity
for free (death-row correctness) with no per-class special-casing. Runtime-only
(not serialised → no file-format/`XmlTest` churn; behaviour-preserving since a
unique id makes `getUuid()==` exactly `ptr==`). Must land **before** the
write-surface sweep (§8.5) and the undo-action rewrites.

**Tasks:**

* **T4.4a** — add the identity to `Object`/`Base`: a small shared POD id value
  (`{epoch, counter}`, `operator==`, `qHash`, `toQString`) + `m_uuid`/`getUuid()`;
  `epoch` seeded once at startup off the RT path, `counter` a wait-free
  `fetch_add`; minted in **every** constructor incl. copy; immutable thereafter.
* **T4.4b** — route identity through **one comparator**, don't re-code it at
  every site. Overload equality on the `Object` pointee (compares `m_uuid`) plus
  a null-safe `shared_ptr` comparator (`sameObject(a,b)`); the raw `==` identity
  checks (`Hydrogen.cpp:495,1093`, `Sampler.cpp:364,395,1944`,
  `AudioEngine.cpp:1754,1775`, death row, note queue) call it, so the rule lives
  in one place. (`std::shared_ptr::operator==` can't be overloaded, so the
  comparison moves to the pointee/helper.) Make the list lookups uuid-based
  **while keeping their positional return** — add `index(const Uuid&)` (and
  uuid-based `del`/`remove`) to `InstrumentList`/`PatternList`/`Instrument`/
  `InstrumentComponent`; callers in `CoreActionController`/`MidiInstrumentMap`/
  `Sampler`/`Drumkit` pass `p->getUuid()`. **`index` returns the same
  position as before — it is NOT replaced by a differently-returning `find`.**
  Fix the two latent `getId()` matches (`Sampler.cpp:427`, `Drumkit.cpp:597`) the
  same way. (The GUI's *positional* `index()` uses are unaffected.)
* **T4.4c** — tests: id uniqueness + fresh-on-copy; cross-process epoch
  non-collision; a regression test that two instruments with identical `Id`s do
  **not** cross-match by id (the death-row / kit-switch hazard); an RT-safety
  assertion that minting takes no lock (no `QUuid::createUuid()` on the note
  path).

**Deferred (decide at editor mode):** persisting/transferring the id for
editor↔engine correspondence (a preserve-on-deserialise path, distinct from the
mint-on-copy rule); exact epoch width.

**Done when:** full suite + GUI smoke green (behaviour-preserving); no object
identity decided by raw pointer or bare `Id` in the swept sites; no
`QUuid::createUuid()` reachable from the audio thread.

---

## 8.5 Phase 4.5 — Single GUI→engine write surface (prerequisite for editor mode)

[ADR 0027](/docs/decisions/0027-coreactioncontroller-single-write-surface.md);
catalogue in [proposal 0005](0005-gui-engine-write-surface-catalogue.md).

**Status: ✅ DONE (2026-06-23).** A review of `src/gui/src/`, `src/core/Basics/`
and `src/core/AudioEngine/` found that the editor-mode premise — the GUI reaches
the engine only through an injected handle — was only half true: besides the ~70
`CoreActionController` (CAC) entry points and the Phase-2 reads, the GUI **directly
mutated** engine-owned `Basics` at ~110 sites, 16 taking the `AudioEngine` lock by
hand. CAC is now the single GUI→engine write surface, **CI-enforced**: the guard
`tools/check_write_surface` (CTest `WriteSurfaceGuard`) is green over engine-owned
`Basics` **and** mutating/playback calls on `AudioEngine`/`Transport`/`Sampler`. The
per-section catalogue in
[proposal 0005](0005-gui-engine-write-surface-catalogue.md) is fully marked done
(§2.1–2.7, bucket C incl. the audio/MIDI driver sweep of
[ADR 0029](0029-audio-driver-access-across-editor-split.md), bucket D). Suite
`OK (305 tests)`, ctest gate green. **Deferred by design (editor-mode, not sweep
gaps):** preview/audition of editor-local / not-in-song objects through the sampler
(allowlisted in the guard, ADR 0016), and the read-under-lock telemetry migration
(ADR 0018). Editor mode (T5.2-cont/T5.3/T5.4) resumes as CAC-over-IPC.

**Tasks** (full catalogue + per-class tables in proposal 0005):

* **T4.5a — ✅ DONE.** CAC gained the bucket-B entry points:
  `editNoteProperty`/`addOrRemoveNote`/`setPatternSize`; the full `Instrument` set +
  `setInstrumentAttack/Decay/Sustain/Release` (ADSR); `setComponent*`/`setLayer*`;
  `setPanLaw`; `toggleGridCell`; `previewInstrument` (folds the MixerLine stop-notes);
  `selectPattern`/`toggleNextPattern`/`movePattern`; plus `sequencerPlay` on
  `IEngineAccess` (`sequencerStop` already there). Each mutating entry owns the
  `AudioEngine` lock.
* **T4.5b — ✅ DONE.** Every GUI class swept (PatternEditor, SongEditor + pattern
  list, InstrumentEditor, ComponentView/LayerPreview, dialogs): direct `Basics`
  mutations and their undo commands route through CAC (guard-clean), view-refresh
  moved to `EventListener` reactions (e.g. `movePattern` → `PatternChanged`), and
  missing undo added for the instrument/component/layer edits.
* **T4.5c — ✅ DONE.** Bucket C rerouted: interpolation
  (`PreferencesDialog`→Preferences, `ExportSongDialog`→transient engine override),
  MIDI table/instrument-map (persisted via ADR 0023), audio/MIDI **drivers**
  (ADR 0029 override-layer config), metronome volume (Preferences). Bucket D:
  peak-meter model writes removed (read as telemetry); export/selection flags
  stay local.
* **T4.5d — ✅ DONE.** CI grep guard `tools/check_write_surface` (CTest
  `WriteSurfaceGuard`) is **green** and covers engine-owned `Basics` **and**
  mutating/playback calls on `AudioEngine`/`Transport`/`Sampler`. The only
  allowlisted exceptions are the preview/audition render commands (ADR 0016). Read-
  under-lock telemetry is intentionally not enforced (editor-mode migration, ADR 0018).

**Tests first:** per new CAC entry point, a unit test against a local headless
engine asserting the state change (+ that it is lock-safe); the standalone GUI
smoke + undo/redo stay green at every class converted.

**Done when:** the CI guard is clean (zero direct mutations remain); full suite +
GUI smoke green; undo/redo behaviour unchanged or improved. Editor mode then
resumes as pure CAC-over-IPC.

*Status against done-when (2026-06-23):* **met.** The guard (now covering `Basics` +
`AudioEngine`/`Transport`/`Sampler`) is clean, the suite is `OK (305 tests)`, the
ctest gate (incl. `WriteSurfaceGuard`) is green, and undo/redo is unchanged or
improved. The only carve-outs are the explicitly editor-mode-deferred preview/
audition and telemetry-read concerns.

---

## 9. Phase 5 — Out-of-process editor, IPC & layered config

ADRs 0016, 0018, 0022, 0023. Second-largest block.
**Depends on Phase 4.5** (single write surface): the editor-mode command tasks
below assume CAC is the complete GUI→engine write surface.

**Status: 🚧 IN PROGRESS** — T5.1's protocol foundation landed (2026-06-17),
suite `OK (272 tests)`. The IPC message codec, telemetry block, and event
classification are implemented in `src/core/IPC/` and covered by
`IpcProtocolTest` (7 cases): length-prefixed `QDataStream` framing
(`IpcMessage`/`IpcFrameReader`) with the CoreActionController opcode vocabulary +
typed args + XML payloads; `hello` handshake versioning; full `Event::Type`
round-trip; the seqlock'd `EngineTelemetry` POD (`telemetryStore`/`Load`, version
gate, threaded tear-free check); and `isEngineOriginEvent()` (OnlineImportProgress
stays editor-internal). T5.5 (layered config, ADR 0022) and T5.6 (concurrency-safe persistence, ADR
0023) also landed (2026-06-17), suite `OK (279 tests)`: `PluginConfig`
(`src/core/Preferences/`) defines the override-field set (single source of
truth), `applyOverride()` composes base⊕override, and `mergeForWrite()`/`persist()`
do the locked re-read + field-level 3-way merge + atomic `QSaveFile` write
(diff-against-baseline, override excluded). Covered by `PluginConfigTest` (4) and
`ConfigConcurrencyTest` (3, incl. a parallel-thread no-corruption check).
The live `QLocalSocket` transport + EventQueue-draining bridge (`IpcChannel`/
`IpcServer`/`IpcEngineBridge`, `IpcTransportTest`), the editor state mirror +
`IpcEngineAccess` (T5.2, `EditorMirrorTest`), and T5.7 sound-library rescan have
since landed.

**T5.2-cont — CAC over IPC ([ADR 0030](/docs/decisions/0030-coreactioncontroller-over-ipc.md)) — STARTED 2026-06-23.**
Stage 1 (mechanism) landed, suite `OK (306 tests)` + ctest 5/5: CAC's GUI-facing
methods are `virtual`; a new `IpcCoreActionController` (`src/core/IPC/`) overrides
them to marshal the command to an `IpcMessage` over the channel **and** apply it
to the local mirror (the `sequencerStop` pattern); `IpcEngineAccess::getCoreActionController()`
now returns it. Verified end-to-end by `IpcTransportTest::testProxyCommandReachesEngine`
(`proxy.setBpm` → mirror updated **and** opcode dispatched to the authoritative
engine).

**Stage 2 — vocabulary expansion (batches 2a + 2b), suite `OK (307 tests)` + ctest
5/5.** The GUI calls **96** distinct CAC methods (via `getCoreActionController()->`
or a cached `pController->`). The simple fire-and-forget tier is now fully routed —
**~74/96** covered: batch 2a (35 scalar Instrument/Component/Layer/strip/song
setters — `setInstrument*`/`setComponent*`/`setLayer*`/`setStripIs*`/`setHumanize*`/
`setSwing`/`setPanLaw`/`setPlaybackTrack*`) and batch 2b (23 transport/pattern/jack/
midi-clock commands — `move/removePattern`, `moveInstrument`, `renameComponent`,
`setPatternSize`, `toggle*`, `activate{Timeline,Jack*,PlaylistSong}`, `startCountIn`,
`clearMidi*Log`, `previewInstrument`, …). Opcodes + bridge cases + proxy overrides
were generated from the real CAC signatures (so they match exactly) and spliced in
lockstep; `IpcTransportTest::testProxyMarshalsParameterCommands` covers the wire
round-trip.

Batch 2c added the two many-arg note/grid edits that need no new plumbing —
`editNoteProperty` (16 scalar/enum/QString args) and `toggleGridCell` (`GridPoint`)
— bringing coverage to **~76/96**. That exhausts the Tier-2 commands that are pure
fire-and-forget.

**Request/response channel — ✅ DONE (2026-06-24), suite `OK (308 tests)`.** The wire
frame now carries a `quint32 requestId` (`IpcMessage`; `IPC_PROTOCOL_VERSION` → 2),
there is a `Reply` opcode, and `IpcChannel::request( req, reply, timeout )` stamps a
fresh non-zero id, sends, and blocks until the matching reply arrives — queueing any
other frames received meanwhile for `receive()`/`messageReceived()`, in order.
Verified by `IpcTransportTest::testRequestResponseRoundTrip` (threaded engine-side
responder echoes the id in a `Reply`; the blocking `request()` returns the
correlated result). This is the tier-3 prerequisite.

**Out-param / object-returning wiring — ✅ DONE (batch 2d), suite `OK (309 tests)`.**
The usage analysis decided the mechanism per method:
* `setInstrumentMidiOutNote`/`Channel` → **request/response** (the caller needs the
  engine's feedback-event id, to blacklist its own echo): a bridge
  `handleRequest()` applies the command and replies with the id; the proxy
  `request()`s when `pEventId != nullptr` (else plain send), and also base-applies
  to the mirror for the displayed instrument property. Tested
  (`IpcTransportTest::testProxyMidiOutRequestResponse` — bridge reply + proxy
  round-trip filling the id).
* `addOrRemoveNote` (`Uuid*`) and `handleNote` (`QStringList*`) → **dual-apply**:
  the out-param is filled mirror-side by the base call (the GUI's local undo /
  feedback uses the mirror), the opcode drives the authoritative engine.
* `loadPattern`/`loadPlaylist`/`loadPreferences`/`loadSong` → **no proxy override**:
  they are pure file parsers with no engine side effect, so they run on the mirror
  locally (shared disk); their parsed object then feeds a payload command
  (`setSong`/…) handled in the object-payload group below.

**Object-payload / value-struct wiring — batch 2e DONE, suite `OK (310 tests)`.**
The serialiser-ready subset is wired:
* `setSong` → **payload command**: the proxy serialises the song via
  `Song::toXmlBuffer()` into the `SetSong` payload (the engine reconstructs with
  `Song::fromXmlBuffer`); dual-applies the live object to the mirror. Tested
  (`testProxySetSongPayload` — payload round-trips, engine song name matches).
* `setSongProperties` / `setPatternProperties` → **value-struct commands**: their
  args are strings/ints + a `License` (marshalled as its license-string +
  copyright-holder, reconstructed via `License(str, holder)`) + a tags
  `QStringList` (a native `QVariant` type). No whole-object payload needed.

**Per-type serialisers — DONE.** `toXmlBuffer`/`fromXmlBuffer` added to
`Pattern`/`Drumkit`/`Instrument`/`Playlist` (mirroring the pre-existing `Song`
pair; tested by `H2ProjectTest::testBasicsBufferRoundTrip`). `Instrument::toXmlBuffer`
is non-`const` (its `saveTo` is). `Playlist::load_from` (declared-but-undefined)
was implemented by extracting the node-parsing out of `Playlist::load()` (which
now delegates to it after its legacy-format check); `saveTo` took a
`bool bUseRelativePaths` (was a `Preferences`) so `toXmlBuffer` can force absolute
paths (no playlist file to resolve against over IPC).

**Object-payload wiring (batch 2f) — DONE, suite `OK (313 tests)`.**
* `setDrumkit` / `setPattern` / `replaceInstrument` → **payload commands**: the
  proxy serialises the object into the message payload (drumkit/instrument carry
  samples by path → engine reloads; pattern serialised against the current
  drumkit for id/type resolution); identity rides as args (`setPattern`:
  number+replace; `replaceInstrument`: the old instrument's id). Dual-applied to
  the mirror. Engine reconstructs via the `fromXmlBuffer` serialisers.
* `addInstrument` → **request/response** when the caller needs the `long*` event
  id (instrument as payload, index as arg, id from the `Reply`); falls back to a
  plain payload command otherwise.
* **File-save ops** (`saveSong`/`saveSongAs`/`savePlaylist`/`savePlaylistAs`) →
  **engine-only commands** (no mirror dual-apply): the authoritative engine owns
  the write to the shared file; the editor must not double-write/race it.
* Tested: `IpcTransportTest::testProxyObjectPayloadCommands` (setDrumkit/
  setPattern/replaceInstrument payload + engine reconstruction; saveSongAs
  opcode/args) and `testProxyAddInstrumentRequestResponse` (event id from reply).

**Playlist wiring (batch 2g) — DONE, suite `OK (319 tests)`.**
* `setPlaylist` → **payload command**: the proxy serialises the playlist via
  `Playlist::toXmlBuffer()`; the engine reconstructs with `fromXmlBuffer`.
* `addToPlaylist` / `removeFromPlaylist` → marshal a single `PlaylistEntry` as its
  **mime text** (`PlaylistEntry::toMimeText`/`fromMimeText`, already present) plus
  the index; `remove` matches engine-side by value (`PlaylistEntry::operator==`).
* Tested by `IpcTransportTest::testProxyPlaylistCommands` (set → add → remove,
  each reconstructed + applied on the engine end).

**MIDI action map wiring (batch 2h) — DONE, suite `OK (423 tests)`.**
* `setMidiEventMap` → **payload command**: the editor's MIDI action table
  (`MidiActionTable`) is the sole writer of the map — every row edit,
  including the two MIDI undo actions, funnels through `persistMidiMap()` —
  which now, besides the config-file save, forwards the whole map via
  `CAC::setMidiEventMap` so the authoritative engine's MIDI dispatch applies
  the same bindings live. The proxy serialises the map with the new
  `MidiEventMap::toXmlBuffer()` (reusing `saveTo`/`loadFrom`, mirroring the
  file format's node nesting); the engine reconstructs with `fromXmlBuffer`
  and installs it granularly — no driver restarts, no `UpdatePreferences`
  (engine consumers read the map live per event; `MidiEventMapChanged`
  stays editor-side — the table fires it and the `MidiLearnable` widgets
  listen there).
* Rejected: per-row commands (table row indices are GUI-internal — the
  table keeps empty rows the map does not know — and the map is small
  enough for a whole-map payload) and riding `SetPreferences` (its
  core-props subset is coarser and couples the table to the prefs-dialog
  flow).
* Residual: engine-side preference replacements still override the
  engine's map — an NSM session open reloads the whole engine
  Preferences from the session file (`NsmClient` → `loadPreferences` +
  `setPreferences`); the next table edit re-forwards the map. The
  `shared_ptr` swap in `Preferences::setMidiEventMap` (and, since batch
  2i, `setMidiInstrumentMap`) is unsynchronised against the MIDI
  thread's live reads — a pre-existing hazard shape shared with
  `setPreferences` (whole-object swap) that deserves a class-level fix
  (mutex or atomics in the Preferences/Hydrogen accessors), tracked
  here.
* Tested: `IpcRoundTripTest::testMidiEventMapRoundTrip` (serialization
  round-trip + `setMidiEventMap` over IPC, engine map asserted equal) and
  `CoreActionControllerTest::testSetMidiEventMap` (base install + null
  rejection).

**MIDI instrument map wiring (batch 2i) — DONE, suite `OK (425 tests)`.**
* `setMidiInstrumentMap` → **payload command**, same whole-map shape as
  batch 2h: the MIDI control dialog funnels every edit through
  `persistMidiSettings()`, which now — besides the config-file save —
  forwards the instrument map so the authoritative engine's MIDI I/O
  (incoming note mapping, outgoing note/channel selection in the
  Sampler) applies the same settings live. Serialisation reuses
  `MidiInstrumentMap::saveTo`/`loadFrom` via the new
  `toXmlBuffer`/`fromXmlBuffer` (same nested-node unwrap as 2h); no
  driver restarts, no `UpdatePreferences`, and no event — none exists
  for this map (the GUI refreshes via local `changePreferences()`).
* Shares the residuals of batch 2h: an NSM session open replaces the
  whole engine Preferences (the next dialog edit re-forwards), and the
  `shared_ptr` swap is unsynchronised against live readers — here
  including the Sampler's audio-thread reads.
* Tested: `IpcRoundTripTest::testMidiInstrumentMapRoundTrip`
  (serialization round-trip incl. type- and id-based custom input
  mappings + `setMidiInstrumentMap` over IPC, engine map asserted
  equal) and `CoreActionControllerTest::testSetMidiInstrumentMap`
  (base install + null rejection).

**MIDI-learn channel wiring (batch 2j) — DONE, suite `OK (427 tests)`.**
* Closes the MIDI-learn gap tracked since batch 2h: the sense widget
  polled the mirror's `getLastMidiEvent()`, which only the engine's
  `MidiInput` ever sets — in the split the poll never saw an event.
  The reset before listening now crosses as a command
  (`CoreActionController::setLastMidiEvent`, args `[int type, int
  parameter]` — type and parameter are one logical value, the MIDI
  input writes both per event), and the poll is a new
  `IEngineAccess::getLastMidiEvent()` read: standalone serves the
  local engine (`LocalEngineAccess`), editor mode issues a single
  blocking query (`GetLastMidiEvent`) so the (type, parameter)
  snapshot can not tear across two separate queries — which could
  mix an event type from one MIDI message with the parameter of the
  next.
* The widget polls at 10 Hz while its dialog is open; each poll is one
  FIFO-ordered round-trip on the local channel (ordered after the
  reset command, so the widget never re-learns a pre-reset event). No
  event is fired — the widget polls; the engine has no listener.
* Opcodes appended at the enum tail (wire compatibility, like the
  ADR 0029 queries — unlike the mid-enum batch sections of 2a–2i).
* The pair itself stays plain, unsynchronised Hydrogen state (MIDI
  thread writes vs. bridge-thread reads engine-side, GUI-thread reads
  standalone) — the same class-level hazard shape as the tracked
  residual above.
* Tested: `IpcRoundTripTest::testLastMidiEventRoundTrip` (query level:
  the engine's pair is served and a divergent mirror value does not
  shadow it; command level: the reset crosses; the reset is visible
  through the query) and `CoreActionControllerTest::testSetLastMidiEvent`
  (base install of the pair + the reset);
  `EngineAccessTest::testLocalEngineAccess` covers the standalone
  passthrough.

**Custom sound library dirs wiring (batch 2k) — DONE, suite `OK (429 tests)`.**
* `SoundLibraryTree::addDirToLibrary`/`removeDirFromLibrary` (driven by
  `SE_modifyCustomLibraryDirsAction`) were mirror-local: they wrote the
  editor's `Preferences` copy and rescanned the mirror's database. They
  are now funnels into `CoreActionController::addCustomSoundLibraryDir`/
  `removeCustomSoundLibraryDir` (args `[QString dirPath]`): the base
  mutates that side's `Preferences` and rescans its
  `SoundLibraryDatabase` — the rescan is the part a plain
  `SetPreferences` sync would miss (`applyCorePropsFromXml` installs
  the dirs but triggers no engine rescan, and live GUI edits never
  send the bulk sync anyway). The engine-side apply's
  `SoundLibraryChanged` crosses (`isEngineOriginEvent`) and refreshes
  the editor's sound library idempotently — the established echo
  shape.
* Opcodes appended at the enum tail (wire compatibility, like 2j).
* Behaviour notes: add is now idempotent (an already-registered dir is
  a logged no-op returning true — previously a duplicate entry was
  appended and then skipped with an `ERRORLOG` during the scan),
  remove is idempotent (`removeAll` no-op), and an empty path is
  rejected with `ERRORLOG` + false (previously a silent return in the
  tree). The rescan runs only on an actual change. The undo action is
  untouched — redo/undo call the funnels.
* Race surface unchanged but resting on convention, not locks: each
  side's write (prefs list swap + database rebuild) is confined to
  that side's command thread (GUI thread editor-side, single bridge
  thread engine-side — one connection, dispatch and event forwarding
  serialized in the same loop). The prefs list and the database are
  unguarded structures, and the engine's local OSC/MIDI/NSM inputs
  can read the dirs list and mutate the database concurrently — a
  hazard pre-dating this batch (shared with `SetPreferences` and
  `RescanSoundLibrary`), which this batch makes more likely to
  manifest by turning engine-side database rebuilds into a routine
  side effect of ordinary GUI edits. Deserves the same class-level
  fix as the tracked `shared_ptr`-swap residual.
* Tested: `CoreActionControllerTest::testAddRemoveCustomSoundLibraryDir`
  (base: prefs and database level with a real kit saved into a
  `QTemporaryDir`, idempotence, empty rejection, original dirs
  restored) and `IpcRoundTripTest::testCustomLibraryDirsRoundTrip`
  (the add/remove cross; the engine's database gains and drops the
  kit — proving the rescan crossed, not just the prefs list; the
  mirror stays coherent synchronously via the dual-apply base call).

**Song export wiring (batch 2l) — DONE, suite `OK (431 tests)`.**
* One-shot design (decided against a per-file re-drive after review): the
  pre-split dialog chained renders through progress events —
  `audioExportProgressEvent` called `exportTracks()` which issued the next
  `startExportSong`. Across IPC that would turn every file into a
  round-trip with the dialog re-driving engine state it cannot see.
  Instead `CoreActionController::exportSong(sampleRate, sampleDepth,
  compression, interpolateMode, rubberbandBatch, renders)` carries the
  whole plan — the master file plus per-instrument trackouts with their
  exclusion lists (ADR 0027) — in a single request. The engine
  (`Hydrogen::exportSong`) parks the transport state, applies the batch
  flag and interpolation override, arms the session, and a dedicated plan
  thread renders the queue sequentially within one `DiskWriterDriver`
  session, polling the writer's per-file done/failed flags. The mirror
  never renders.
* `stopExportSession` is the universal stop and idempotent: it cancels
  the plan, joins the plan thread, and finishes the session (restore
  song mode/loop, recalculate rubberband when the batch flag was set,
  restore flag + interpolation override, restart the drivers). On
  natural completion the plan thread finishes the session itself, so a
  later editor-side stop joins the exited thread and does nothing.
  `TestHelper::exportSong` and the CLI keep using the primitives
  directly (the plan-session gating keeps them compatible).
* GUI: `ExportSongDialog` builds the plan upfront — overwrite prompts
  included (No aborts before anything renders; YesToAll honoured) —
  reads the rubberband checkbox and resampler combo fresh at OK, and
  sends one `exportSong`. Progress events only update the bar (scaled
  over `m_nPlanDone`/`m_nPlanTotal`) and detect completion/failure
  (`IEngineAccess::isExportWritingFailed` disambiguates a failed 100,
  ADR 0029); `closeExport` reduces to the idempotent
  `stopExportSession` plus the editor-local pref/timeline restores.
  Deleted: the `exportTracks()` chaining, the live
  `toggleRubberbandBatchMode`/`setResamplerMode` slots (both applied at
  export start now), and the `m_bExportTrackouts`/`m_nInstrument`
  members. The `EditorPathExerciser` exports through the controller as
  well.
* IPC surface: `ExportSong` request (args `[int rate, int depth, double
  compression, int mode, bool batch, int renderCount, per render:
  QString fileName + QStringList excluded uuids]`, reply `[bool]`),
  `StopExportSession` command, `GetExportWritingFailed` query —
  appended at the enum tail (wire compatibility, like 2j/2k).
  `IpcCoreActionController::exportSong` is engine-only (no channel →
  `WARNINGLOG` + false — a base call would render on the mirror);
  `stopExportSession` sends when a channel exists, else base call (the
  `saveSong` pattern). In editor mode `isExportWritingFailed` is a
  blocking query; the -1 progress event stays the authoritative failure
  signal, the query only disambiguates a completed 100.
* Driver fixes surfaced by the new cancel test: `DiskWriterDriver` kept
  its writer-thread handle in a file-scope global that survived across
  the per-session driver instances — `disconnect()` joined it
  unconditionally, so stopping a session before its first render joined
  a stale, already-reaped handle (segfault), and every additional file
  in a multi-render session leaked a zombie thread. The handle is now a
  per-instance member with a validity flag: `write()` reaps the previous
  finished writer before spawning the next, `disconnect()` joins only
  when a writer exists. `write()` also resets the done/failed flags per
  file (plan polling), and libsndfile errors report
  `sf_strerror(pSndfile)`.
* Drive-by dialog fixes: `findUniqueExportFileNameForInstrument`
  compared a fixed `m_nInstrument` instead of the loop index (the
  occurrence count was constant — colliding instrument names were never
  disambiguated) and dereferenced without a null check; the progress bar
  computed `std::min(nValue, 0)` (always zero); the -1 failure path left
  the close button disabled (only a 100 re-enabled it). All three are
  resolved by the rewrite.
* Residuals: non-export transport commands during an active session are
  not rejected (pre-split behaviour — the session parks song mode/loop,
  so a locate would fight the render; same exposure as before); a
  single file's render is uncancellable mid-way (a stop takes effect
  between process callbacks, as today); a user-initiated cancel makes
  the aborted writer push a spurious `AudioExportProgress -1`
  (harmless today — the modal dialog is deleted before the GUI thread
  can dispatch it; a trap if the dialog ever becomes non-modal); the
  IPC-level cancel and
  write-failure paths rest on the base test's coverage (the IPC test
  exercises the happy path and exclusion marshalling); `m_bIsRunning`
  remains an unsynchronised bool (writer-thread read vs. disconnect
  write) — the pre-existing hazard shape shared with the tracked
  `Preferences` swap residual; an NSM session open reloads the engine
  `Preferences` and overrides the batch-flag restore (same shape as the
  2h/2i residual).
* Tested: `CoreActionControllerTest::testExportSong` (base: two-render
  plan with an excluded instrument, both files written, batch flag
  flipped and restored, interpolation override applied and cleared,
  drivers back, immediate cancel + idempotent second stop) and
  `IpcRoundTripTest::testExportSongRoundTrip` (the mirror's driver is
  never a `DiskWriterDriver` before/during/after, the exclusion-list
  marshalling is exercised (its semantics stay asserted at engine
  level in `AudioExportTest`), the session lifecycle is observable via
  the query,
  the stop is idempotent over IPC).

**Export transport fixes (batch 2m) — DONE, suite `OK (431 tests)`.**
* User report: after a multi-instrument export the editor's transport kept
  rolling and the toolbar stop seemed not to take. Two root causes, both
  reproduced in `IpcRoundTripTest::testExportSongRoundTrip` (extended:
  transport rolling into the export, both state machines asserted at rest
  afterwards, a stop issued over the channel during a six-render plan):
  1. The telemetry play-follow (`EditorStateMirror::applyTransportSnapshot`)
     mirrors the engine's rolling state into the editor — including the
     export's background renders. Every render's `play()` rolled the
     mirror, and a stale in-flight `playing=1` snapshot (built during the
     last render) re-armed the mirror *after* the engine's final stop,
     leaving the editor rolling with the engine at rest.
     `EngineSession::buildTelemetrySnapshot` now reports the user-facing
     transport: `playing=0` while an export session is active (the
     session parks the user transport in `Hydrogen::startExportSession`).
  2. A stop landing while the plan was still running was overridden by the
     next render's `play()` (engine) and by the play-follow (mirror) — the
     "unstoppable" wedge. `Hydrogen::sequencerStop()` now cancels an
     active export session (the teardown parks the transport itself; the
     arm-time park runs before the session flag is set, so no recursion).
  3. The cancel path raced the writer thread: `finishExportSession` ran
     `stopExportSong()` (an unlocked `Sampler::stopPlayingNotes()` flush
     plus a locate) and the song mode/loop restore *before* the disk
     writer was joined — in the cancel path the writer is still
     mid-render, and the sampler-queue mutation under the live render
     surfaced as an ADSR use-after-free segfault in the writer's render
     path (a ~1-in-2 flake once the IPC test stopped mid-render
     deterministically). `finishExportSession` now tears the driver down
     first: between that teardown and the driver restart no audio
     callback exists at all, so the unlocked flush is safe by
     construction. The join is bounded by a new cooperative cancel flag
     on `DiskWriterDriver` (`std::atomic<bool> m_bCancelRequested`,
     raised in `disconnect()`, checked by the render loop every buffer,
     reset on arm) — a mid-render stop joins within one buffer instead
     of waiting out the song.
* Belt and suspenders: `IpcCoreActionController::exportSong` parks the
  mirror's transport directly on a successful command — the editor's
  transport stops the moment the export starts, without waiting a
  telemetry cycle. Unconditional on purpose: the mirror's state may be
  mid-transition (a pending play), and stopping an idle transport is a
  no-op.
* This partially resolves the 2l residuals: "non-export transport
  commands during an active session are not rejected" — a stop now
  cancels the session instead of fighting the renders (other transport
  commands like locate/play keep the pre-split exposure); and "a single
  file's render is uncancellable mid-way" — the cancel flag ends the
  in-flight render within one buffer.
* Tested: the extended `testExportSongRoundTrip` phases (at-rest asserts
  on both sides after a two-render plan that started rolling, the
  stop-during-plan cancel with the queued renders never run — this phase
  also exercises the mid-render teardown window that produced the
  segfault, verified with three consecutive full-suite runs) plus new
  engine-state asserts in the base `CoreActionControllerTest::
  testExportSong` (local mode: neither rolling nor a sticky pending
  Playing after the plan).

**MIDI note recording crossing (batch 2n) — DONE, suite `OK (432 tests)`.**
* User report: MIDI notes recorded in the split never appeared in the
  pattern editor. `Hydrogen::addRealtimeNote()` (engine-only, reached from
  `CoreActionController::handleNote` — engines own all control surfaces,
  ADR 0016) queues note-ons into the engine-local
  `EventQueue::m_addMidiNoteVector`; in standalone the GUI's
  `HydrogenApp::onEventQueueTimer` drains that vector and integrates each
  note as an undoable pattern edit (pattern-editor DB row correlation +
  `SE_addOrRemoveNoteAction` under an undo macro). In the split the
  editor drains the *mirror's* vector — the engine's entries never
  crossed, so the recording was lost entirely (and the engine's vector
  grew unboundedly: nothing else drains it in the split).
* Design: cross the raw note, reuse the GUI drain verbatim. The engine
  stays authoritative for the *input* (MIDI in, record-arm, playhead,
  quantize — all engine-side, unchanged), the editor for the *edit* (the
  drain's undo macro and toggle semantics are editor-domain; replicating
  them engine-side would duplicate GUI logic and lose the undo history).
  The note closes the loop through the existing editor→engine write
  path: the drain's undo action dual-applies via
  `IpcCoreActionController`, so both songs end up with the note.
  Rejected alternatives: engine-side integration + `UpdateSong` re-pull
  (loses undo, a full-song XML pull per note), a new `Event::Type`
  (events carry only type+value+id — eight fields don't fit), telemetry
  SHM (lossy-tolerant by classification, ADR 0018 — recorded notes must
  not drop).
* Mechanics: new fire-and-forget opcode `IpcOpcode::MidiNoteRecorded`
  carrying the eight `AddMidiNoteVector` fields as args
  (`IpcMessage::fromMidiNote`/`toMidiNoteFields`, the `fromEvent`/
  `toEventFields` pattern). The engine's serve loop drains the vector
  beside `forwardEvents` (`EngineSession::forwardMidiNotes`): the drain
  takes the audio engine lock with the telemetry discipline —
  `tryLockFor(2ms)`; the pushing audio thread only try-locks itself, so
  it can not be starved; on contention the notes wait one serve cycle —
  swaps the vector out under the lock, and sends outside it (a
  potentially blocking write must never hold the engine lock).
  `addRealtimeNote()` itself is untouched: the realtime path stays
  exactly as in standalone. While no editor is attached,
  `discardMidiNotes()` drains instead (without a GUI consuming them the
  notes are unrecordable, and the vector must not pile up — the
  `discardEvents` rationale). Editor-side, `EditorStateMirror::
  applyEvent` handles the opcode by re-queueing the note into the
  mirror's `m_addMidiNoteVector`; `onEventQueueTimer` then consumes it
  unchanged. Ordering is single-serve-thread → FIFO channel →
  single-reader-thread; latency is one serve cycle — imperceptible for
  recording.
* Residuals: note-off length edits stay engine-side (the authoritative
  playhead computes the length) and reach the mirror via the existing
  `SongIsModified` debounced re-pull — functional, but the length
  appears after the debounce window and the interim `PatternChanged`
  redraw is stale; a dedicated crossing message for the mutated note is
   the follow-up if the latency bothers. A very short tap (shorter than
   the note-on's crossing + dual-apply round-trip, ~60-100ms) can note
   off on the engine before the note exists there — the length edit
   finds nothing and the default length stands. The vector keeps its
   pre-existing unsynchronised pop shape (standalone: audio-thread push
   under the engine lock vs. GUI-timer erase without; the split adds a
   reader-thread push against the same erase) — a class-level
   `EventQueue` mutex is the clean fix if it ever bites. Both residuals
   are resolved later: the reroute became batch 2o; the tap-race window
   is verified structurally closed and the mutex landed as batch 2q.
* Tested: `IpcRoundTripTest::testMidiNoteRecordingRoundTrip` (RED first:
  the note never reached the mirror) — the engine records while Playing
  with record armed, the note crosses with all eight fields intact
  (id/pattern/velocity/pan/length/key/octave exact, the column bounded by
  the quantized playhead), and the engine's vector is drained (no
  unbounded growth). The GUI-side integration (undo macro, DB row
  correlation) is GUI code and stays manually verified in the running
  split. Standalone is untouched by construction (the serve loop only
  exists in the split); three consecutive full-suite runs green.

**Note-off length edits via the undo path (batch 2o) — DONE, suite
`OK (432 tests)`.**
* User report: the note-off length adjustment in
  `Hydrogen::addRealtimeNote()` mutated the engine's pattern directly —
  in the split the edit never reached the editor (only the debounced
  `SongIsModified` re-pull carried it, with a stale `PatternChanged`
  redraw in between), and the mutation itself was the last remaining
  direct pattern writer besides the CAC-owned path (ADR 0027), running on
  the audio thread. Dormant races it carried: the audio-thread mutation
  vs. the editor's edit path; a pitch-blind search that matched *any*
  note of the instrument at the last recorded tick and mutated *every*
  match (two same-instrument notes at one column with different pitches
  had their lengths clobbered by one note-off); in the split, two writers
  (engine mutation vs. dual-apply) racing for the same note; and the
  length edit was invisible to undo.
* Reroute (user-prescribed): `EventQueue::AddMidiNoteVector` gains
  `bNoteOff`. The engine still computes the hold length — only the
  authoritative playhead knows it, and the mirror's lags — including the
  wrap trim and pattern-size clamp (hoisted out of the removed mutation
  loop; the clamp condition never depended on the matched note), but
  instead of mutating, it queues a note-off entry (`nColumn` =
  `m_nLastRecordedMIDINoteTick`, `nLength` = computed, key/octave with
  the same derivation as note-on; the common fields and the pitch
  derivation are now built once for both entry kinds). The note-on entry
  sets `bNoteOff=false`; `m_nLastRecordedMIDINoteTick` stays note-on-only.
  The `PatternChanged` push and `setPatternModified` call are gone — the
  edit's own flow marks the modification.
* Editor side, the drain (`HydrogenApp::onEventQueueTimer`) branches on
  `bNoteOff`: the same DB-row correlation and pitch-aware `findNote`, then
  a single `SE_editNotePropertiesAction(Property::Length, …)` in the
  pass-through idiom (only `nLength`/`nOldLength` differ; everything else
  old==new from the found note) — no undo macro for the single command.
  The action routes through `PatternEditor::editNotePropertiesAction` →
  `CoreActionController::editNoteProperty` (ADR 0027), so in the split it
  dual-applies to the engine like every other edit, and the length edit
  is undoable in both modes. A note-off with no matching note (the
  note-on not integrated yet, undone, or removed) degrades to a
  `WARNINGLOG` + skip — the same lost-tap outcome as before, minus the
  wrong-note clobbering. Drive-by fix in the same loop: the
  no-DB-row case returned *before* erasing the head entry, wedging the
  whole vector behind an unmappable instrument (pre-existing; now drops
  the note and continues).
* IPC: `fromMidiNote`/`toMidiNoteFields` carry the ninth `bNoteOff` arg
  (same-binary wire compatibility, as established).
* Residual: the tap race — a note-off arriving before the note-on's
  crossing + integration (~60-100ms in the split, one GUI timer cycle in
  standalone) finds no note and the tap keeps the default length. The
  vector's unsynchronised push/erase shape stays pre-existing (documented
  in 2n). Later verified structurally closed (see 2q): MIDI ordering, the
  end-to-end FIFO, and the drain's same-thread synchronous dual-apply
  mean the note-off's `findNote` can never run before its note-on's
  local apply — the window is zero by construction, not by timing luck.
  The skip-on-no-note branch remains as the tripwire for the genuine
  drop paths (DB-row miss, local-apply failure); the vector hazard
  itself was fixed as batch 2q.
* Tested: `testMidiNoteRecordingRoundTrip` extended (RED first: the
  note-off entry never crossed) — after the note-on phase a note-off
  crosses with `bNoteOff=true`, `nColumn` equal to the note-on's column,
  the length bounded by the pattern, id/pattern/key/octave matching; the
  engine's vector is drained again; and the engine's pattern still holds
  zero notes — the regression assert that the direct mutation is gone
  (in the headless harness nobody integrates the entries, so any note in
  the engine's pattern could only come from the removed path). The GUI
  branch (undoable length edit, skip-on-no-note) is GUI code and stays
  manually verified. Three consecutive full-suite runs green.

**Note-off targeting (batch 2p) — DONE, suite `OK (432 tests)`.**
* User report: 2o's note-offs did not work. The note-off entry targeted
  `m_nLastRecordedMIDINoteTick` — the tick of whichever note-on was
  recorded *last*, not of the note-on this note-off belongs to. Any
  intervening note-on (another instrument or pitch held or tapped while
  the first note was still held — the normal polyphonic case) moved the
  scalar, so the editor's exact-position `findNote` missed the actual
  note (or, pre-2o, the engine's pitch-blind search mutated the wrong
  one). The exact-position lookup itself is right — the note *is* at an
  exact position — the entry just carried the wrong one.
* Fix: the scalar is replaced by
  `Hydrogen::m_pendingRecordedNoteOns`, a map keyed by
  (instrument id, key, octave) with (tick, pattern number) of the
  recorded note-on. A note-on stores its position; the matching note-off
  resolves *its own* note-on from the map, computes the hold length
  against that tick (wrap trim and pattern-size clamp against the
  recorded pattern's length — the entry also carries the recorded
  pattern, so a pattern switch between on and off no longer retargets
  into the wrong pattern), and erases the spent entry. A note-off with
  no pending note-on (not recorded, already released, or the recorded
  pattern deleted) queues nothing — the playback section still releases
  the sounding note. A re-trigger of a still-pending pitch overwrites
  its entry, matching the editor's note-on replace semantics. The
  `m_nLastRecordedMIDINoteTick` member is gone (it had no other
  consumer; `toQString()` reports the map size instead).
* Test calibration facts surfaced on the way: the empty song's patterns
  are one 4/4 bar — `nTicksPerQuarter` is 48, so `Pattern()` is 192
  ticks (not 768), the default quantize grid is 12 ticks, and
  `AudioEngine::getPlayhead()` returns a `shared_ptr<Transport>` that
  the engine may swap — polls must re-fetch it, a captured pointer reads
  a frozen tick.
* Tested: `testMidiNoteRecordingRoundTrip` extended again (RED first:
  `noteOffA2.nColumn == noteOnA2.nColumn` failed — the note-off carried
  the intervening note-on B's tick): a second instrument is added to the
  engine's kit; note-on A2, an intervening note-on B of the other
  instrument, then the note-off for A2 must target A2's column; a
  transport-wrap phase (note-on past the pattern midpoint, note-off
  after the wrap) asserts the exact trim
  `length == patternLength - noteOnColumn`; a stray note-off asserts
  nothing is queued. Four full-suite runs: green, green, one known
  `MidiDriverTest::testMidiClock` BPM-tolerance flake (unrelated,
  rerun green), green.

**Midi-note vector locking (batch 2q) — DONE, suite `OK (433 tests)`.**
* Follow-up to the 2n/2o residuals: one verification, one fix.
* Verified closed — the tap race as feared in 2n/2o. A fast tap's
  note-off entry can not overtake its note-on's integration: MIDI
  ordering (a note-off for a pitch follows its note-on), the
  end-to-end FIFO (engine vector → serve drain → channel → reader →
  mirror vector), and the GUI drain's same-thread processing with a
  synchronous dual-apply (`IpcCoreActionController::addOrRemoveNote`
  sends *and* applies locally in one call) stack up — the note exists
  in the mirror's pattern before the loop advances to the note-off
  entry. Post-2p the engine side has no existence dependency either
  (the pending map is populated under the same lock at note-on time).
  The `findNote`-miss `WARNINGLOG` stays as the tripwire for the
  genuine drop paths (DB-row miss, local-apply failure, pattern
  deleted mid-flight).
* Fixed — the vector push/erase hazard. `m_addMidiNoteVector` was a
  public member pushed by the audio thread (standalone,
  `Hydrogen::addRealtimeNote`) or the IPC reader thread
  (`EditorStateMirror::applyEvent`, split) while the GUI timer
  iterated/erased it with no lock — undefined behaviour that could eat
  a fast tap's note-off or corrupt the vector. The member is private
  now, behind a dedicated mutex with three methods:
  `pushMidiNoteAction` (producers), `drainMidiNoteActions`
  (swap-under-lock for the consumers — the GUI timer in both modes and
  the engine serve loop in the split), and `getMidiNoteActions` (a
  non-consuming snapshot for tests/debugging, which also makes the
  round-trip test's own cross-thread polling defined behaviour). The
  GUI drain swaps the batch out once and iterates its private copy —
  the per-entry `erase(begin())` is gone, and with it the whole
  wedge class from 2o (a skipped entry is just a `continue`). The
  engine-side `drainMidiNotes` drops its audio-engine `tryLockFor`
  detour: the queue's own mutex serializes push against swap, both
  critical sections are a single vector operation, so neither side can
  stall the other and the audio engine lock is not involved at all.
* One corner changes semantics: a null song mid-drain used to `return`
  and retry the whole vector against the next song; the batch drain
  drops the remainder with a `WARNINGLOG` instead — the entries were
  recorded against a song that is being replaced, so retrying them
  against the next one would be wrong (the 2o drop-not-wedge
  principle).
* Tested: `EventQueueTest::testMidiNoteVectorThreadedAccess` (RED
  first: the API did not exist) — FIFO drain order, drain-consumes,
  snapshot-does-not-consume, and a 2000-entry producer thread drained
  concurrently by the test thread with every entry accounted for
  exactly once and in order (the audio/reader-thread vs. GUI-timer
  shape). `testMidiNoteRecordingRoundTrip` converted to the snapshot
  API, unchanged in behaviour. Three consecutive full-suite runs green.

**Metronome volume across the split (batch 2r) — DONE, suite
`OK (433 tests)`.**
* User report: `HydrogenApp::onPreferencesChanged()` applied the
  metronome volume chosen in `PreferencesDialog` via
  `m_pHydrogen->getAudioEngine()->getMetronomeInstrument()->setVolume()`
  — in the split that pokes the editor's *mirror* engine; the
  authoritative engine never heard the change and kept clicking at the
  old volume.
* Fix: the setting becomes a CAC command, `setMetronomeVolume(float)`,
  following the `setMetronomeIsActive` sibling end to end. The base
  implementation writes the engine's `Preferences::m_fMetronomeVolume`
  copy (what a restarted engine re-applies — see the AudioEngine
  constructor) and pokes the runtime volume on the engine-owned
  metronome instrument; `IpcCoreActionController` dual-applies (send
  `SetMetronomeVolume` [float] + base call, so the mirror stays
  consistent too); `IpcEngineBridge` dispatches the opcode onto the
  engine-side controller. `onPreferencesChanged` now routes through
  the CAC — in standalone the base call reproduces the old direct poke
  exactly (plus an idempotent pref write).
* Tested: `EngineSessionTest::testCommandDispatchedToEngine` extended
  (RED first: the method did not exist) — the editor-side controller
  sets 0.25 and the authoritative engine ends up with both its pref
  copy and its metronome instrument at 0.25. Three consecutive
  full-suite runs green.

**Is-modified flags across the split (batch 2s) — DONE, suite
`OK (433 tests)`.**
* User report: the save handlers in `MainForm` (`action_pattern_save`,
  `action_pattern_save_as`, `action_drumkit_save`,
  `action_drumkit_save_as`, `onAutoSaveTimer`) cleared the is-modified
  flag directly on the editor's objects (`Pattern::setIsModified(false)`
  + a manual event push). In the split the authoritative engine's
  copies stayed dirty forever.
* Design correction: the pattern/drumkit crossings already exist as Class C song
  state on the `IEngineAccess` surface (ADR 0026 point 13) —
  `IpcEngineAccess::setPatternModified()/setDrumkitModified()` dual-apply and
  the bridge applies them under `Suppress`. The sites now route through
  `HydrogenApp::pEngine()` (the `IEngineAccess` handle) like `Modifier` and
  `NotePropertiesRuler` already do — the manual event pushes went away too
  (`Hydrogen::setPatternModified()`/ `setDrumkitModified()` push them).
* Only the playlist variant was missing: new
  `Hydrogen::setPlaylistIsModified(bool)` wrapper (no song-dirty
  coupling — the playlist is an artifact in its own right, and no
  event: none exists for playlists), `IEngineAccess` virtual +
  `LocalEngineAccess`/`IpcEngineAccess` impls (dual-apply; no
  `Suppress` needed engine-side since the flip queues no event),
  `SetPlaylistIsModified` opcode + bridge case.
* `onAutoSaveTimer` re-asserts playlist dirty *after* the autosave
  write (an autosave is not a save) — that flip crosses now too.
* Gotcha: `Pattern::setIsModified()`/`Drumkit::setIsModified()` only
  stick for file-backed objects (non-empty path); the test gives
  pattern 0 a path first (idiom from
  `ConnectViaIpcModeTest::testEngineWrapperFlipWhileDirtyEchoes`).
* Tested: `EngineSessionTest::testCommandDispatchedToEngine` extended
  (RED first: `setPlaylistIsModified` did not exist) — both directions
  (mark + clear) for all three flags on the authoritative engine.
  Three consecutive full-suite runs green.

**Save-as path policy across the split (batch 2t) — DONE, suite
`OK (433 tests)`.**
* User report: after `SongPropertiesDialog` or save/save-as the mirror-local
  `Song::setPath()` diverged from the engine, patched over with full-XML
  `CAC::setSong()` re-syncs in `MainForm` (reject path, NSM export restore,
  demo open) — costly (whole-song round-trip per save) and error-prone.
* Prescription: deduce the final path upfront and let the dual-apply CAC
  save routines assign it directly. The mirror-local song is never re-pointed
  behind the engine's back anymore.
* `CoreActionController::PathPolicy` (Adopt default / Keep): `saveSongAs()`
  gains the policy; the base snapshots the former path/backed-by-project and
  restores it for Keep on success *and* failure (a failed export must not
  re-point the session song). `IpcCoreActionController::saveSongAs()` is no
  longer send-only: it sends `[path, keepMissing, policy]` and applies
  mirror-side (Adopt: `setPath` + `setBackedByProject(false)`; both: dirty
  flip + recent-files + last-song-path + local `UpdateSong(1)`).
  `saveSong()` gains the mirror-side dirty flip + `UpdateSong(1)`.
* Echo gating: the engine's own `UpdateSong(1)` pushes (plain save branch +
  save-as) are now `ProcessMode::Headless`-gated — in editor mode the
  override pushes on the mirror instead; the engine-origin echo only ever
  triggered a redundant full `ipcSyncSong` re-pull. The discard-autosave
  branch keeps its unconditional push (content actually changed).
* The path left `setSongProperties()` end-to-end (param dropped in base,
  IPC send, bridge case `args.size() >= 7`): its only caller was the undo
  action, and undoing a properties edit must not re-point the song anyway.
  `SE_modifySongPropertiesAction` drops both path fields (14 → 12 ctor
  args); the dialog's ok-handler no longer `setPath()`s in its SaveAs
  branch.
* GUI: `SongPropertiesDialog` gains a `sDefaultPath` ctor param (wins over
  the song's stale backing path when non-empty) + `getChosenPath()`.
  `MainForm::action_file_save_as(sSuggestedPath = {})` computes the default
  locally (no pre-seeding `setPath`), passes `Keep` under NSM / `Adopt`
  otherwise, and the reject path is a plain `return` — both `setSong()`
  re-syncs deleted. The System-context redirect in `action_file_save`
  suggests the user-level path instead of writing it into the song.
  Demo open: `HydrogenApp::openFile(..., bOpenAsUnsaved)` clears the path on
  the payload *before* the single `setSong` crossing (was: open + local
  `setPath("")` + second crossing). `SoundLibraryTree`'s two detached
  flows save to `dialog.getChosenPath()`.
* Side effect (bug fix): the NSM export status bar message now shows the
  chosen export target (was: the unchanged session path). Follow-up
  (user-approved): the song properties undo entry was labeled
  "Modify pattern properties" (copy-paste from the pattern action) — new
  commonized string `CommonStrings::getActionModifySongProperties()`
  ("Modify song properties").
* Follow-up audit (user-reported): the two remaining `setPath()` calls in
  `MainForm::onAutoSaveTimer()` (song + playlist restores after the
  autosave write) are legitimate and stay: `Song::save()` /
  `Playlist::saveAs()` adopt the target path unconditionally, the autosave
  file must not become the backing path, and the writes are editor-local
  recovery writes that cross nothing. Routing them through the save-as
  commands would pollute recent-files/last-path preferences and fire
  events — `PathPolicy::Keep` is export semantics, not recovery semantics.
  `tools/check_write_surface` updated to the post-2t `setPath` surface:
  the three stale MainForm allow entries removed (sites deleted in this
  batch), the restores regrouped under a documented autosave section,
  `pPlaylist` added to the guarded pointer set, and the
  `HydrogenApp::openFile` scratch-copy clear allowlisted.
* Base-class access: `getHydrogen()` protected accessor +
  `insertRecentFile()` moved to protected for the mirror-side bookkeeping.
* Tested: `EngineSessionTest::testCommandDispatchedToEngine` extended
  (RED first: `PathPolicy` did not exist) — Adopt: engine and mirror both
  carry the target path synchronously after the call, file exists; Keep:
  file written, both paths unchanged. Three consecutive full-suite runs
  green.

**Sound-library rescans across the split (batch 2u) — DONE, suite
`OK (434 tests)`.**
* User report: `MainForm::loadDrumkit()` and the other GUI library mutations
  rescan only the mirror-local `SoundLibraryDatabase` — but the authoritative
  engine owns one too (built unconditionally in its `Hydrogen` ctor) and
  resolves songs (`PatternList::loadFrom`), bridge pattern deserialization,
  `GetSoundLibraryInfo`, and the `getEmptySong()` GMRockKit fallback through
  it. A GUI-side rescan that stays mirror-local leaves the engine stale.
* New `IEngineAccess::updateSoundLibrary(SoundLibraryInfo::Type, trigger)` +
  `rescanSoundLibrary()`: one general per-type rescan (Drumkit / Pattern /
  Song) plus the full rescan, replacing the per-type db calls at all
  editor-side call sites. `LocalEngineAccess` forwards to the wrapped
  engine's db (standalone behavior unchanged); `IpcEngineAccess` dual-applies
  — mirror db sub-scan with the caller's trigger (its `SoundLibraryChanged`
  push refreshes the GUI) + new `UpdateSoundLibrary` opcode `[int type]`;
  the bridge applies the engine-side sub-scan under `Suppress` (the editor
  already pushed the event on its mirror — no echo). `rescanSoundLibrary()`
  reuses the existing (previously sender-less) `RescanSoundLibrary` opcode;
  the engine-side `update()` pushes unconditionally, its echo re-fans-out on
  the editor as a harmless redundant refresh. `Type::Instrument` is a
  programming error (the db tracks no instrument list): ERRORLOG + no-op,
  no send; values outside the enum fall through to the bridge's generic
  failure.
* GUI reroute: `OnlineImportDialog` (3 per-type calls),
  `SoundLibraryTree` (properties / duplicate / delete flows, 7),
  `DrumkitPropertiesDialog` (1), `MainForm` (pattern saves ×2, loadDrumkit,
  drumkit save), `SoundLibraryPanel::onRescanClicked` (full rescan) — all
  routed through the engine access; the now-dead `pDB` locals removed.
* Tested: new `EngineSessionTest::testSoundLibraryRescanCrossesSplit`
  (RED first: the methods did not exist). Fixture artifacts (flat baseKit
  copy, pattern, song) land in the user-level library dirs *after* both
  databases were built, under unique names; per type the rescan must make
  the engine's db know the artifact (pumped — bridge thread) and the
  mirror's synchronously (dual-apply); the full rescan must drop a removed
  pattern from both. Probe gotchas: `SoundLibraryDatabase::getDrumkit()`
  loads a missing kit from file and inserts it (session-drumkit fallback),
  so the drumkit probe asserts map membership via `getDrumkitDatabase()`;
  and the user-level dirs come back with a trailing separator, so probe
  keys go through `QDir::cleanPath` (a stray `//` never matches a scan
  key). Three consecutive full-suite runs green.

**MIDI setup fix across the split (batch 2v) — DONE, suite
`OK (435 tests)`.**
* User report: `MainForm::onFixMidiSetup()` reset the MIDI out notes of
  the current drumkit's instruments directly on the GUI's song copy and
  flipped the modified flag via the engine access — the first half never
  reached the engine's drumkit.
* New `CoreActionController::setDefaultMidiOutNotes(trigger)`: validates
  every instrument slot up front (a bulk reset must not land half-applied
  on a null slot), then resets each note to its list-slot default —
  `InstrumentList::defaultMidiOutNote(ii)` is the new single definition of
  the default formula (slot index + MIDI note offset, clamped), reused by
  `InstrumentList::setDefaultMidiOutNotes()`. Per changed instrument it
  pushes `InstrumentParametersChanged` (same event the scalar instrument
  setters use) and flips the drumkit modified flag through
  `setDrumkitModified(true, trigger)`; idempotent — an all-default kit
  changes nothing. `IpcCoreActionController` dual-applies: arg-less
  `SetDefaultMidiOutNotes` opcode + base call on the mirror; the bridge
  applies on the engine under `Suppress` (no SongIsModified echo).
* GUI reroute: `onFixMidiSetup()` calls
  `getCoreActionController()->setDefaultMidiOutNotes()` — the explicit
  `setDrumkitModified()` call is bundled in the command now; the infobar
  hide stays local. `tools/check_write_surface` gained
  `PATTERN_SET_DEFAULT_MIDI_OUT_NOTES` — the generic pointer pattern can
  not see through the `getInstruments()` getter chain, the new pattern
  keeps the direct call out of the GUI for good.
* Tested: new `EngineSessionTest::testSetDefaultMidiOutNotesCrossesSplit`
  (RED first: neither the command nor the default-note helper existed).
  Both sides' kits are broken to a single shared out note (the condition
  that makes the GUI offer the fix); the command must restore the
  per-slot defaults on the engine (pumped — bridge thread) and the mirror
  (synchronously, dual-apply) and flip both drumkits' modified flags.
  Three consecutive full-suite runs green.

**MIDI actions across the split (batch 2w) — DONE, suite
`OK (436 tests)`.**
* User report: `MainForm::executeShortcut` (keyboard shortcuts) triggered
  MIDI actions via `HydrogenApp::pHydrogen()->getMidiActionManager()` —
  the *mirror's* manager (`IpcEngineAccess::getMidiActionManager()` is a
  mirror passthrough, the exact analogue of the pre-ADR-0030 mirror-CAC
  bug). All 16 trigger sites applied to the mirror only — even the 30
  CAC-routed handlers, since the mirror's manager fetches the mirror's
  *local* controller. Engine-side paths stay untouched: `MidiInput`
  (every MIDI driver) and `OscServer` call the manager on their owning
  engine via the async worker queue.
* New `IEngineAccess::handleMidiAction(shared_ptr<MidiAction>)`:
  `LocalEngineAccess` forwards to the wrapped engine's manager
  (out-of-line in the .cpp — `Hydrogen.h` only fwd-declares the manager,
  same precedent as the db commands); `IpcEngineAccess` dual-applies —
  arg-carrying `HandleMidiAction` opcode `[type, value, instrument,
  component, layer, pattern, song, factor]` + synchronous mirror apply.
  The bridge reconstructs the action and runs the engine's own
  `handleMidiActionSync` on the bridge thread. Wire discipline: only
  parameters the action's type supports cross as non-zero —
  `MidiAction`'s getters *and* setters assert on unsupported params
  (found the hard way: an assert-abort mid-suite), so both sides gate on
  the instance's `Requires` mask; `value` is unguarded. Types outside
  the enum fail the dispatch-map lookup (graceful false).
* Editor-local classification: new static
  `MidiActionManager::isEditorLocal(Type)` — undo/redo only (they drive
  the editor's command stack; the engine holds none). Everything else
  crosses; engine-side event echoes re-fan-out on the editor as
  harmless redundant refreshes. Relative actions apply the same delta
  to both sides from the same base.
* GUI reroute: `executeShortcut` hoists `pAccess = HydrogenApp::pEngine()`
  and all 16 sites call `pAccess->handleMidiAction( pAction )` — action
  construction unchanged. `tools/check_write_surface` gained
  `PATTERN_MIDI_ACTION_MANAGER`: direct manager triggering (sync or
  async) is barred from the GUI for good.
* Tested: new `EngineSessionTest::testMidiActionCrossesSplit` (RED
  first: neither the access method nor the classification existed).
  `StripMuteToggle` must flip the engine's strip (pumped — bridge
  thread) and the mirror's (synchronously); `BpmIncr` must raise the
  engine's next BPM — the mirror's `bpmIncrease` is a designed no-op in
  editor mode (tempo source Remote, like `handleBeatCounter`), so the
  mirror's BPM lands via the engine's `TempoChanged` echo and its
  telemetry-based correction (pumped, not synchronous). Plus unit
  asserts on the `isEditorLocal` classification. Three consecutive
  full-suite runs green.
* Deferred (later-batch candidate): 12 of the 15 DIRECT-core handlers
  bypass CAC with direct Basics writes *inside* the engine too
  (engine-side MIDI/OSC path). Correct today; a write-surface-purity
  refactor onto their existing CAC equivalents.

**Rubberband batch recalculation across the split (batch 2x) — DONE,
suite `OK (437 tests)`.**
* User report: `MainToolBar::rubberbandButtonToggle` called
  `Drumkit::recalculateRubberband` directly on the GUI's song copy
  (under a hand-rolled audio engine lock) — only the mirror's samples
  got re-pitched. Engine-side paths stay untouched:
  `Transport::setBpm` (tempo change) and the export-session restore
  call it on the engine's own kit.
* New `CoreActionController::recalculateRubberband()`: resolves song,
  drumkit and the *local* playhead tempo inside the command (no wire
  arguments — each side's tempo is telemetry-close), locks its own
  audio engine (same shape as `panic()` and the export restore) and
  swaps the rubberband-enabled samples in memory. No events, no
  modified flip — the drumkit-internal `setSample` runs under
  `Suppress`, matching the pre-split GUI call. `IpcCoreActionController`
  dual-applies: arg-less `RecalculateRubberband` opcode + base call on
  the mirror; the bridge applies on the engine.
* GUI reroute: the toggle calls
  `getCoreActionController()->recalculateRubberband()`; the dead
  `pHydrogen`/`pSong`/`pDrumkit` locals are gone.
  `tools/check_write_surface` gained `PATTERN_RECALCULATE_RUBBERBAND`
  (direct drumkit-pointer calls barred; the generic pointer pattern
  can not see through the `getDrumkit()` getter chain).
* Tested: new `EngineSessionTest::testRecalculateRubberbandCrossesSplit`
  (RED first: the command did not exist). Both sides' kits are armed
  (batch mode on + instrument 0's first sample marked `bUse`); the
  command must swap the engine's layer sample (pumped — bridge thread)
  and the mirror's (synchronously, dual-apply) — observable as a new
  `shared_ptr<Sample>` in the layer, which lands even without the
  rubberband CLI (`Sample::load` only warns on CLI failure). Three
  consecutive full-suite runs green.
* Finding (pre-existing, resolved in batch 2y — corrected diagnosis
  there): the engine's copy of the batch-mode flag never armed, so the
  engine-side recalculation (this command *and* the tempo-change path)
  stayed inert in production. The original 2x diagnosis was wrong in
  one respect: there is no separate `m_nRubberBandBatchMode` — the
  getter/setter alias the Core-owned
  `m_bUseTheRubberbandBpmChangeEvent` schema row, which does cross
  with every `setPreferences` push. The gap was the write path: the
  toggle wrote the mirror's flag directly and no push followed.

**Preferences write-path crossings (batch 2y) — DONE, suite
`OK (439 tests)`.**
* User report (follow-up to the 2x finding): the rubberband batch
  mode must be exchanged via IPC; audit requested for other affected
  `Preferences` members.
* Audit: zipped the 136 schema rows (`PreferencesSchema::kSchemaRows`,
  55 `Owner::Core` / 81 `Owner::Gui`) against every `Preferences`
  access in `src/core` (the engine-side runtime surface). The Core
  set is complete for engine behavior — audio driver/buffer/rate,
  metronome, count-in, max notes, interpolation, all JACK flags, the
  full MIDI driver block, OSC, beat counter, rubberband CLI path,
  custom sound-library dirs, the pattern-editor grid/triplets/
  quantize/hear-new-notes flags (read engine-side in Hydrogen's
  realtime recording code), recent files, last song/playlist paths.
  The Gui-owned rows (theme, geometry, last-directories, export
  dialog state, sound-library display flags, shortcuts) have no
  engine readers. Two real gaps, both *write-path* gaps: the batch
  mode flag (aliasing the Core-owned row — see the corrected 2x
  finding above) was written directly on the mirror by the MainToolBar
  toggle and the export dialog's restore, with no `setPreferences`
  push following; and the punch-in/out markers
  (`m_nPunchInPos`/`m_nPunchOutPos` — runtime-only members on the
  outer `Preferences` class, outside `PreferencesData`/schema by
  design) are written by the SongEditor ruler only, while the
  engine's recording decision reads `inPunchArea()` live — punch
  recording was silently dead in the split.
* New granular commands (the `setMidiEventMap()` shape — installs on
  the local preferences, no driver restarts, no UpdatePreferences
  event): `CoreActionController::setRubberBandBatchMode(int)` and
  `setPunchArea(int,int)` — the punch pair crosses as one command so
  the engine never sees a half-updated area; an out position below
  the in position clears the area (every position records).
  `IpcCoreActionController` dual-applies both
  (`SetRubberBandBatchMode [mode]`, `SetPunchArea [in, out]` + base
  call on the mirror); the bridge installs them on the engine's
  preferences.
* GUI reroute: the MainToolBar toggle (both branches, dead `pPref`
  local gone), the export dialog's batch-mode restore, and all five
  ruler punch sites (dead `pPref` local in `mousePressEvent` gone).
  `tools/check_write_surface` gained
  `PATTERN_SET_RUBBERBAND_BATCH_MODE` and `PATTERN_SET_PUNCH_AREA`
  (direct preferences writes barred from the GUI for good).
* Tested: new `EngineSessionTest::testSetRubberBandBatchModeCrossesSplit`
  and `testSetPunchAreaCrossesSplit` (RED first: neither command
  existed) — each must land on the engine (pumped — bridge thread)
  and the mirror (synchronously, dual-apply); the punch test asserts
  the `inPunchArea()` semantics both defined (4,9) and cleared
  (0,-1 — every position records; the first test version had the
  clearing semantics backwards and the failure taught the real one).
  `IpcRoundTripTest::testPreferencesRoundTrip` now seeds a
  non-default batch mode so the core-props fragment carries the flag
  explicitly. Three consecutive full-suite runs green.
* Observation (pre-existing, not 2y's): two legacy IPC tests flaked
  once each across seven runs (`testSoundLibraryRescanCrossesSplit`,
  `testCommandDispatchedToEngine` — pump-timeout-shaped, on code 2y
  does not touch; both green in every other run). The 4 s
  `pumpUntil` budget gets tighter as the suite grows; a dedicated
  flakiness pass may be due.

**MIDI control dialog write paths (batch 2z) — DONE, suite
`OK (442 tests)`.**
* User report: `MidiControlDialog::persistMidiSettings()` only covered
  the MIDI instrument map, while the callbacks also change other
  MIDI-related `Preferences` members. Persisting those members and the
  `MidiInstrumentMap` had to be split into two methods, plus a
  `CoreActionController` method dual-applying the members currently
  only altered on the local mirror.
* Audit: seven scalar members were written on the mirror only — all
  Core-owned schema rows, all read live by the engine
  (`m_bMidiNoteOffIgnore` and `m_midiActionChannel` in MidiInput's
  dispatch, `m_bEnableMidiFeedback`/`m_midiFeedbackChannel` in the
  feedback sends, `m_bMidiTransportInputHandling` in MidiInput's
  transport handling, `m_bMidiTransportOutputSend` in AudioEngine's
  transport sends, `m_midiSendNoteOff` in Sampler/MidiOutput/
  MidiBaseDriver/SMF). The clock in/out toggles already cross via
  their dedicated commands. No other GUI site writes any of them.
* New granular command `CoreActionController::setMidiControlSettings(
  noteOffIgnore, actionChannel, enableFeedback, transportInputHandling,
  transportOutputSend, feedbackChannel, sendNoteOff)` — the
  `setMidiInstrumentMap()` shape (installs on the local preferences,
  no driver restarts, no UpdatePreferences event).
  `IpcCoreActionController` dual-applies it
  (`SetMidiControlSettings` + base call); the bridge installs the
  tuple on the engine's preferences.
* Dialog split: `persistMidiSettings()` now routes the seven scalars
  through the command — reading them from the dialog's widgets, which
  are the staging state, so no direct mirror writes remain — and then
  writes the config (command-before-save, so the config stores what
  the engine runs). `persistMidiInstrumentMap()` pushes the whole map
  (still staged in place on the mirror by the mapping callbacks) and
  saves. The eight map callbacks rerouted; the two channel spin boxes
  dropped their `valueChanged` mirror writes (the widget holds the
  intermediate state; the commit on focus-out/Enter crosses it, as the
  durable write always did).
* Clock mirror fix (folded in on user approval): both clock commands
  early-returned in `ProcessMode::Editor` *before* installing on the
  preferences — the toggle crossed to the engine, but the mirror kept
  the stale value and the dialog's `save(false)` wrote it back to the
  config (the toggle silently reverted on restart). Restructured: the
  install now happens in every process mode; Editor mode returns right
  after it (the authoritative engine applies the live side-effects
  when the IPC message lands); the unchanged-value check now only
  guards the side-effects.
* Tested: three new `EngineSessionTest` crossing tests. RED in two
  stages: the build failed on the missing command (EXIT=2), then —
  command implemented, clock fix not yet applied — the suite ran 442
  with exactly the two clock tests failing on the Editor-mode
  early-return. Three consecutive full-suite runs green after the fix.
  `IpcRoundTripTest::testPreferencesRoundTrip` seeds a non-default
  MIDI control tuple (note-off not ignored, action channel 3 — which
  also exercises the action channel's dedicated config codec — and
  send-note-off "never").
* `tools/check_write_surface` gained `PATTERN_SET_MIDI_CONTROL_SETTINGS`
  (direct member writes and setter calls on the live preferences
  barred from the GUI for good).
* Observation (pre-existing, left as-is): the mapping tab's global
  channel spin boxes still persist on every `valueChanged` tick, while
  the scalar channel spin boxes defer to commit — an inconsistency in
  write frequency, not in correctness.

**Automation path view write paths (batch 2aa) — DONE, suite
`OK (444 tests)`.**
* User report: `AutomationPathView` stores a shared_ptr to the current
  song's `AutomationPath` and its setters (`move()`, `addPoint()`,
  `removePoint()`) mutate that mirror object directly — the writes had
  to cross the IPC split and be undoable, via new `UndoActions.h`
  classes wrapping new `CoreActionController` methods.
* Audit: the view mutated the mirror's path at press/drag/keypress and
  only crossed at mouse-release — `SongEditorPanel` pushed
  `SE_automationPath*Action`s on the view's `pointAdded/Removed/Moved`
  signals, whose redo-on-push re-applied onto the already-mutated
  mirror (the add overwrote idempotently, the move's remove leg and
  the remove's own leg no-opped on the missing point). A move crossed
  as a remove+add pair (two commands, non-atomic). Two edges were
  outright broken: a release outside the widget bounds never emitted
  the signal, so a press-added point never reached the engine at all
  (and wasn't undoable either); and the grab-move captured the origin
  *after* the press-snap, so undo restored the snap position instead
  of where the point actually came from. The add/remove commands also
  carried no validation (an occupied-x add silently overwrote the
  sitting point, an absent-x remove silently succeeded) and had no
  test coverage.
* New atomic command `CoreActionController::moveAutomationPoint( oldX,
  oldY, newX, newY )` with full validation: refused when no point sits
  at the exact source coordinates (new `AutomationPath::findExact()` —
  the tolerance-based `find()` could grab a neighbouring point), when
  the source value differs from `oldY` (a stale command), or when
  another point occupies `newX` (`AutomationPath::move()` erases
  before inserting and `std::map::insert` does not overwrite — the
  moved point would be silently dropped). `addAutomationPoint` now
  refuses an occupied x, `removeAutomationPoint` an absent point.
  `IpcCoreActionController` dual-applies the move
  (`MoveAutomationPoint`, four float args); the bridge case applies it
  on the engine.
* View reroute: the three undo classes lost their unused `m_pPath`
  member and the legacy `pHydrogen()` accessor (now `pEngine()`), and
  the move action wraps the atomic command. The view pushes the
  commands as the *only* write path — add at press, one move per drag
  step, remove at keypress — under a shared `automationPath:edit`
  undo context closed in `mouseReleaseEvent()` (also on the
  out-of-bounds path), coalescing a whole press-drag-release
  interaction into a single undo step, mixer-style. The grab-move now
  captures the true pre-snap origin. The `pointAdded`/`pointRemoved`/
  `pointMoved` signals, the `SongEditorPanel` slots and their
  connections are gone (the commands fire at interaction time now);
  the direct `setSongModified()` pokes went with them — the commands
  mark it on both sides.
* Tested: two new `EngineSessionTest` crossing tests (add/remove
  including the refusals; the atomic move including stale-source,
  absent-source, occupied-destination and y-only cases). RED in two
  stages: the build failed on the missing command (EXIT=2), then —
  command plumbed, validation not yet applied — the suite ran 444
  with exactly the two new tests failing on the refusal asserts. Three
  consecutive full-suite runs green after (a fourth run hit a one-off
  crash in the pre-existing
  `ConnectViaIpcModeTest::testHandledMidiLogClearOrdering` — "pure
  virtual method called" mid-`pumpUntil`, outside this batch's call
  graph; same family as the batch 2y flakiness observation, but a
  crash rather than a pump timeout — worth its own pass).
* `tools/check_write_surface` gained
  `PATTERN_AUTOMATION_PATH_DIRECT_WRITES` (direct
  `addPoint`/`removePoint`/`move` calls on path receivers barred from
  the GUI for good).
* Behavior notes: dragging a point onto an occupied x now refuses the
  step (the point stays put) instead of the old silent drop — the
  latent erase-without-overwrite data loss; after a refused step the
  view's selection follows the tolerance `find()` and grabs the
  occupying point, the nearest sane behaviour for a measure-zero
  mouse collision.

**T5.3 editor-mode bootstrap — DONE, suite `OK (318 tests)` + ctest 5/5.**
* New `--plugin-editor <endpoint>` CLI option (`Parser`, hidden from help).
* New core helper `EditorSession` (`src/core/IPC/`): `connect(endpoint, mirror)`
  opens an `IpcChannel`, attaches an `EditorStateMirror` (inbound events / song
  snapshots → mirror), and sends the `hello`. `createEngineAccess()` yields the
  IPC-backed `IpcEngineAccess` the GUI fans out from. Owns the channel + state
  mirror; not the mirror engine.
* `main.cpp` branches on the endpoint: in editor mode it builds a headless mirror
  (Fake driver, no NSM, no local audio driver), attaches via `EditorSession`, and
  injects through the new `HydrogenApp::setEditorBootstrap(mirror, access, prefs)`
  — which generalised `m_pEngineAccess` from `LocalEngineAccess` to the
  `IEngineAccess` interface so `pEngine()` returns either backing. A failed
  connection aborts with a logged message (verified via the GUI binary).
* The GUI itself is unchanged: the de-singletoning is complete (0 direct
  `Hydrogen::get_instance()` calls in `src/gui`, 397 `pEngine()` fan-outs), so it
  transparently reads the mirror and writes over IPC.
* `EditorModeTest` (5 cases): attach + hello handshake; graceful failure on a bad
  endpoint; inbound song-snapshot + event applied to the mirror; command issued
  via the engine-access reaches the engine; **engine survives editor disconnect**
  (the server keeps listening and accepts a reattaching editor).

**T5.4 editor lifecycle — mostly DONE (both ends of the serve loop built).**
* **Editor side** (`EditorSession`): connect / inbound-sync / command forwarding /
  clean teardown / engine-survives-disconnect — done and tested (`EditorModeTest`).
* **Engine side** (`EngineSession`, `src/core/IPC/`): the serve loop, the
  host-facing counterpart. `start(engine, endpoint)` listens and runs a dedicated
  **bridge thread** (off the audio thread, ADR 0018) that accepts the editor,
  answers the `hello`, sends the current song as initial state, then loops:
  receive → `IpcEngineBridge::dispatchCommand` (commands) / `handleRequest`
  (request-response, correlated by `requestId`), and drains the engine
  `EventQueue` → `forwardEvent` (engine-origin events only). On editor disconnect
  it returns to accepting, so a respawned editor re-attaches (engine survives
  editor crash). The `IpcServer` + accepted channel live entirely on the bridge
  thread (QLocalSocket is thread-affine); a `std::promise` reports listen success
  to `start()`. Covered end-to-end by `EngineSessionTest` (5 cases: arg
  rejection, initial-state priming, command dispatch, event forwarding,
  survive+reconnect) driving a real `EngineSession` ↔ `EditorSession` pair.
* **Plugin integration** (`HydrogenPlugin`, `src/plugin/`): the per-instance
  plugin engine wrapper now owns the editor lifecycle (ADR 0016).
  `openEditor()` generates a unique endpoint, starts an `EngineSession` serving
  this instance's engine, and spawns `hydrogen --plugin-editor <endpoint>` as a
  separate process; `closeEditor()` terminates the process and stops the serve
  loop; the dtor closes it. Auto-respawn (cap 3) relaunches the editor **only on
  a crash** (a clean exit — the user closed the window — leaves it closed). The
  editor binary is resolved from `setEditorBinary()` → `$HYDROGEN_EDITOR_PATH` →
  `hydrogen` on PATH. `openEditor()` first ensures a `QCoreApplication` exists
  (creates a minimal one if the host is non-Qt) so Qt's QProcess / local-socket
  classes work. Covered by `PluginLifecycleTest` (open-serves-engine,
  command-reaches-engine, reopen) which attach a real `EditorSession` to the
  plugin's endpoint with the process spawn suppressed (the test acts as the
  editor).
* **CLAP gui extension** (`HydrogenClap.cpp`): the host's show/hide UI is wired to
  the editor. Using CLAP's **floating** model (ADR 0016 — the editor is its own
  top-level window, not embedded): `is_api_supported`/`get_preferred_api` advertise
  floating on the platform window API; `create` allocates nothing; `show` →
  `openEditor()`, `hide`/`destroy` → `closeEditor()`; the embedded methods
  (`set_parent`/size/resize) report unsupported. Spec-validated by the
  `clap-validate` ctest (which skips actually opening the window, so no spawn in
  CI).
* **LV2 UI** (`HydrogenLv2.cpp` + `hydrogen.ttl`): a non-embedding UI via LV2's
  `ui:showInterface` (driven by the mandatory `ui:idleInterface`). The UI obtains
  the in-process engine through the `instance-access` feature and maps `show` →
  `openEditor()`, `hide` → `closeEditor()`, and `idle` → poll
  `isEditorProcessRunning()` (returns non-zero when the user closed the editor
  window, so the host hides + can re-show). `lv2ui_descriptor` ships in the same
  module; the TTL declares the platform UI type (X11UI/CocoaUI/WindowsUI via
  cmake) + the two interfaces. Passes `lv2lint` + `lv2-smoke` (lv2lint warns —
  accepted — that instance-access and same-binary DSP/UI are discouraged; both
  are intrinsic to driving an out-of-process *local* editor).
* **Editor-binary discovery** (`HydrogenPlugin::resolveEditorBinary`): the plugin
  locates the editor relative to its own install instead of relying on PATH.
  Precedence: explicit `setEditorBinary()` → `$HYDROGEN_EDITOR_PATH` → a
  `hydrogen[.exe]` found next to the plugin (or `../bin`, or the macOS `.app`) →
  `hydrogen` on PATH. The plugin's own location is captured per format — CLAP from
  `entryInit(plugin_path)`, LV2 from `instantiate(bundle_path)` — and handed to
  the engine via `setEditorSearchDir()`. The resolver is unit-tested
  (`PluginLifecycleTest::testEditorBinaryDiscovery`). The exact bundle layout is
  finalised by packaging (T6.2).
* **Still deferred:** the VST3/packaging work (T6.1–T6.3).

**Editor Preferences UI — host-owned controls read-only — DONE.** Added
`HydrogenApp::isEditorMode()` (static flag set by `setEditorBootstrap`). In editor
mode `PreferencesDialog` presents the host-owned configuration **read-only**
(`setEnabled(false)`, value still visible) rather than hidden — per the user's
preference and ADR 0026: audio driver / device / sample rate / buffer size + JACK
options, the MIDI driver + in/out ports, and the OSC server controls (the set
tracks `PluginConfig::isOverridePath`). Whole-tab hiding was rejected because the
Audio tab also holds user-level settings (metronome volume, polyphony,
interpolation) and the MIDI tab holds non-host MIDI prefs, which stay editable.
i18n-safe (no new strings). Standalone unaffected (`GuiStartup` green).

**Layered-config consumption — deferred (no host yet).** Persistence is wired
(`Preferences` retains the load baseline at `Preferences.cpp:464`;
`PluginConfig::persist` runs the locked 3-way merge at `Preferences.cpp:1895`).
`PluginConfig::applyOverride` (the host pushing its audio/MIDI/JACK override layer
onto the editor's base config at startup) has no caller yet — it activates when a
real host launches the editor with a config override.

**Tests first**
* `IpcProtocolTest` (unit): every `CoreActionController` command and every
  `Event::Type` round-trips through the message codec; telemetry shared-memory
  struct is layout-versioned and back/forward-compatible.
* `PluginConfigTest` (unit): base ← override layering keeps base fields
  (theme/shortcuts/language) from the shared config while the override subset
  (audio/MIDI I/O, JACK/OSC, recent/last-file) takes host/state values; override
  subset excluded from shared-config writes; a base-layer change persists and
  survives reload.
* `ConfigConcurrencyTest` (unit): two `Preferences` changing *different* base
  fields then persisting concurrently → both survive (atomic+locked field merge);
  same field → bounded last-writer-wins, never corruption.
* `EditorModeTest` (integration, Linux): `hydrogen --plugin-editor <endpoint>`
  attaches to a fake engine endpoint and builds `MainForm` **without** a local
  engine/driver; live session receives events and issues commands; engine keeps
  running if the editor disconnects/crashes.

**Tasks**
* **T5.1 IPC transport** (ADR 0018): `QLocalSocket` control channel using
  **length-prefixed `QDataStream` framing** (pinned version; `hello` handshake) —
  commands = `CoreActionController` vocabulary, events = `EventQueue` marshalling,
  large Song/Drumkit/state payloads as a `QByteArray` of the existing XML — plus a
  **seqlock'd shared-memory telemetry block** (`formatVersion`-validated POD;
  playhead, peaks, process time; per-instrument peak cap 256). An IPC bridge
  thread drains `EventQueue` off the audio thread. **Tag each `Event::Type` as
  engine-origin (marshalled) or editor-internal (stays local)** — the bridge
  forwards only engine-origin events. *Tests first:* extend `IpcProtocolTest` —
  command/event round-trip, tear-free telemetry read, version-mismatch fallback,
  and `OnlineImportProgress` (editor-internal) not marshalled while engine-origin
  events are.
* **T5.2 `IpcEngineAccess`** — second `IEngineAccess` implementation (from Phase
  2) backed by the IPC client, so the GUI is unchanged.
* **T5.3 Editor mode — ✅ DONE.** `--plugin-editor <endpoint>` in
  `Parser`/`main.cpp`: skip the local driver; build a headless mirror; attach via
  the new `EditorSession`; inject `IpcEngineAccess` through
  `HydrogenApp::setEditorBootstrap`; build the unchanged `MainForm`. Covered by
  `EditorModeTest`.
* **T5.4 Editor lifecycle — ◑ MOSTLY DONE.** Both ends of the serve loop are
  built: editor side (`EditorSession`, `EditorModeTest`) and engine side
  (`EngineSession` serve loop on a bridge thread — accept, handshake, initial
  state, command dispatch + request/response, EventQueue forwarding, survive +
  reconnect; `EngineSessionTest`). Deferred to the real plugin host: spawning /
  respawn-supervising the editor *process* and a host-side `QCoreApplication` for
  Qt sockets (only `FakePluginHost` exists today).
* **T5.5 Layered config** (ADR 0022): base from shared `~/.hydrogen`; override
  subset from host/state; retained load baseline on `Preferences` — all ✅ DONE.
  Editor's Preferences UI presents host-owned controls **read-only** (not hidden)
  via `HydrogenApp::isEditorMode()` — ✅ DONE. Remaining: the host pushing its
  override layer at editor startup (`PluginConfig::applyOverride` caller) — gated
  on a real plugin host.
* **T5.6 Concurrency-safe persistence** (ADR 0023): replace the snapshot-on-close
  `save()` on the shared-config path (`Preferences.cpp:1405`; callers
  `main.cpp:534`, `MainForm.cpp:2758`) with `QLockFile` + `QSaveFile` + a 3-way
  field merge whose changed-field set is computed by **diff-against-baseline** (no
  dirty-set); debounced write-through during the session; teardown flushes pending
  changes only.
* **T5.7 Sound-library across the split** (ADR 0016): keep `OnlineImporter` +
  `OnlineImportDialog` **editor-side** (their Qt signals / `EventQueue` interplay
  stay in-process — no marshalling); after an import batch, send a **"rescan sound
  library" IPC command** so the engine refreshes its `SoundLibraryDatabase` and
  can load newly installed kits/songs. *Tests first:* extend `IpcProtocolTest`
  (rescan command round-trips) and an integration check that an editor-side
  install becomes loadable engine-side after the rescan. The CLI keeps using
  `OnlineImporter` directly (standalone/headless) — unchanged.

**Done when:** unit tests green on all platforms; integration tests green on
Linux.

---

## 10. Phase 6 — VST3 via clap-wrapper & packaging

ADR 0014, proposal 0003 §9.

**Status: 🚧 IMPLEMENTATION COMPLETE — pending artifact-build verification.** T6.1
(VST3 via clap-wrapper) ✅; T6.2 packaging ✅ (Linux `.tar.xz`, macOS `.dmg`
staging, Windows NSIS install rules); T6.3 CI ✅ wired in `.appveyor.yml` (CLAP/LV2
always, VST3 artifact-gated, `vst3-validate` + conformance tools). The first real
CI runs (Linux/macOS/Windows) were exercised and the defects they surfaced are
fixed (see "Fixes from the first real CI runs" below). Everything buildable/runnable
on the Linux dev box is green (`build.sh r m t`: OK(329) + ctest 9/9 + Steinberg
validator 47/47). **What's left is the artifact-gated packaging path on a real
`*-artifacts` run** — see "Remaining" below.

**Tests first**
* VST3 validator smoke step in CI — ✅ done as the `vst3-validate` CTest (T6.3).
* `windows/ci/test_installation.py` extended to assert the installed plugin
  bundles — proposed but **reverted by the maintainer**; the Windows installer
  verification currently checks the app + data only. (A confirm-on-artifact-run
  item; see Remaining.)

**Tasks**
* **T6.1 — ✅ DONE.** `clap-wrapper` (free-audio, `extern/clap-wrapper` submodule,
  pinned **v0.10.1** — newer tags require CMake ≥ 3.27 via `CMP0149`; we're on
  3.25) wraps the native CLAP (`clap/HydrogenClap.cpp` → `clap_entry`) into
  `Hydrogen.vst3`. Behind `-DWANT_VST3=1` (opt-in; it fetches the Steinberg VST3
  SDK via CPM at configure time), `src/plugin/CMakeLists.txt` points clap-wrapper
  at our vendored CLAP SDK (`CLAP_SDK_ROOT`), `add_subdirectory`s it, and calls
  `target_add_vst3_wrapper(TARGET hydrogen-vst3 OUTPUT_NAME Hydrogen)`. Build
  details worked through: the CLAP entry is compiled in its own static lib so it
  isn't subjected to clap-wrapper's PUBLIC `-Wall -Werror`; `-Wno-format-security`
  on the SDK/wrapper targets suppresses the clash between the SDK's `-Wno-format`
  and our global `-Werror=format-security` (suppressing the warning entirely, not
  just demoting it to a per-TU error/warning); clap-wrapper's own **blanket
  `-Werror`** (an INTERFACE option it forces, untoggleable) is stripped from
  `clap-wrapper-compile-options` after `add_subdirectory` — otherwise its vendored
  **fmt v9** fails to compile on GCC 13/14 (`-Wdangling-reference` /
  `-Wtautological-compare`), and a per-target `-Wno-error` can't override an
  INTERFACE flag (it lands last) — and on GCC the now-non-fatal fmt warnings are
  also silenced (`-Wno-dangling-reference -Wno-tautological-compare`, guarded to
  GNU since clang doesn't know them); and `CMAKE_POSITION_INDEPENDENT_CODE` makes
  the wrapper static libs linkable into the shared module. Produces a valid Linux
  bundle (`Hydrogen.vst3/Contents/x86_64-linux/Hydrogen.so` exporting
  `GetPluginFactory`/`ModuleEntry`); `vst3-smoke` ctest checks the entry export.
  The CLAP gui (floating) / LV2 UI exposure from Phase 5 carries into the VST3
  since it wraps the same CLAP entry.
  **VST3 conformance (passes the full Steinberg validator, 47/47):** wiring the
  validator (T6.3) surfaced three issues, all fixed on our side (clap-wrapper kept
  pristine): (a) **null factory** — clap-wrapper only binds our statically-linked
  `clap_entry` under `STATICALLY_LINKED_CLAP_ENTRY`, which `target_add_vst3_wrapper`
  never defines; we set it on `clap-wrapper-shared-detail` (the target that
  compiles `fsutil.cpp`) + the vst3 lib; (b) **link** — that reference would drop
  the entry from a STATIC lib, so `hydrogen-vst3-entry` became an OBJECT library
  (its objects are always linked; portable, no `--whole-archive`); (c) **MIDI
  Mapping** — clap-wrapper registers its MIDI-CC→VST3 mapping params inside
  `setupParameters()`, called only when the CLAP plugin exposes a params
  extension; Hydrogen had none (ADR 0021), so we add a minimal empty params
  extension (count 0), **scoped to the VST3 build only** (`H2_CLAP_FOR_VST3`,
  set on `hydrogen-vst3-entry`). It must NOT be exposed by the native CLAP plugin:
  doing so makes clap-validator enable its param-fuzz / state-reproducibility test
  suites (skipped for a parameterless plugin), which is out of scope and breaks
  `clap-validate`. Net: `build.sh t` is green — clap-validate passes, VST3
  validator 47/47.
* **T6.2 — 🚧 Linux + macOS DONE; Windows via existing NSIS.** Packaging per platform.
  **Linux**: `install(TARGETS/DIRECTORY ...)` rules in `src/plugin/CMakeLists.txt`
  stage all three bundles under the standard plugin dirs (`lib/clap/Hydrogen.clap`,
  `lib/lv2/hydrogen.lv2/`, `lib/vst3/Hydrogen.vst3/` — the VST3 source path uses a
  `$<CONFIG>` genex), alongside `bin/hydrogen` (the editor) which the installed
  plugin discovers by walking up to `bin/` (ADR 0016). CPack `TXZ` generator emits
  a relocatable `hydrogen-<ver>-linux-<arch>.tar.xz`. Required fixing several
  **pre-existing absolute-path install bugs** that broke staged/CPack installs:
  icon (`CMAKE_INSTALL_FULL_DATAROOTDIR`→`DATAROOTDIR`), i18n + doc
  (`H2_SYS_PATH`→`H2_DATA_PATH`), and a literal-quote bug in the version string
  (`git log --pretty=format:'%h'` → `:%h`; `execute_process` runs git without a
  shell). Verified: `cmake --install --prefix /tmp/h2stage` produces the correct
  layout and the 46 MB `.tar.xz` contains `bin/hydrogen`, all 3 plugin bundles, and
  the 21 `.qm` translations. (Packages against **system Qt** — full Qt bundling for
  true relocatability rides the existing AppImage/linuxdeploy path, follow-up.)
  **macOS**: `macos/build_dmg.sh` gained a `-p <dir>` option that discovers the
  built `Hydrogen.clap` / `Hydrogen.vst3` / `hydrogen.lv2` bundles (top-level or
  under a per-config subdir) and stages them into a `Plugins/` folder in the disk
  image, with a `README.txt` telling the user which `~/Library/Audio/Plug-Ins/`
  dir each goes in. Like Linux, the bundles ship against the app's Qt (fully
  Qt-bundled, self-contained plugins are the same follow-up). Logic unit-tested on
  Linux; the `hdiutil`/`macdeployqt` parts can only run on macOS CI.
  **Windows**: the `src/plugin` `install()` rules place the bundles under the
  install root (`H2_LIB_PATH="."` on MINGW → `clap/`, `lv2/`, `vst3/` next to
  `hydrogen.exe`), so the existing `cpack -G NSIS` packages them once the plugins
  are built. Note: the CLAP target is a MODULE library whose `.clap` is a LIBRARY
  artifact on Linux/macOS but a RUNTIME (DLL) artifact on Windows — so a
  `RUNTIME DESTINATION` is needed for the `.clap` to land in the Windows package
  (a `RUNTIME`-adding fix was proposed but **reverted by the maintainer**, so
  confirm/redo this when the Windows artifact build is exercised). (Placing
  bundles into the system plugin scan dirs — `%COMMONPROGRAMFILES%\{VST3,CLAP}` —
  via NSIS extra commands is a possible enhancement; the current "bundle under
  install dir" matches Linux.)
* **T6.3 — 🚧 wired in `.appveyor.yml` (CI is AppVeyor, not GH Actions).**
  **CLAP + LV2** are built on every Linux (Ubuntu 22.04 test job), macOS and
  Windows run — `-DWANT_CLAP=ON -DWANT_LV2=ON` — so the plugin CTest suite runs
  alongside the unit tests on all three (extended the `ctest -R` regex on each).
  On the **Linux test job** the conformance tools themselves are built so those
  tests actually execute (not skipped): the job apt-installs `cargo` + `meson`/
  `ninja-build` + `lv2-dev`/`liblilv-dev` (+ curl/elf/x11 for lv2lint's optional
  groups) and runs `./build.sh deps` before configure, which builds clap-validator
  (cargo) and lv2lint (meson) into the paths CMake `find_program`s. `CARGO_HOME`
  points at the cached dir so the crate registry persists (the Rust *target* still
  recompiles per run — caching it via `CARGO_TARGET_DIR`+PATH is a possible
  speed-up follow-up). macOS/Windows don't build the tools (would need brew/MSYS
  toolchains; conformance is platform-agnostic and covered on Linux), so
  `clap-validate`/`lv2lint` stay skipped there.
  The bundles ride the existing artifacts: into the macOS `.dmg` (via
  `build_dmg.sh -p .`) and the Windows NSIS installer (via the `install()` rules);
  the AppImage job stays the standalone-app artifact. **VST3** is **gated behind
  `UPLOAD_ARTIFACTS`** on every platform (heavy — clap-wrapper fetches the
  Steinberg VST3 SDK at configure): on macOS/Windows it is built, packaged and
  uploaded on artifact builds; on Linux it is built **only as a clean-compile
  check** (no bundle/archive uploaded — but it *is* validated, see below).
  **Full Steinberg VST3 validator**: `tools/run_vst3_validator.sh <sdk-src>
  <bundle.vst3>` configures the SDK source clap-wrapper already fetched
  (`<build>/clap-wrapper/cpm/vst3sdk`, v3.7.6) as a standalone project — which
  builds the SDK's `validator` tool unconditionally — with VSTGUI + plug-in
  samples off, then runs it on the bundle (non-zero exit on any failed test).
  Wrapped as a CTest **`vst3-validate`** (UNIX, registered under `WANT_VST3`)
  alongside `clap-validate`/`lv2-smoke`/`vst3-smoke`: its command is
  `run_vst3_validator.sh`, which builds the SDK validator on demand and runs it on
  the bundle. So it runs from the normal ctest suite — `build.sh t` (which echoes
  the validator's `N tests passed` summary afterwards, since CTest hides a passing
  test's stdout) and the CI `ctest -R …|vst3-validate` (registered only on the
  artifact builds that build VST3, so it no-ops elsewhere). Verified on Linux:
  ctest 9/9, and the Steinberg validator reports **47 tests passed, 0 failed** on
  the Hydrogen VST3 bundle (after the T6.1 conformance fixes). The helper needed
  several robustness fixes found during verification: it fetches the SDK's `cmake`
  submodule on demand (clap-wrapper omits it), configures with
  `-DSMTG_ADD_VST3_HOSTING_SAMPLES=OFF` (drops the GTK-dependent editorhost
  sample), and force-includes `<cstdint>` (`-DCMAKE_CXX_FLAGS=-include cstdint`)
  because the pinned SDK's `moduleinfo.h` uses `uint32_t` without the include and
  GCC 13+ no longer pulls it in transitively; it also accepts a build root and
  locates the `.vst3` itself (robust to Debug/Release). Windows (MinGW) validator left as a follow-up — building the
  Steinberg validator under MinGW is unproven.
  Unverifiable from the Linux dev box — the first real AppVeyor run is the
  validation; known risks to watch: Ubuntu 22.04's CMake (3.22) vs clap-wrapper's
  floor, clap-wrapper/Steinberg-SDK on MinGW, and the SDK hosting-utilities
  subdir building cleanly with VSTGUI off. Remaining: confirm on a real CI run.

**Fixes from the first real CI runs & editor runtime testing (2026-06-26/27).**
The CI was exercised and several real defects surfaced and were fixed (all green
again — `build.sh r m t`: OK(329) + ctest 9/9):
* **macOS unit tests (IPC large payloads).** `EditorModeTest::testReceivesEngineState`
  + `IpcTransportTest::testProxySetSongPayload`/`testProxyObjectPayloadCommands`
  failed on macOS: a synchronous `send()`-then-`receive()` of a payload larger
  than the socket buffer deadlocks when the peer isn't draining concurrently.
  macOS's default Unix-socket buffer (~8 KB) is far smaller than a Song/Drumkit
  XML (~66–76 KB); Linux's ~208 KB hid it. Fix: `IpcChannel` enlarges
  `SO_SNDBUF`/`SO_RCVBUF` (8 MB, OS-clamped) on Unix so the frame fits in one
  flush (`src/core/IPC/IpcChannel.cpp`).
* **Windows build (LV2 smoke host).** Enabling `WANT_LV2` on Windows for the first
  time compiled `lv2_smoke.cpp`, which used POSIX `<dlfcn.h>` (absent on MinGW).
  Fix: a cross-platform `dlOpen/dlSym/dlClose` shim (Win32 `LoadLibrary` / POSIX
  `dlopen`). The rest of `src/plugin` is Qt-based and portable.
* **`vst3-validate` picked the wrong bundle.** clap-wrapper also emits an example
  `clapasvst3.vst3` (its `.so` not built); `run_vst3_validator.sh` now matches the
  specific `Hydrogen.vst3` name (a bare `*.vst3` glob `dlopen`-failed on it).
* **Linux validator on GCC 13+.** The pinned SDK's `moduleinfo.h` uses `uint32_t`
  without `<cstdint>`; the helper force-includes it (`-include cstdint`).
* **`--plugin-editor <bad-endpoint>` segfault (editor mode, Phase 5).** The
  headless mirror was created with the *Fake* audio driver, which spawns a
  processing thread; on the failed-connect abort, `main()` freed the Logger while
  that thread was still logging → use-after-free. Fix:
  `EditorSession::configureMirrorPreferences()` gives the mirror the passive
  **Null** driver (no thread), no MIDI, no OSC — shared by `main()` and the tests;
  the abort path also deletes the engine before the Logger/QApplication. Guarded by
  `EditorModeTest::testMirrorUsesPassiveAudioDriver` + the `EditorBadEndpoint` ctest.
* **Reporter treated a clean failed-connect as a crash.** The crash reporter
  popped "Hydrogen exited abnormally" for any non-zero child exit. Fix:
  `Reporter::EXIT_CODE_CLEAN_FAILURE` (used by the editor abort) +
  `Reporter::shouldReportCrash()` — report only actual crashes (`CrashExit`) or
  *unexpected* non-zero exits. Guarded by the `EditorReporterCleanFailure` ctest.

**Remaining (verification on infrastructure the Linux dev box can't reach — for
the maintainer to run):**
1. **An artifact AppVeyor run** (`*-artifacts` branch / tag). The per-commit
   Linux/macOS/Windows builds + tests now pass (the fixes above); what's still
   unexercised is the artifact-gated VST3 + packaging path. Watch the flagged
   risks: Ubuntu 22.04 CMake (3.22) vs clap-wrapper's floor, and clap-wrapper +
   Steinberg SDK building under **MinGW** (Windows VST3 still unproven).
2. **macOS `.dmg`** — confirm `build_dmg.sh -p` + `macdeployqt`/`hdiutil` produce a
   working image with the `Plugins/` folder on a real macOS runner.
3. **Windows VST3** — confirm clap-wrapper's VST3 wrapper builds under MinGW at
   all, the single-file `Hydrogen.vst3` installs (the `install(DIRECTORY …)` rule
   assumes a bundle dir — may need a Windows branch), and add the MinGW validator
   step once the build is proven.

**Follow-ups (explicitly out of the core scope, tracked):**
* **Full Qt bundling** for self-contained plugins on every OS (currently ship
  against system/app Qt) — rides the linuxdeploy/macdeployqt/windeployqt paths.
* Optional: install plugins into the OS scan dirs (`%COMMONPROGRAMFILES%\{VST3,
  CLAP}`, `~/Library/Audio/Plug-Ins/…`) rather than next to the app.
* Optional CI speed-up: cache the Rust `CARGO_TARGET_DIR` (+PATH) so clap-validator
  builds incrementally instead of recompiling each Linux run.

**Done when:** CLAP/LV2/VST3 all pass validators (✅ Linux, incl. the
Steinberg validator 47/47; ⏳ confirm on a CI artifact run); installers place
bundles correctly (✅ Linux `.tar.xz`; ⏳ macOS `.dmg` + Windows NSIS on an
artifact run — incl. the Windows CLAP `RUNTIME DESTINATION` to redo); artifacts
produced only on tag/`*-artifacts` builds (logic in place; ⏳ confirm on a real
run).

---

## 11. Phase 7 — Multi-instance hardening, host matrix & convenience features

ADR 0015, 0019, proposal 0003 §11.

**Tests / activities**
* Stress: many instances instantiate/process/destroy in a loop (extends
  `PluginLifecycleTest`/`MultiInstanceTest`); CPU/RT validation under N instances.
* Host matrix: Ardour, Qtractor, Reaper, Bitwig, Cubase, Live — load, play,
  save/restore project, reopen; verify state round-trip and per-instance config
  persistence under parallel close.
* Finalise user-facing limitations (proposal 0003 §7).

**Late-stage tasks** (depend on the editor from Phase 5):
* **T7.1 Output-bus remapping widget** (ADR 0019) — *pure convenience, optional
  for first ship.* *Tests first:* extend `OutputBusTest` (custom mapping, incl.
  several instruments deliberately on one bus, routes correctly, persists,
  survives add/remove/move/reload). *Implementation:* a GUI widget to reassign
  instruments to buses; persist the custom `Instrument::Id`→bus mapping in plugin
  state under format versioning; feeds the CLAP/VST3 rename hook (T4a.3).
  Especially valuable on LV2, whose bus names are fixed. The 1-to-1 default ships
  without this.
* **T7.2 Plugin-mode GUI adaptation** ([ADR
  0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)) —
  *required for a clean integration (the core features are already disabled in
  T3.5, so a stale control is harmless but confusing until hidden).* Gate the
  following on the "running as plugin" predicate: **PreferencesDialog** — hide
  conflicting options, show host audio settings (sample rate, buffer size)
  read-only (cf. [ADR
  0022](/docs/decisions/0022-layered-plugin-configuration.md)); **no
  PlaylistEditor**; **disable Timeline / Tempo Markers** (and tags, for
  consistency); **main-menu transport** — hide most actions, keep play/pause +
  loop as read-only state indicators (loop shown always-on); **main toolbar** —
  hide BPM, tap-tempo, beat-counter widgets; **song/pattern-editor rulers** — no
  click-to-relocate and drop the hover affordance that advertises it.
  **MidiActionTable**: filter out actions omitted by `MidiActionManager`
  *Tests:* GUI gating is largely visual; cover what is unit-testable (the
  predicate toggles widget visibility/enabled state; ruler relocation is a no-op
  in plugin mode) and verify the rest in the host-matrix pass.

**Done when:** green across the host matrix; no state cross-contamination; config
survives parallel teardown in a real DAW; the editor presents no controls that
contradict the host (T7.2). (T7.1 is optional for the first release; T7.2 is
required for a proper integration.)

---

## 11.6 Phase 8 — Engine clock decoupled from the audio driver — ADR 0031

The `AudioDriver` conflates the audio-output sink with the engine **clock** (the
caller of `audioEngine_process(nframes)` that advances transport + runs
notes/MIDI). So "no audio driver" means "no clock" and the engine freezes — which
blocks MIDI-only operation and leaves the editor mirror's transport dead.
[ADR 0031](/docs/decisions/0031-decouple-engine-clock-from-audio-driver.md)
decouples them: one **always-clocked software driver** whose audio output is
optional, headless as the new fallback, and a hybrid editor-transport sync.
Depends on Phase 3 (PluginAudioDriver / host-transport follower) and Phase 5
(editor split + telemetry block).

**Tests first**
* Software driver advances transport on its own clock: a `Hydrogen` with the
  software driver, set Playing, observe the transport frame increase over real
  time with no real audio — and stop cleanly (thread joined, no leak; extends
  `PluginLifecycleTest`/`AudioDriverTest`).
* `audioEngine_process` is output-optional: with `producesAudio=false`, notes
  still process and `handleNote` still emits MIDI / advances note lifetime, but no
  real output buffer is written.
* **MIDI-only standalone**: software audio driver + a MIDI driver → a played note
  emits MIDI note-on/off, transport advances, no audio buffers touched.
* **Headless fallback**: a failing real driver (forced) falls back to a *clocked*
  headless driver — the engine still processes — not a frozen `NullDriver`;
  `AudioDriverInfo` reports `isPresent && !isRunning`.
* **Editor hybrid sync**: the mirror's transport free-runs at the host rate, a
  transport event relocates it immediately, and a telemetry frame forces a re-sync
  (drift bounded) — new `EditorMirrorTest`/`EditorModeTest` cases.

**Tasks**
* **T8.1 — ✅ consolidate the software driver.** Added `SoftwareDriver`
  (`src/core/IO/SoftwareDriver.{h,cpp}`, the generalised `FakeAudioDriver`): the
  always-clocked timer pump with `{sampleRate, bufferSize, producesAudio}` and a
  clean join in `disconnect()`/dtor. `createAudioDriver` routes `Fake` →
  `SoftwareDriver(producesAudio=true)`; `getDriverNames` + the `AudioDriverInfo`
  descriptor recognise it. Render-to-scratch (output always valid). Behaviour-
  preserving: the `FakeAudioDriver` casts in the tests (`TransportTest`,
  `AudioEngineTest`, `MidiActionTest`) were migrated to `SoftwareDriver`. Suite
  green, OK(329).
* **T8.2 — ✅ headless fallback.** `createAudioDriver` routes `Null` →
  `SoftwareDriver(producesAudio=false)`, so the audio-init-failure fallback and
  the editor mirror are now **clocked but output-less** (engine no longer freezes
  for lack of audio). `AudioDriverInfo` reports `isPresent && !isRunning` for it.
  `EditorModeTest::testMirrorUsesHeadlessDriver` asserts the mirror is a headless
  `SoftwareDriver` with valid rate/buffer (was the inert `NullDriver`); the
  `EditorBadEndpoint` guard now checks clean teardown only (the clock thread is
  expected). `startMidiDriver` already runs independently of audio.
* **T8.3 — ✅ MIDI-only mode.** First-class "no audio" standalone selection
  (`AudioDriver::Null` → headless `SoftwareDriver`) paired with a real MIDI driver;
  notes process on the clock, MIDI in/out works, no audio output. Added a
  fixed-fallback guard to `SoftwareDriver::init` (buffer → 1024, rate → 48000 when
  `Preferences` has no configured audio device — a 0 rate would divide by zero, a 0
  buffer would busy-loop the pump). New `AudioDriverTest::testMidiOnlyMode` spins up
  an isolated `Null`+`LoopBack` instance and asserts: headless software driver
  (`!producesAudio`, valid rate/buffer), a live MIDI driver alongside it, and that
  `sequencerPlay()` advances the transport (the headless clock runs — impossible
  with the old inert `NullDriver`).
* **T8.4 — ✅ editor hybrid sync.** The mirror's clocked software driver free-runs
  the playhead; the host is followed by **consuming the telemetry block** (ADR
  0018, previously unwritten/unconsumed — now both sides are wired):
  - *Writer*: `EngineSession` creates the telemetry segment keyed off its endpoint
    (`EngineTelemetryShm::keyForEndpoint`, derived identically both sides — no
    handshake negotiation) and `store()`s a transport snapshot
    (`buildTransportSnapshot`: frame / bpm / playing / tick) each ~50 ms serve-loop
    tick. Off the audio thread — advisory precision is enough for a ~5 s drift
    correction; the seqlock keeps reads tear-free.
  - *Reader / apply*: `EditorStateMirror::attachTelemetry` attaches the block and
    starts a 5 s re-sync `QTimer`; inbound transport **events** (`State`,
    `Relocation`, `TempoChanged`, `BbtChanged`) trigger an immediate
    `syncTransportFromTelemetry`. `applyTransportSnapshot` follows play/stop
    (`sequencerPlay/Stop`), tempo (`setNextBpm`, best-effort — a mirror timeline
    would override), and frame: relocate **only when stopped (exact)** or on a
    large divergence (> ~0.5 s) — snapping a rolling playhead to a slightly-stale
    telemetry frame would jerk it backward by the read latency, so routine lag is
    ignored while seeks / accumulated drift are corrected.
  - Relocation uses a new read-only `CoreActionController::relocateToFrame` (locks +
    the private `AudioEngine::locateToFrame`; no JACK broadcast / MIDI feedback) —
    the mirror calls its *own* controller (local follow, never forwarded; the editor
    never initiates transport, ADR 0026). Absent/mismatched block ⇒ events-only
    fallback. Tests: `EditorModeTest::testEngineBuildsTransportSnapshot` (snapshot
    reflects engine + key determinism + shm round-trip) and
    `testMirrorFollowsTransportTelemetry` (mirror follows play/stop/tempo and
    relocates to the host frame).
* **T8.5 — ✅ remove the old classes (FakeAudioDriver only).** Deleted
  `FakeAudioDriver.{h,cpp}` (no longer instantiated → `SoftwareDriver`, not a base
  of anything): removed its `AudioEngine.h` include + the vestigial
  `friend FakeAudioDriver::connect()` (SoftwareDriver needs no friendship), its
  `Hydrogen.cpp` include, and the now-dead `dynamic_pointer_cast<FakeAudioDriver>`
  branch in `getDriverNames`. Refreshed the stale `configureMirrorPreferences`
  comment (the mirror is clocked now, not "threadless").
  **`NullDriver` is retained but renamed to `StubAudioDriver`** — contrary to the
  original plan it is *not* dead: it is the inert (Null Object) base class of the
  not-compiled backend stubs (`AlsaAudioDriver`, `PortAudioDriver`, `OssDriver`,
  `PulseAudioDriver`, `CoreAudioDriver`, `JackDriver` all `: public StubAudioDriver`
  in their backend-absent `#else` branch), and `LocalEngineAccess::getAudioDriverInfo`
  relies on `dynamic_pointer_cast<StubAudioDriver>` to classify such a stub as
  present-but-not-running. Only its use as the inert *fallback instance* was retired
  (→ headless `SoftwareDriver`). It was renamed (`NullDriver.{h,cpp}` →
  `StubAudioDriver.{h,cpp}`, class + guard + `H2_OBJECT`) because after ADR 0031
  "Null" was overloaded: the `Preferences::AudioDriver::Null` selection now routes
  to the *active* headless `SoftwareDriver`, while this class is the *inert*
  unavailable-backend placeholder — keeping the name would conflate the two.
  Deleting it outright is not worth it: the stubs would otherwise each duplicate the
  inert method bodies, or `AudioDriver`'s pure virtuals would have to be demoted to
  defaulted ones (losing the compile-time "you must implement this" check for real
  drivers). The one untouched reference is a non-translated `"NullDriver"` UI label
  in `PreferencesDialog` (left as-is per the i18n string policy — flag for a
  separate UX/translation pass). The `Preferences::AudioDriver` enum keeps
  `Fake`/`Null` — they are the selection/routing keys into `createAudioDriver`.

**Done when:** standalone MIDI-only works (own clock, MIDI in/out, no audio);
audio-device failure degrades to a responsive headless engine; the editor mirror's
transport advances locally and stays drift-bounded against the host; a single
software driver replaces `NullDriver`+`FakeAudioDriver`; suite + ctest green.

**✅ DONE (2026-06-27).** All of T8.1–T8.5 landed; unit suite OK (331 tests) and the
editor/GUI ctests (`GuiStartup`, `EditorBadEndpoint`, `EditorReporterCleanFailure`)
green. One scope correction: the inert backend-stub base class is retained (see
T8.5), renamed `NullDriver` → `StubAudioDriver` to disambiguate it from the active
headless `SoftwareDriver` that `AudioDriver::Null` now routes to. `SoftwareDriver`
replaced `FakeAudioDriver` outright and the old class's inert *fallback-instance*
role — not the stub-base class itself.

**✅ Silent-render fast path (2026-06-27).** The headless software driver no longer
renders to a discarded scratch buffer — the Sampler skips the sample math outright
when the driver does not consume audio, while keeping note lifecycle and MIDI
bit-for-bit identical:
- `AudioDriver::producesAudio()` — new `virtual bool` capability (default `true`;
  `SoftwareDriver` overrides it with its `producesAudio` flag, plus a
  `setProducesAudio()` to toggle a running driver). Real output / Disk / Plugin /
  hardware drivers stay `true`.
- `Sampler::renderNote` gates the *output-only* work behind it: the
  resample/copySample interpolation, the resonant filter, and the mix into the
  master / track / bus buffers + peaks are skipped when `!producesAudio()`. What
  still runs every cycle (so behaviour is unchanged): sample-exhaustion end
  detection, `nNoteEnd`, `applyADSR` (its envelope/release state and return depend
  only on frame counts, not buffer contents — fed a zero-filled scratch over the
  same range), the `fSamplePosition` advance, and the Note-Off offset frame. So a
  full-length sample note's Note-Off still fires when the sample *would* have
  finished, custom-length Note-Offs keep their scheduled timing, and
  `releasePlayingNotes()` still ends notes.
- TDD: `MidiNoteTest::testSendNoteOffNoRender` renders the two-note scenario across
  all three `MidiSendNoteOff` modes once with audio rendered and once silent (the
  same `SoftwareDriver` toggled, so the only variable is the sample math) and
  requires an identical MIDI Note-On/Off type stream, plus that the silent path
  keeps the full-length sample Note-Off trailing its Note-On by the sample duration
  (i.e. it advanced the sample rather than sending the Note-Off early).

  Verified: unit suite **OK (332 tests)** (the new parity test is the +1) and the
  editor/GUI ctests (`GuiStartup`, `EditorBadEndpoint`, `EditorReporterCleanFailure`)
  green.

---

## 12. Test traceability (proposal 0003 §11 → phase)

| Test | Phase | Scope |
|---|---|---|
| existing suite green + `XmlTest` (old `<fx>` ignored) | PR | unit, all platforms |
| `FakePluginHost(Test)` | P0 | unit, all platforms |
| `MultiInstanceTest`, `PluginLifecycleTest`, `LoggerInstanceTest` | P1 | unit, all platforms |
| `EngineAccessTest` | P2 | unit, all platforms |
| `PluginProcessTest`, `TransportTest`(ext), `PluginMidiTest` | P3 | unit, all platforms |
| `PluginFeatureGateTest` (+ `MidiActionTest`/`TransportTest` ext) | P3 / T3.5 | unit, all platforms |
| `OutputBusTest` (default 1-to-1 + naming) | P4a | unit, all platforms |
| `OutputBusTest` (custom mapping) | P7 / T7.1 *(late, optional)* | unit, all platforms |
| GUI-gating checks (plugin-mode adaptation) | P7 / T7.2 *(late, required)* | unit + host-matrix |
| `H2ProjectTest`, `PluginStateTest` (embed toggle), `SampleTest`(ext), `XmlTest`(ext, unified open) | P4b | unit, all platforms |
| CLAP/LV2 validators | P4c | CI smoke |
| `IpcProtocolTest`, `PluginConfigTest`, `ConfigConcurrencyTest` | P5 | unit, all platforms |
| `EditorModeTest` | P5 | integration, Linux |
| VST3 validator, installer checks | P6 | CI smoke |
| stress / host-matrix | P7 | manual + CI where possible |

---

## 13. Cross-cutting practices

* **Branching:** one feature branch per phase off the `feat/plugin` integration
  branch; each task is a small reviewable commit that keeps the suite green.
* **CI gating:** plugin builds default `OFF`; the always-on Ubuntu 22.04 job
  builds plugin *targets* + runs the new unit tests; bundles are produced only in
  artifact-gated jobs (§10) to spare the 1 GB shared cache.
* **No silent scope cuts:** if a behaviour is deferred (e.g. an overflow rule,
  a host quirk), it is captured as a failing/ignored test or a documented
  limitation, never dropped silently.
* **Docs:** if implementation forces a design change, update the relevant ADR and
  this plan in the same PR.

## 14. Open implementation details

Resolved upfront (see the linked ADRs); only the first is genuinely deferred:

* **Remapping-widget UX (T7.1)** — *still deferred* (late-stage, optional). The
  routing model is settled in [ADR 0019](/docs/decisions/0019-plugin-output-bus-layout.md)
  (1-to-1 default, no automatic sharing, manual sharing allowed), pinned by
  `OutputBusTest`. Intended shape: a per-instrument **output selector in the
  mixer** (`Master` / `Bus 1…N`, pre-filled 1-to-1, bus entries showing the
  assigned instrument name; same-bus selection = manual sharing), persisted in
  state and feeding the CLAP/VST3 rename hook (T4a.3). The 1-to-1 default ships
  without it, so the exact widget can be finalised when built.
* **Telemetry shared-memory layout + peak cap** — *resolved*
  ([ADR 0018](/docs/decisions/0018-plugin-editor-ipc-transport.md)): seqlock'd POD
  struct, `formatVersion`-validated, fixed per-instrument peak cap of **256**;
  version mismatch → events-only fallback. Pinned by `IpcProtocolTest` in P5.
* **`Preferences` change-tracking** — *resolved*
  ([ADR 0023](/docs/decisions/0023-concurrency-safe-config-persistence.md)):
  **diff-against-baseline** (no dirty-set), which is also the 3-way merge's input.
  Realised in T5.6.
* **IPC wire format** — *resolved*
  ([ADR 0018](/docs/decisions/0018-plugin-editor-ipc-transport.md)): length-prefixed
  `QDataStream` (pinned version) framing; small messages as typed fields; large
  Song/Drumkit/state payloads as a `QByteArray` of the existing XML. Realised in
  T5.1.
