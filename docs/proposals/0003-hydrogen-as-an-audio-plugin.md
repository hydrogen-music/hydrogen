# Proposal 0003 — Hydrogen as an audio plugin

Status: accepted (design) — 2026-06-08 — author: pm

This proposal collects the design rationale and the implementation plan for
turning Hydrogen into an audio plugin. The discrete decisions are recorded as
ADRs and linked throughout:

* [ADR 0013](/docs/decisions/0013-provide-hydrogen-as-an-audio-plugin.md) —
  groovebox plugin following host transport (Mode B)
* [ADR 0014](/docs/decisions/0014-plugin-format-strategy.md) —
  native LV2 + CLAP, VST3 via clap-wrapper
* [ADR 0015](/docs/decisions/0015-per-instance-engine-context.md) —
  Hydrogen as per-instance context; multi-instance from day one
* [ADR 0016](/docs/decisions/0016-out-of-process-plugin-ui.md) —
  out-of-process Qt editor
* [ADR 0017](/docs/decisions/0017-embed-song-in-plugin-state.md) —
  plugin state embeds a self-contained project by default (song-only opt-out)
* [ADR 0018](/docs/decisions/0018-plugin-editor-ipc-transport.md) —
  editor↔engine IPC: control socket + shared-memory telemetry
* [ADR 0019](/docs/decisions/0019-plugin-output-bus-layout.md) —
  master + compile-time-fixed bank of N stereo buses
* [ADR 0020](/docs/decisions/0020-plugin-state-sample-embedding.md) —
  samples embedded as a libarchive bundle, decoded in memory
* [ADR 0021](/docs/decisions/0021-no-host-parameter-automation-v1.md) —
  no host-automatable parameters in v1
* [ADR 0022](/docs/decisions/0022-layered-plugin-configuration.md) —
  layered config: plugin overrides over the shared user config
* [ADR 0023](/docs/decisions/0023-concurrency-safe-config-persistence.md) —
  concurrency-safe persistence of the shared user config
* [ADR 0024](/docs/decisions/0024-remove-ladspa-lrdf-effect-hosting.md) —
  remove LADSPA/LRDF effect hosting (deprecate 1.2.7, remove 2.0)
* [ADR 0025](/docs/decisions/0025-h2project-self-contained-format.md) —
  the `.h2project` self-contained project format (song + samples)
* [ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md) —
  plugin-mode feature disablement (core) and GUI adaptation

---

## 1. Goal

Ship Hydrogen as a plugin that hosts can load like any instrument, while keeping
what makes Hydrogen distinctive: patterns, song mode, the sampler, drumkits, and
the full existing editor. Concretely: a **groovebox plugin** that **follows the
host transport**, runs **multiple instances per process**, exposes the **full Qt
GUI out-of-process**, and stores its **entire song in the project**.

License context: Hydrogen is **GPLv2-or-later** (`COPYING`), which is what makes
the chosen SDKs usable (notably VST3 via its GPLv3 leg).

## 2. Plugin format comparison (summary)

| | LV2 | VST3 | CLAP | AU | VST2 | AAX |
|---|---|---|---|---|---|---|
| SDK license | ISC | GPLv3 / proprietary | MIT | Apple SDK | withdrawn 2018 | Avid + PACE |
| GPLv2+ compatible | ✅ | ✅ (via "or later") | ✅ | ✅ | ⚠️ no SDK | ❌ |
| Linux / Win / macOS | ✅/✅/✅ | ✅/✅/✅ | ✅/✅/✅ | ❌/❌/✅ | legacy | ❌/✅/✅ |
| API | C + `.ttl` | C++ COM-like | C, header-only | C/ObjC | C | C++ |
| Primary hosts | Ardour, Qtractor, Carla, Zrythm | ~all DAWs | Bitwig, Reaper, Studio One, FL | Logic, GarageBand | declining | Pro Tools |
| **Our plan** | **native** | **via clap-wrapper** | **native (primary)** | **not targeted** | skip | skip |

See [ADR 0014](/docs/decisions/0014-plugin-format-strategy.md) for the reasoning.
The target set is **LV2, CLAP, and VST3 only.** One engine integration seam
(shared by native LV2 and native CLAP) yields LV2 + CLAP directly and VST3
through `clap-wrapper`. AU is **not** targeted, so Logic/GarageBand are out of
scope; it could be added later via `clap-wrapper` with no engine changes.

## 3. Feature support in the plugin

