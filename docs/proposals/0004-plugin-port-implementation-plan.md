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

### 7a. Output buses — ADR 0019
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

### 7b. State, samples & `.h2project` — ADR 0017 / 0020 / 0025
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

### 7c. Native CLAP + LV2 — ADR 0014
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

## 9. Phase 5 — Out-of-process editor, IPC & layered config

ADRs 0016, 0018, 0022, 0023. Second-largest block.

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
* **T5.3 Editor mode** — `--plugin-editor <endpoint>` in `Parser`/`main.cpp`:
  skip `create_instance()`/driver; inject `IpcEngineAccess`; build the unchanged
  `MainForm`.
* **T5.4 Editor lifecycle** — plugin launches/reconnects/respawns/tears down the
  editor process; engine survives editor crash.
* **T5.5 Layered config** (ADR 0022): base from shared `~/.hydrogen`; override
  subset from host/state; retained load baseline on `Preferences`; hide host-owned
  controls in the editor's Preferences UI.
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

**Tests first**
* VST3 validator smoke step in CI; reuse the Windows installer verification
  pattern (`windows/ci/test_installation.py`) extended to assert plugin bundles
  install to the right locations.

**Tasks**
* **T6.1** Build `clap-wrapper` to emit `Hydrogen.vst3` from the CLAP.
* **T6.2** Packaging per platform: Linux relocatable `.tar.xz` (bundles +
  `hydrogen` binary + Qt); macOS bundles folded into `.dmg`
  (`macos/build_dmg.sh`); Windows NSIS installer (extend `cpack -G NSIS`) placing
  `.clap`/`.vst3`/`.lv2` in standard locations.
* **T6.3** Wire plugin bundles into the **artifact-gated** CI jobs (behind
  `UPLOAD_ARTIFACTS`), keeping per-commit jobs lean (§10).

**Done when:** CLAP/LV2/VST3 all pass validators; installers place bundles
correctly; artifacts produced only on tag/`*-artifacts` builds.

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