| Feature | Source | Plugin status |
|---|---|---|
| Sample playback / sampler | `src/core/Sampler/` | ✅ Full |
| Drumkit loading (.h2drumkit) | `src/core/Basics/Drumkit` | ✅ Full |
| Velocity layers, round-robin, ADSR, pitch | Sampler | ✅ Full |
| Pattern sequencer + song mode | `AudioEngine`, `Transport` | ✅ Synced to host transport |
| Per-instrument outputs | JACK per-track today | ✅ via plugin output buses |
| MIDI note / CC input | `MidiActionManager`, `MidiEventMap` | ✅ Full (host MIDI) |
| MIDI note / CC output | `MidiOutput` | ⚠️ Where host routes plugin MIDI out |
| MIDI clock (in/out) | `Tempo::Midi`, clock stream | ❌ Disabled — host owns clock ([ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)) |
| Timeline / tempo markers | `src/core/Timeline.h` | ❌ Disabled ([ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)) |
| Tap tempo, Beat Counter, BPM-as-master, JACK Timebase | `Tempo::*`, `JackDriver` | ❌ Disabled / replaced by host transport ([ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)) |
| Loop state | `Transport` | 🔒 Forced **always on** ([ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)) |
| OSC server, NSM, Playlist + playlist MIDI actions | `OscServer`, `NsmClient`, `MidiActionManager` | ❌ Disabled ([ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)) |
| LADSPA effect inserts | `src/core/FX/LadspaFX` | ❌ **Removed from Hydrogen entirely** in 2.0 ([ADR 0024](/docs/decisions/0024-remove-ladspa-lrdf-effect-hosting.md)); host owns effects |
| Audio export / bounce | `DiskWriterDriver` | ➖ Host renders |
| Full Qt editor (mixer, editors) | `src/gui/` | ✅ Out-of-process, adapted ([ADR 0016](/docs/decisions/0016-out-of-process-plugin-ui.md), [ADR 0026](/docs/decisions/0026-plugin-mode-feature-disablement-and-gui.md)) |

## 4. Target architecture

```
            HOST PROCESS                                EDITOR PROCESS (per instance)
 ┌─────────────────────────────────────┐      ┌─────────────────────────────────┐
 │  Plugin binary (LV2 / CLAP)          │      │ hydrogen --plugin-editor (QApp)  │
 │                                      │ IPC  │                                  │
 │  PluginAudioDriver  ── host buffers  │◄────►│  existing src/gui widgets        │
 │  PluginMidiDriver   ── host MIDI     │      │  (one engine handle, observes    │
 │  HostTransport      ── host tempo/pos│      │   EventQueue over IPC, sends     │
 │                                      │      │   commands via CoreActionCtrl)   │
 │  Hydrogen (per-instance context)     │      └─────────────────────────────────┘
 │   ├ AudioEngine ─ audioEngine_process│
 │   ├ Sampler                          │     clap-wrapper ──► VST3
 │   ├ Preferences (per-instance)       │
 │   ├ EventQueue  (per-instance)       │
 │   ├ Logger      (per-instance, own log file)
 │   └ SoundLibraryDatabase (per-instance)
 └─────────────────────────────────────┘
```

Four engine-side seams replace the standalone app's driver/transport/UI roles:

1. **`PluginAudioDriver : AudioDriver`** (`src/core/IO/AudioDriver.h:37`) — its
   `getOut_L/R()` point at host buffers; `init`/`connect` are no-ops; the host's
   `process()` drives `audioEngine_process()` (`AudioEngine.cpp:1743`).
2. **`PluginMidiDriver : MidiBaseDriver`** (`src/core/IO/MidiBaseDriver.h:45`) —
   injects host MIDI events into the existing input queue.
3. **Host transport source** — each block, read host play-state/tempo/position
   and drive the playhead/queuing transports instead of `incrementPlayhead()` and
   Timebase-master logic ([ADR 0013](/docs/decisions/0013-provide-hydrogen-as-an-audio-plugin.md)).
4. **Engine↔UI IPC** — a `QLocalSocket` carries discrete `EventQueue` events
   (engine→editor) and `CoreActionController` commands (editor→engine); a
   versioned shared-memory block carries high-rate telemetry (playhead, peaks,
   process time). See [ADR 0016](/docs/decisions/0016-out-of-process-plugin-ui.md)
   and [ADR 0018](/docs/decisions/0018-plugin-editor-ipc-transport.md).

## 5. The de-singletoning refactor (the critical path)

See [ADR 0015](/docs/decisions/0015-per-instance-engine-context.md). ~681
`get_instance()` sites in `src/core` and ~1,292 in `src/gui`. **Everything
engine-related becomes per-instance, owned by `Hydrogen`** —
`Preferences`/`EventQueue`/`Logger`, plus `SoundLibraryDatabase` (which is already
a per-`Hydrogen` leaf and stays that way). Nothing is shared between instances, to
avoid state leakage. The **per-instance `Logger`** is special because logging is
wired through the *static* `Base::__logger` (`Object.h:155`); it is handled by an
**ambient thread-local context resolved at log time** (logging only, no `Base`
member or constructor changes) — see [ADR 0015](/docs/decisions/0015-per-instance-engine-context.md).

**Chosen — Option 1, Hydrogen as the per-instance context:**

```cpp
// Hydrogen.h — static __instance / get_instance() removed; constructable N times
class Hydrogen {
public:
    Hydrogen(std::shared_ptr<Preferences> pPref, int nOscPort);   // was create_instance()
    AudioEngine*                 getAudioEngine() const { return m_pAudioEngine; }
    std::shared_ptr<Preferences> getPreferences() const { return m_pPreferences; }
    EventQueue*                  getEventQueue()  const { return m_pEventQueue; }
    Logger*                      getLogger()      const { return m_pLogger; }
private:
    std::shared_ptr<Preferences> m_pPreferences;   // per-instance
    EventQueue*                  m_pEventQueue;     // per-instance
    Logger*                      m_pLogger;         // per-instance (own log file)
    AudioEngine*                 m_pAudioEngine;
    // SoundLibraryDatabase: already a per-Hydrogen leaf — unchanged.
};

// per instance, no global state:
m_pHydrogen = std::make_unique<Hydrogen>(Preferences::create(), oscPort);

// call sites:
m_pHydrogen->getEventQueue()->pushEvent(Event::Type::PatternChanged, 0);  // was EventQueue::get_instance()->...
// Logging: macros resolve currentContext().logger() (thread-local, set by an RAII
// scope at instance-entry points) instead of the static Base::__logger; a process-
// default fallback covers static/instance-less contexts. Ambient context = logging only.
```

Rejected alternatives (for the record):

```cpp
// Option 2 — pure EngineContext: a separate container, Hydrogen demoted to a member
struct EngineContext { std::shared_ptr<Preferences> preferences; std::unique_ptr<EventQueue> eventQueue;
                       std::unique_ptr<Hydrogen> hydrogen; std::unique_ptr<AudioEngine> audioEngine; };

// Option 3 — pure DI: every dependency threaded explicitly
class AudioEngine { public: AudioEngine(Preferences& pref, EventQueue& events); };
```

Option 2 duplicates the container concept; Option 3 is the largest/riskiest diff.
New leaf classes may still take explicit references (DI-style) for testability.

## 6. Implementation plan (phased)

> The **detailed, test-driven** breakdown of this plan — per-task, each gated by a
> test written first — lives in
> [proposal 0004](0004-plugin-port-implementation-plan.md). The phases below are
> the overview.

Multi-instance is required from day one ([ADR 0015](/docs/decisions/0015-per-instance-engine-context.md)),
so the de-singletoning leads. Phases are roughly sequential; bracketed estimates
are engineering-weeks for one experienced C++/Qt developer and are indicative.

* **Phase 0 — Spike (1–2 wk).** Throwaway single-instance CLAP that wires a
  `PluginAudioDriver` to `audioEngine_process()` and makes sound under one host.
  Goal: de-risk the audio seam only; no transport sync, no UI, *not shipped*.

* **Phase 1 — De-singleton the core (5–7 wk).** [ADR 0015]
  Remove `Hydrogen::__instance`/throw; make `Hydrogen` constructable N times,
  owning per-instance `Preferences`/`EventQueue`/`Logger`. Convert `get_instance()`
  chains in `src/core`. `SoundLibraryDatabase` already is a per-`Hydrogen` leaf —
  keep it per-instance (do **not** promote to a singleton). The **per-instance
  `Logger`** is handled by an ambient thread-local context resolved at log time
  (logging only): redefine the log macros off the static `Base::__logger`
  (`Object.h:155`) to `currentContext().logger()` + a process-default fallback —
  no `Base` member or constructor changes. Standalone app keeps passing tests
  throughout.

* **Phase 2 — GUI de-singleton + standalone bring-up (2–3 wk).**
  Convert the ~1,292 GUI `get_instance()` sites to resolve against a single
  injected engine handle. Confirm the standalone app runs unchanged on the
  refactored core (regression gate before any plugin UI work).

* **Phase 3 — Plugin host seams (2–3 wk).** [ADR 0013]
  Production `PluginAudioDriver` + `PluginMidiDriver`; host-transport follower
  driving playhead/queuing transports; output-bus mapping for per-instrument
  outs. Exclude `DiskWriter` and NSM/JACK-session from the plugin build. (The
  LADSPA/LRDF effect host is already gone — removed in Phase R, [ADR 0024](/docs/decisions/0024-remove-ladspa-lrdf-effect-hosting.md).)

* **Phase 4 — Native CLAP + LV2 (2–3 wk).** [ADR 0014]
  Native CLAP plugin (state, params, note ports, transport) over the Phase 3
  seam; native LV2 sharing the same seam. State = `.h2project` bundle by default
  with a song-only toggle ([ADR 0017](/docs/decisions/0017-embed-song-in-plugin-state.md),
  [ADR 0025](/docs/decisions/0025-h2project-self-contained-format.md)); the same
  codec also gives the standalone app a `.h2project` export/open.

* **Phase 5 — Out-of-process editor + IPC (6–10 wk).** [ADR 0016]
  Add an **editor mode to the existing `hydrogen`** binary
  (`hydrogen --plugin-editor <endpoint>`): startup skips engine/driver creation
  and injects an IPC-backed access object behind the same interface the local
  `Hydrogen` provides, then builds the unchanged `MainForm`. Build the engine↔UI
  IPC (EventQueue marshalling out, CoreActionController commands in); editor-
  process lifecycle (launch/reconnect/respawn/teardown); engine keeps playing if
  the editor closes or crashes. Wire the **layered config**
  ([ADR 0022](/docs/decisions/0022-layered-plugin-configuration.md)): shared
  `~/.hydrogen` as the base (theme/shortcuts/language/layout), with audio-MIDI-I/O
  and recent/last-file settings overridden from the host engine and plugin state.
  Base-layer changes **are persisted**, but the snapshot-on-close `save()` is
  replaced on the shared-config path by the concurrency-safe atomic/locked
  field-merge of [ADR 0023](/docs/decisions/0023-concurrency-safe-config-persistence.md)
  so parallel teardown cannot corrupt the file.

* **Phase 6 — VST3 via clap-wrapper + packaging (1–2 wk).** [ADR 0014]
  Add `clap-wrapper` build to emit VST3 from the CLAP. Package each bundle with
  its editor executable across Linux/Windows/macOS.

* **Phase 7 — Multi-instance hardening (2–3 wk).** [ADR 0015]
  State-isolation tests (two instances must not share song/tempo/events), CPU/RT
  validation under N instances, host-matrix testing (Ardour, Qtractor, Reaper,
  Bitwig, Cubase, Live), state save/load round-trips.

**Indicative total: ~5–7 months.** The dominant, format-independent costs are
Phases 1–2 (de-singletoning) and Phase 5 (out-of-process UI + IPC). Adding the
VST3 format on top of CLAP is marginal (Phase 6).

Build, distribution, and CI-cost detail per platform are in §9–§10; the test
coverage each phase must land — gated by the same `src/tests/tests` run that
guards the standalone app — is in §11. In particular `MultiInstanceTest` lands
with Phases 1–2 as the regression gate for the whole refactor.

## 7. Known limitations (to communicate to users)

* Host tempo overrides Hydrogen's timeline tempo markers
  ([ADR 0013](/docs/decisions/0013-provide-hydrogen-as-an-audio-plugin.md)).
* No LADSPA/LRDF effect inserts anywhere — the subsystem is removed from Hydrogen
  in 2.0 (deprecated 1.2.7, [ADR 0024](/docs/decisions/0024-remove-ladspa-lrdf-effect-hosting.md)); use host effects.
* No internal audio export, NSM, or JACK-session from the plugin — the host owns
  rendering and sessions.
* MIDI output/clock works only where the host routes plugin MIDI out.
* State blobs are large for big multi-sample kits when sample-embedding is on
  (the default); the per-project toggle stores song-only state for huge kits, at
  the cost of needing the kit installed ([ADR 0017](/docs/decisions/0017-embed-song-in-plugin-state.md),
  [ADR 0025](/docs/decisions/0025-h2project-self-contained-format.md)).
* No VST2 (no legal SDK) and no AAX (GPL-incompatible).

## 8. Resolved design details

All four items previously open here are now decided:

* **IPC transport** — `QLocalSocket` for ordered events/commands +
  versioned shared-memory block for high-rate telemetry; commands map onto the
  `CoreActionController` vocabulary; an IPC bridge thread drains `EventQueue` off
  the audio thread. ([ADR 0018](/docs/decisions/0018-plugin-editor-ipc-transport.md))
* **Output-bus layout** — master bus (always the full mix) + a bank of **N**
  stereo buses, **N a compile-time CMake option** (`H2_PLUGIN_OUTPUT_BUSES`,
  default 32). **Default mapping is 1-to-1** (first N instruments → buses,
  pre-fader); surplus/unmapped instruments play through master only — so kits
  with > N instruments work and **nothing is *automatically* summed onto a shared
  bus**. v1 targets **dynamic bus renaming to the instrument names on CLAP
  (`RESCAN_NAMES`) and VST3 (`kIoTitlesChanged`)**; LV2 uses fixed `Out 1…N`
  labels (static `.ttl`, generated from N, plus a `stereo` variant; VST3 inherits
  via `clap-wrapper`). A **remapping widget** (lets the user change the mapping,
  incl. *manually* putting several instruments on one bus) and its persisted
  custom mapping are a **late-stage convenience** — the default needs no UI.
  ([ADR 0019](/docs/decisions/0019-plugin-output-bus-layout.md))
* **Sample embedding / `.h2project`** — the self-contained artifact is a
  libarchive bundle (song XML + unique sample files, content-hash de-duplicated)
  decoded **from memory** via libsndfile virtual I/O (no temp files → sandbox-safe,
  no temp-dir lifecycle; needs a new in-memory load path on `Sample`). This codec
  is promoted to a named **`.h2project`** format
  ([ADR 0025](/docs/decisions/0025-h2project-self-contained-format.md)): the
  standalone app gets a menu action to create one, the plugin embeds it in state
  **by default with a song-only toggle** for huge kits
  ([ADR 0017](/docs/decisions/0017-embed-song-in-plugin-state.md)), and a single
  open path loads either `.h2song` or `.h2project`.
  ([ADR 0020](/docs/decisions/0020-plugin-state-sample-embedding.md))
* **Parameter automation** — **none in v1**. Hydrogen has effectively no
  automation surface today (only master volume); host-automatable parameters are
  deferred until the application grows a real automation model.
  ([ADR 0021](/docs/decisions/0021-no-host-parameter-automation-v1.md))

These touch the implementation plan as follows: Phase 4 (CLAP/LV2) generates the
LV2 `.ttl` from `H2_PLUGIN_OUTPUT_BUSES` and adds the in-memory `Sample` load
path; Phase 5 (out-of-process editor) builds the socket + shared-memory IPC; the
Id→bus mapping rides in the embedded song state from Phase 4 onward.

## 9. Build & distribution per platform

The release pipeline today (`.appveyor.yml`) has four jobs, each producing the
standalone app: **Ubuntu 22.04** (Qt6, builds + runs the test suite, no artifact),
**Ubuntu 18.04** (Qt5, `WANT_APPIMAGE`, ships the `.AppImage` via `linuxdeploy`),
**macOS** (Qt5, `macos/build_dmg.sh` → `.dmg`), **Windows64** (MinGW/MSYS2 Qt5,
`windeployqt` + `cpack -G NSIS` → `.exe`). ccache is used on Linux+Windows, a
Homebrew tarball cache on macOS, within a **1 GB cache shared across all jobs**.

### New build artifacts

Three plugin bundles plus one editor executable, all driven from the same
`hydrogen-core` the standalone app already builds:

| Artifact | Form | Built from |
|---|---|---|
| `Hydrogen.clap` | shared lib with `.clap` extension | native CLAP wrapper + engine seams |
| `Hydrogen.lv2/` | bundle dir: `.so`/`.dll`/`.dylib` + generated `.ttl` + `manifest.ttl` | native LV2 wrapper |
| `Hydrogen.vst3/` | VST3 bundle dir | CLAP → `clap-wrapper` |
| editor | **a mode of the existing `hydrogen` binary** (`hydrogen --plugin-editor`), [ADR 0016](/docs/decisions/0016-out-of-process-plugin-ui.md) | `src/gui` + an IPC-backed access object |

The editor is **not a new executable** — it is the existing `hydrogen` GUI run in
an editor mode that attaches to the host-side engine over IPC instead of creating
its own (see ADR 0016 and §4). The plugin bundle therefore ships the standard
`hydrogen` binary plus the format wrappers, not a bespoke editor program.

New CMake options gate the wrappers so the standalone build is unaffected:
`WANT_LV2`, `WANT_CLAP`, `WANT_VST3` (implies CLAP + `clap-wrapper`), and the
existing `H2_PLUGIN_OUTPUT_BUSES` ([ADR 0019](/docs/decisions/0019-plugin-output-bus-layout.md)).
New build dependencies are light: `clap` headers (header-only), `lv2` headers
(header-only), and `clap-wrapper` (a moderate C++ submodule). The LV2 `.ttl` is
**generated at configure/build time** from `H2_PLUGIN_OUTPUT_BUSES`, with a
separate `stereo` variant.

### Qt deployment — the key packaging difference

Because the editor is **out-of-process**, the plugin binaries themselves stay
thin (engine + thin controller; core still links Qt Core/Xml/Gui/Network, but not
Widgets/Svg). The heavy Qt Widgets UI lives only in the `hydrogen` binary running
in editor mode. This matters: shipping Qt *inside* a plugin `.so` that is
`dlopen`ed into an arbitrary host risks symbol clashes with a host's own Qt.
Keeping Widgets in a separate process sidesteps the worst of it; the plugin's own
Qt-Core dependency is deployed alongside the bundle per platform, and the editor's
full Qt deployment is the same one the standalone app already ships.

### Per-platform packaging & install locations

* **Linux.** A plugin is **not** an AppImage (AppImage is a self-contained app,
  not a host-loadable bundle). Ship a relocatable **`.tar.xz`** (and later distro
  packages) containing `Hydrogen.lv2/`, `Hydrogen.clap`, `Hydrogen.vst3/`, the
  `hydrogen` binary (which doubles as the editor), and the bundled Qt libs.
  Install to the standard `~/.lv2` `~/.clap` `~/.vst3` (or `/usr/lib/...`). Built
  in the **Qt5 AppImage job's toolchain** (it already deploys Qt via
  `linuxdeploy`), gated behind `UPLOAD_ARTIFACTS` like the AppImage.
* **macOS.** Bundle into the standard `~/Library/Audio/Plug-Ins/{CLAP,VST3,LV2}`
  layout; distribute inside the existing **`.dmg`** (extend `macos/build_dmg.sh`)
  or a `.pkg`. The `hydrogen.app` (used for the editor mode) is embedded in/next
  to the bundle. Code-signing/notarization is required for distribution outside
  Gatekeeper warnings — a packaging task, no engine impact.
* **Windows.** Install `.clap`/`.vst3` to `%CommonProgramFiles%\{CLAP,VST3}` and
  `.lv2` to `%APPDATA%\LV2` via an **extended NSIS installer** (reuse the existing
  `cpack -G NSIS` flow); `windeployqt` deploys Qt for `hydrogen.exe` exactly as it
  does today — the same binary serves the editor mode.

## 10. CI time & cache impact

What actually costs time in CI:

* **Compilation of the wrappers is small.** CLAP and LV2 headers are header-only;
  `clap-wrapper` is moderate but compiled once and **ccache-cached** thereafter.
  The engine refactor (de-singletoning) changes existing code but adds little net
  compile volume.
* **The editor adds essentially no GUI build cost** — it is the existing
  `hydrogen` binary with an extra startup mode, not a second program, so there is
  no additional full Qt-Widgets compile or link.
* **Extra link + deploy + packaging steps** are the real additions: three more
  link targets, plus per-format `windeployqt`/`linuxdeploy`/bundle assembly, plus
  validator smoke tests (§11).
* **Links are not ccache-cached** — each added plugin target adds an
  uncacheable link step every build.

**Rough estimate** (warm cache): **+15–30 % wall-clock per job** that builds
plugins, dominated by linking + packaging, not compilation. Cold-cache builds
(cache miss / dep bump) add a one-off `clap-wrapper` compile, minutes-scale.

**Cache pressure is the real risk, not CPU.** The 1 GB cache is already described
as "hopelessly tight" in `.appveyor.yml`. Mitigations:
* `clap`/`lv2` are header-only (no ccache footprint); `clap-wrapper` adds a
  bounded set of objects — measure and, if needed, bump the per-job ccache budget
  from 256 MB only where plugins are built.
* **Don't build all artifacts in all jobs.** Keep the always-on **Ubuntu 22.04
  (Qt6)** job focused on *building the plugin targets + running the engine/IPC
  unit tests* (the regression guard), and produce the shippable plugin **bundles**
  only in the **artifact-gated** jobs (alongside AppImage/dmg/exe, behind
  `UPLOAD_ARTIFACTS`). This keeps per-commit CI lean and pushes the heavy
  packaging to release/tag builds.

A possible 5th job (dedicated plugin-validation) is deliberately avoided to spare
the shared cache; validation runs inside existing jobs instead.

## 11. Test plan (`src/tests/`)

The suite is CppUnit, globbed from `src/tests/*.cpp`, registered in
`registeredTests.h`, and linked against **`hydrogen-core`** — i.e. it already
exercises the exact library the plugin wraps, so most new coverage is plain
core-level tests that run on **all four platforms** with no host needed. New tests
are added the same way (a `*Test.h/.cpp` pair + a `CPPUNIT_TEST_SUITE_REGISTRATION`
line). Process-level/host-level tests go behind the existing
`WANT_INTEGRATION_TESTS` (already gated to Linux + debug, off MinGW/macOS).

The goal is to cover **all essential plugin behaviours**, not only the riskiest
decisions — anything a host can do to the plugin, and anything the plugin must
guarantee back, should have a regression test in the same `src/tests/tests` run
that already guards the standalone app. A shared **`FakePluginHost`** test
fixture (a non-audio harness that supplies buffers, a transport state, MIDI
events, and a chosen sample rate / block size, modelled on the existing
`FakeAudioDriver`) underpins most of group A–C.

**Group A — engine under a host (no hardware driver).** Run on all platforms.

| Area | New/extended test | Asserts | Models after |
|---|---|---|---|
| **Multi-instance** ([ADR 0015](/docs/decisions/0015-per-instance-engine-context.md)) — *top regression guard* | `MultiInstanceTest` | Two `Hydrogen` instances with independent song/tempo/transport/`EventQueue`; mutating one never affects the other; clean construct/destruct, no leaked global state | `MemoryLeakageTest`, `AudioEngineTest` |
| **Host-transport follower** ([ADR 0013](/docs/decisions/0013-provide-hydrogen-as-an-audio-plugin.md)) | extend `TransportTest`; revive the disabled `AudioDriverTest` via `FakePluginHost` | Playhead tracks host position sample-accurately; tempo, start/stop, relocate, and loop all follow the host | `TransportTest`, `AudioEngineTest` |
| **Variable block size & sample rate** | `PluginProcessTest` | Correct, click-free output for arbitrary/odd `nframes`, mid-stream block-size changes, and sample-rate changes (tick size recomputed); identical render regardless of block boundary | `AudioEngineTest`, `AudioExportTest` |
| **Sample-accurate scheduling** | extend `PluginProcessTest` | A note due mid-block fires at the right frame offset, independent of where the block starts | `TransportTest`, `MidiNoteTest` |
| **Reposition / loop voice handling** | extend `TransportTest` | Host relocate/loop-wrap neither hangs nor wrongly cuts voices; defined, stable behaviour | `TransportTest`, `AudioEngineTest` |
| **Silence & denormal safety** | extend `AudioBenchmark`/`PluginProcessTest` | No notes → exact silence; output never contains NaN/denormals under sustained processing | `AudioBenchmark` |
| **Timeline vs host tempo** | extend `TransportTest` | With a timeline present, host tempo wins ([ADR 0013](/docs/decisions/0013-provide-hydrogen-as-an-audio-plugin.md)); markers stay advisory | `TransportTest` |

**Group B — MIDI input/output.** Run on all platforms.

| Area | New/extended test | Asserts | Models after |
|---|---|---|---|
| **Host MIDI → notes** | `PluginMidiTest` (+ extend `MidiNoteTest`) | Host note-on/off triggers the right instrument at the right velocity; note-off/choke honoured | `MidiNoteTest` |
| **MIDI CC → actions** | extend `MidiActionTest` | CC/program events routed through `MidiActionManager` still map to the configured actions when delivered as host MIDI | `MidiActionTest` |
| **MIDI event timing** | extend `PluginMidiTest` | Events carrying a sample offset land at that frame within the block | `MidiNoteTest`, `TransportTest` |
| **MIDI out (where supported)** | extend `MidiExportTest` | Engine-emitted MIDI (incl. notes/clock) is produced correctly for hosts that route plugin MIDI out | `MidiExportTest` |

**Group C — audio routing / output buses** ([ADR 0019](/docs/decisions/0019-plugin-output-bus-layout.md)). Run on all platforms.

| Area | New/extended test | Asserts | Models after |
|---|---|---|---|
| **Default 1-to-1 mapping** | `OutputBusTest` | First N instruments feed buses 1…N (pre-fader); master carries the full sum; surplus/unmapped instruments route to **master only**; nothing is *automatically* summed onto a shared bus | `AudioExportTest` |
| **Bus naming (CLAP/VST3)** | extend `OutputBusTest` | Bus label tracks the assigned instrument name; the rename hook fires on kit/mapping change (CLAP `RESCAN_NAMES` / VST3 `kIoTitlesChanged`); LV2 keeps fixed `Out N` labels | — |
| **Declared layout matches N** | extend `OutputBusTest` | Active bus/channel count equals the configured `H2_PLUGIN_OUTPUT_BUSES` | — |
| **Custom mapping** *(late-stage)* | extend `OutputBusTest` | A user-set mapping (incl. several instruments deliberately on one bus) routes correctly, persists, and survives add/remove/move/reload | `AudioExportTest`, `DrumkitTest` |

**Group D — state, samples & `.h2project`** ([ADR 0017](/docs/decisions/0017-embed-song-in-plugin-state.md), [ADR 0020](/docs/decisions/0020-plugin-state-sample-embedding.md), [ADR 0025](/docs/decisions/0025-h2project-self-contained-format.md)). Run on all platforms.

| Area | New/extended test | Asserts | Models after |
|---|---|---|---|
| **`.h2project` round-trip** | `H2ProjectTest` | Song+kit → `.h2project` bundle → reconstructs identically (instruments, patterns, samples, bus mapping); reusable by standalone export and plugin state | `DrumkitExportTest`, `XmlTest` |
| **In-memory sample decode** | extend `SampleTest` | A sample decoded from a memory buffer (libsndfile virtual I/O) is bit-identical to the file-loaded one; content-hash dedup works | `SampleTest` |
| **Unified open path** | extend `XmlTest`/`H2ProjectTest` | One open endpoint loads both `.h2song` and `.h2project` (detected by container); plugin state reader accepts both song-only and full-bundle | `XmlTest`, `DrumkitExportTest` |
| **Embed toggle** | `PluginStateTest` | Toggle ON → state is a portable `.h2project`; OFF → song-only state (kit by reference); both reload correctly in a fresh instance | `DrumkitExportTest` |
| **State version compatibility** | extend `XmlTest` | Older `formatVersion` loads or migrates gracefully; malformed/foreign blobs rejected without crashing | `XmlTest` |
| **Large-kit / off-RT** | extend `PluginStateTest` | Big bundles save/load entirely off the audio thread; no RT allocation on the process path | `AudioExportTest` |

**Group E — IPC & editor mode** ([ADR 0018](/docs/decisions/0018-plugin-editor-ipc-transport.md), [ADR 0016](/docs/decisions/0016-out-of-process-plugin-ui.md)).

| Area | New/extended test | Scope | Asserts |
|---|---|---|---|
| **Protocol codec** | `IpcProtocolTest` | unit, all platforms | Every `CoreActionController` command and every `Event::Type` round-trips through the message codec; telemetry shared-memory struct is layout-versioned and back/forward-compatible |
| **Layered config** ([ADR 0022](/docs/decisions/0022-layered-plugin-configuration.md)) | `PluginConfigTest` | unit, all platforms | A `Preferences` built as base ← override keeps base fields (theme/shortcuts/language) from the shared config while the override subset (audio/MIDI I/O, JACK/OSC, recent/last-file) takes host/state values; the override subset is excluded from any write to the shared config |
| **Config persistence** ([ADR 0022](/docs/decisions/0022-layered-plugin-configuration.md)) | extend `PluginConfigTest` | unit, all platforms | A base-layer change (e.g. theme) made in plugin context is written back to the shared config and survives a reload |
| **Parallel-teardown safety** ([ADR 0023](/docs/decisions/0023-concurrency-safe-config-persistence.md)) | `ConfigConcurrencyTest` | unit, all platforms | Two `Preferences` each changing a *different* base field then persisting concurrently → both changes survive (field-level merge, atomic+locked write); no full-file clobber; same field → bounded last-writer-wins, never corruption |
| **Editor-mode startup** | `EditorModeTest` | integration (`WANT_INTEGRATION_TESTS`, Linux) | `hydrogen --plugin-editor <endpoint>` attaches to a fake engine endpoint and builds `MainForm` **without** creating a local engine/driver; config is composed per [ADR 0022](/docs/decisions/0022-layered-plugin-configuration.md) (general settings shared, plugin subset overridden, shared config untouched) |
| **Live editor session** | extend `EditorModeTest` | integration (Linux) | Spawned editor connects, receives events, issues commands; engine keeps running if the editor disconnects/crashes |

**Group F — lifecycle & per-format load.**

| Area | Test | Scope | Asserts |
|---|---|---|---|
| **Plugin lifecycle** | `PluginLifecycleTest` | unit, all platforms | instantiate → activate → process → deactivate → destroy is repeatable; re-instantiate in the same process (ties into multi-instance) leaves no residue |
| **Per-format validators** | CI smoke step (not CppUnit) | build jobs | `clap-validator` (`.clap`), `lv2lint`/`lv2_validate` (`.lv2`), the VST3 validator (`.vst3`): instantiate, activate, process silence, no crash |

### Sequencing & where they run

* **Phases 1–2** (de-singletoning): `MultiInstanceTest`, `PluginLifecycleTest`,
  and the per-instance `Preferences` isolation checks land first — they are the
  gate that proves the refactor.
* **Phases 3–4** (host seams, CLAP/LV2): groups A, B, C, D.
* **Phase 5** (editor mode + IPC): group E.
* **Phase 6** (packaging): the group F validator smoke step is added to the build
  jobs.

All CppUnit groups (A–D, the unit parts of E–F) run in the existing
`src/tests/tests --appveyor` invocation on **every platform**, so any regression
in plugin-critical engine behaviour fails the same pipeline that guards the
standalone app today. The Linux-only integration parts (editor-mode startup, live
session) run behind `WANT_INTEGRATION_TESTS`, and the per-format validators run as
a CI step in the plugin-building jobs (§10).
