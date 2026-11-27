---
status: accepted
date: 2026-07-29
deciders: pm
---

# AD: Enforcing authoritative-only access in the editor mirror via `ASSERT_NO_EDITOR_MODE` and an integration test

## Context and Problem Statement

When Hydrogen runs in editor mode — `hydrogen --connect-via-ipc <endpoint>`
([ADR 0016](0016-out-of-process-plugin-ui.md), [ADR
0032](0032-h2player-gui-connection-mode.md)) — the GUI process hosts a
**mirror** engine (`ProcessMode::Editor`) with a passive `SoftwareDriver` (Null
audio, no MIDI, no OSC; [ADR
0031](0031-decouple-engine-clock-from-audio-driver.md)). The **authoritative**
engine — with real audio/MIDI/OSC drivers — lives in the headless host process
(plugin host or a standalone headless instance `h2player`). The GUI reaches the
authoritative engine through the IPC seams defined in [ADR
0018](0018-plugin-editor-ipc-transport.md) and [ADR
0027](0027-coreactioncontroller-single-write-surface.md): `IEngineAccess` for
reads, `CoreActionController` for writes, telemetry via shared memory, events
via the control socket.

[ADR 0029](0029-audio-driver-access-across-editor-split.md) removed
`getAudioDriver()` and `getMidiDriver()` from the `IEngineAccess` read
surface so the GUI can no longer obtain driver pointers. But `src/core`
contains methods on `Hydrogen` and other core classes that **directly**
access the audio driver, MIDI driver, and OSC server — e.g.
`Hydrogen::hasJackDriver()` casts the audio driver to `JackDriver`,
`Hydrogen::panic()` calls `getMidiDriver()->sendAllNotesOff()`, and
`Hydrogen::toggleOscServer()` / `recreateOscServer()` operate on
`m_pOscServer`. These methods are meaningful only on the authoritative
engine. If the mirror calls them — whether through a GUI code path, a
shortcut dispatch, or an event handler — the mirror's Null/absent drivers
produce wrong results at best and crashes at worst.

The `ASSERT_NO_EDITOR_MODE` macro (`src/core/Hydrogen.h:642-668`) was
introduced to catch this at the call site: in debug builds
(`H2CORE_HAVE_DEBUG` and `NDEBUG` not defined) it checks
`pHydrogen->getProcessMode() == ProcessMode::Editor` and, if true, logs an
error with the thread ID, flushes the logger, and calls `assert(false)` →
SIGABRT. In release builds it expands to nothing.

Call sites will be added incrementally as the editor-mode work progresses. The
macro is **not yet exhaustive** — it is a growing safety net, not a complete
enforcement.

### The testing gap

The existing editor-mode tests cover construction, IPC handshake, transport
sync, and teardown:

* `EditorAttach` (CTest, `PluginEditorSmoke.cpp`) — spawns the real GUI with
  `--connect-via-ipc` + `--quit-after-startup`; proves the
  spawn→connect→show→teardown chain works. But `--quit-after-startup` fires
  `QTimer::singleShot(0, pQApp, &QApplication::quit)` — one event-loop pass
  — so **no menu actions, transport toggles, dialog openings, or mixer
  interactions are exercised**.
* `ConnectViaIpcModeTest`, `IpcTransportTest`, `EngineSessionTest`
  (CppUnit) — test the IPC protocol and mirror mechanics in-process, but do
  not exercise the GUI's action dispatch paths.

No test currently triggers the GUI code paths most likely to reach an
`ASSERT_NO_EDITOR_MODE` site: transport controls, JACK transport toggles,
panic, audio-engine-info dialog, drumkit switching, mixer strip operations,
timeline edits, etc. The assert could be silently missing from a call site,
and nothing would catch a regression until a user hits it in production.

### In-process vs. out-of-process

`ASSERT_NO_EDITOR_MODE` calls `assert(false)` → SIGABRT, which **kills the
process**. An in-process test (CppUnit or QTest) would be killed by the
assert too, taking the entire test runner down with it. The out-of-process
approach isolates the crash: the test harness spawns the editor as a child
process, monitors its exit code, and reports failure cleanly.

A viable in-process alternative exists: make the macro throw an
`H2Exception` instead of calling `assert(false)` (gated by a compile-time
flag like `-DH2_TEST_ASSERT_NO_EDITOR_MODE`). The test wraps each
`executeShortcut` call in `try/catch`, catches the exception, records which
action triggered it, and moves on — no process death, no `QProcess`, no IPC
handshake, no separate binary. This would be simpler and faster (runs in the
existing CppUnit suite, shares `TestHelper` / `FakePluginHost`
infrastructure, no process spawn or IPC round-trips).

However, two risks make the out-of-process approach the safer choice:

* **Exception safety through Qt.** `executeShortcut` is called directly
  (not through signal/slot), so the exception would propagate to the
  `try/catch` in the test. But the call paths between `executeShortcut` and
  the macro sites traverse Qt-internal code — destructors, `noexcept`
  functions, event-loop reentry — that is not guaranteed to be
  exception-safe. If an exception unwinds through a `noexcept` function or a
  destructor, `std::terminate` fires, which is worse than a clean SIGABRT:
  it kills the process *without* the structured error handling the test
  relies on. Auditing every call path for exception safety is fragile and
  would have to be repeated each time a new `ASSERT_NO_EDITOR_MODE` site is
  added.

* **Silent swallowing.** Intermediate `catch(...)` blocks in core or Qt
  code could catch the exception without re-throwing, hiding the error
  entirely. The test would see no exception, report a pass, and the
  `ASSERT_NO_EDITOR_MODE` violation would go undetected — the exact failure
  mode the test is meant to prevent. The out-of-process approach has no
  such risk: a crash (SIGABRT) is an uncatchable, definitive signal.

## Decision Drivers

* **Exception safety.** The test must not rely on exceptions propagating
  cleanly through Qt-internal code paths. The out-of-process crash model
  (SIGABRT) is immune to `noexcept` / destructor / `std::terminate`
  hazards that would defeat an in-process throw-and-catch approach.
* **No silent swallowing.** The test signal must be uncatchable. A
  `try/catch`-based in-process test could have its exception silently
  swallowed by intermediate `catch(...)` blocks in core or Qt, producing a
  false pass. A process crash cannot be swallowed.
* **The assert is debug-only.** The test must run only in debug builds
  (`H2CORE_HAVE_DEBUG` and `NDEBUG` not defined), matching the macro's own
  `#if` guard. In release builds the macro is a no-op and the test would
  always pass trivially — registering it would be misleading.
* **Coverage must be enumerable and growable.** The `Shortcuts::Action` enum
  (`src/core/Preferences/Shortcuts.h:44-240`) already enumerates every GUI
  action — transport, mixer, timeline, drumkit, playlist, debug dialogs,
  view toggles, virtual keyboard — and `MainForm::handleKeyEvent`
  (`MainForm.cpp:2915-3694`) already dispatches all of them through the
  exact code paths the GUI uses at runtime. This is the natural coverage
  surface: adding a new `Shortcuts::Action` value automatically extends
  test coverage.
* **The test must not block.** Some actions open modal dialogs
  (`InputCaptureDialog`, `QMessageBox`, `AboutDialog`, etc.) which block
  forever in headless offscreen mode. The test must either skip these or
  supply predefined argument values so the dialogs are never shown.
* **Standalone behaviour stays unchanged.** The refactoring to enable the
  test must not alter the production dispatch path. The existing
  `handleKeyEvent` flow — key sequence lookup → `InputCaptureDialog` for
  argument-taking actions → action execution — remains the production path;
  the test injects arguments through a separate entry point.
* **The assert is incremental.** The test is designed to pass today and to catch
  regressions as more `ASSERT_NO_EDITOR_MODE` sites are added. It does not
  require all call sites to be guarded first.

## Considered Options

1. **Out-of-process integration test driven by the `Shortcuts::Action` enum,
   with `handleKeyEvent` refactored to separate argument acquisition from
   action execution.**

   A test harness (same pattern as `PluginEditorSmoke.cpp`) stands up a
   `HydrogenPlugin` serving its engine over IPC, then spawns the real GUI
   with `--connect-via-ipc` + a new `--exercise-editor-paths` flag. The flag
   launches an `EditorPathExerciser` that iterates over all
   `Shortcuts::Action` values (minus a skip list of file-dialog/external-
   browser actions), supplies predefined safe argument values for
   argument-taking actions, and dispatches each through the refactored
   `executeShortcut()` — the same code path `handleKeyEvent` uses in
   production. A clean (zero) exit proves no `ASSERT_NO_EDITOR_MODE` fired;
   a SIGABRT proves one did, and the log shows which action triggered it.

2. **Out-of-process integration test that auto-discovers `QAction`s from the
   menu bar and triggers them via `QAction::trigger()`.**

   Rejected: coverage is implicit (whatever menus happen to exist), adding
   a new menu item does not automatically extend coverage, argument-taking
   actions are not handled (each would need a per-action special case), and
   the dispatch path (`QAction::trigger()` → signal/slot) is indirect
   compared to the keyboard shortcut path the GUI actually uses at runtime.

3. **In-process CppUnit test that throws instead of asserting.**

   A compile-time flag (`-DH2_TEST_ASSERT_NO_EDITOR_MODE`) would make
   `ASSERT_NO_EDITOR_MODE` throw an `H2Exception` instead of calling
   `assert(false)`. The test wraps each `executeShortcut` call in
   `try/catch`, catches the exception, records which action triggered it,
   and continues. Simpler and faster than out-of-process — runs in the
   existing CppUnit suite, shares `TestHelper` / `FakePluginHost`
   infrastructure, no process spawn or IPC.

   Rejected on two grounds (see Decision Drivers): **exception safety** —
   the exception must unwind through Qt-internal code (destructors,
   `noexcept` functions, event-loop reentry) that is not guaranteed to be
   exception-safe; if it unwinds through a `noexcept` function,
   `std::terminate` fires, killing the process without the structured error
   handling the test relies on, and the audit burden grows with every new
   macro site. **Silent swallowing** — intermediate `catch(...)` blocks in
   core or Qt could catch the exception without re-throwing, hiding the
   violation and producing a false pass. The out-of-process crash model
   (SIGABRT) is immune to both: a crash is an uncatchable, definitive
   signal that cannot be swallowed or derailed by exception-unwind
   hazards.

4. **Static source analysis (like `WriteSurfaceGuard`) that grep-checks
   `src/gui` for direct driver access.**

   Rejected: the problem is not GUI code reaching drivers directly (that
   was already addressed by [ADR 0029](0029-audio-driver-access-across-editor-split.md))
   but **core methods** called by the GUI's action dispatch that internally
   access drivers. A static grep of `src/gui` would not catch a
   `Hydrogen::panic()` call reaching `getMidiDriver()->sendAllNotesOff()`.
   The assert lives in `src/core`; the test must exercise the call path from
   GUI action → core method → driver access.

5. **Use the existing `ShotList` infrastructure with a comprehensive shot
   list file.**

   Rejected: `ShotList` is Qt5-only (the Qt6 path is a no-op stub,
   `ShotList.h:114-122`), requires hardcoding widget names and slot names
   that change over time, has no auto-discovery, and has no mechanism to
   auto-close modal dialogs or supply argument values.

## Decision Outcome

Chosen option: **option 1 — out-of-process integration test driven by the
`Shortcuts::Action` enum, with `handleKeyEvent` refactored to separate
argument acquisition from action execution.**

### 1. The `ASSERT_NO_EDITOR_MODE` macro

The macro is already defined (`src/core/Hydrogen.h:642-668`) and its
semantics are settled: debug-only, checks `ProcessMode::Editor`, logs +
flushes + `assert(false)`. This ADR does not change the macro itself. It
records the intent to **systematically place it in core methods that
directly access audio drivers, MIDI drivers, or the OSC server** — methods
that are meaningful only on the authoritative engine and must never be
called on the mirror.

Some candidate call sites (to be guarded incrementally, not all at once):

| Method | File | Accesses |
|--------|------|----------|
| `Hydrogen::hasJackDriver()` | `Hydrogen.cpp:1173` | Audio driver (`dynamic_pointer_cast<JackDriver>`) |
| `Hydrogen::panic()` | `Hydrogen.cpp:1160` | MIDI driver (`sendAllNotesOff`) |
| `Hydrogen::hasJackTransport()` | `Hydrogen.cpp` | Audio driver (JACK transport) |
| `Hydrogen::toggleOscServer()` | `Hydrogen.h:374` | OSC server (`m_pOscServer`) |
| `Hydrogen::recreateOscServer()` | `Hydrogen.h:375` | OSC server (`m_pOscServer`) |
| `AudioEngine` driver init/connect/disconnect | `AudioEngine.cpp` | Audio/MIDI drivers |

The macro is placed at the **top of the method body**, before any driver
access, so the assert fires before the invalid access. It takes the
`Hydrogen*` pointer as its argument so it can check both null and editor
mode.

### 2. Refactoring `handleKeyEvent`

`MainForm::handleKeyEvent` (`MainForm.cpp:2915-3694`) currently interleaves
two concerns: **argument acquisition** (popping `InputCaptureDialog` for
1-arg, 2-arg, and many-arg actions) and **action execution** (the switch
body that calls `CoreActionController`, `MidiActionManager`, or MainForm
slots). The refactoring separates them:

```cpp
// MainForm.h

/// Arguments for a shortcut action. Empty for 0-arg actions.
struct ShortcutArgs {
    QString sArg1, sArg2, sArg3, sArg4;
};

/// Returns the number of arguments the action requires (0, 1, 2, or 4),
/// plus the InputCaptureDialog parameters (type, label, min, max) for each.
/// Pure metadata — no UI.
static int shortcutArgSpec( Shortcuts::Action action,
                            const std::shared_ptr<Song>& pSong,
                            /* out: types, labels, ranges */ );

/// Executes a shortcut action with pre-supplied arguments. This is the
/// existing switch body from handleKeyEvent, minus the InputCaptureDialog
/// calls. Production path: handleKeyEvent fills args from the dialog, then
/// calls this. Test path: EditorPathExerciser fills args from defaultArgs(),
/// then calls this.
void executeShortcut( Shortcuts::Action action, const ShortcutArgs& args );
```

The existing `handleKeyEvent` becomes:

```cpp
bool MainForm::handleKeyEvent( QObject* pQObject, QKeyEvent* pKeyEvent ) {
    // ... key sequence lookup (unchanged) ...
    for ( const auto& action : actions ) {
        ShortcutArgs args;
        int nArgs = shortcutArgSpec( action, pSong, /* ... */ );
        if ( nArgs > 0 ) {
            // Existing InputCaptureDialog flow — unchanged for production
            args = captureArgsFromDialog( action, nArgs, /* ... */ );
            if ( args.cancelled ) return true;
        }
        executeShortcut( action, args );
    }
    // ...
}
```

The production path is behaviourally identical: `handleKeyEvent` still pops
`InputCaptureDialog` for argument-taking actions, then calls
`executeShortcut`. The test path bypasses the dialog and calls
`executeShortcut` directly with predefined values.

### 3. The `EditorPathExerciser`

A new class, launched via `--exercise-editor-paths` after the GUI is fully
constructed and `ProcessMode::Editor` is assigned:

```cpp
// src/gui/src/EditorPathExerciser.h
class EditorPathExerciser : public QObject {
    Q_OBJECT
public:
    explicit EditorPathExerciser( QObject* parent = nullptr );
    void start();

private slots:
    void exerciseNext();

private:
    QList<Shortcuts::Action> m_actions;
    int m_nIndex = 0;

    static QSet<Shortcuts::Action> skippedActions();
    ShortcutArgs defaultArgs( Shortcuts::Action action ) const;
    void finish();
};
```

**Action selection** — built from `Shortcuts::getActionInfoMap()`, minus
an explicit skip list of actions that open modal file dialogs or external
browsers and would block or crash in offscreen mode:

* `OpenSong`, `OpenDemoSong`, `OpenPattern`, `OpenDrumkit`, `ImportDrumkit`,
  `ImportOnlineDrumkit` — file/network dialogs
* `ExportSong`, `ExportMIDI`, `ExportLilyPond`, `ExportDrumkit` — modal
  export dialogs
* `OpenPlaylist`, `SavePlaylist`, `SaveAsPlaylist` — file dialogs
* `PlaylistAddScript`, `PlaylistCreateScript`, `PlaylistEditScript` — file
  dialogs
* `OpenLogFile`, `OpenManual`, `ShowReportBug`, `ShowDonate` —
  `QDesktopServices::openUrl` / modal `QMessageBox`
* `Quit` — would exit before the exerciser finishes

**Argument supply** — `defaultArgs()` returns predefined safe values for
each argument-taking action (e.g. `BPM` → `"120"`, `SelectInstrument` →
`"0"`, `StripVolume` → `{"50", "0"}`, `LayerPitch` → `{"0", "0", "0",
"0"}`). Values are chosen to be in-range and non-destructive.

**Dispatch** — each action is dispatched via
`QTimer::singleShot(100, ...)` so the event loop processes between actions,
allowing async IPC commands and telemetry updates to settle. Each action
is logged via `___INFOLOG` so the test output shows exactly what was
exercised and which action triggered a crash.

**Finish** — after all actions, calls
`QApplication::closeAllWindows()` for a clean exit.

### 4. The `--exercise-editor-paths` CLI flag

Added to `Parser.cpp` alongside `--quit-after-startup` (same
`HiddenFromHelp` pattern). When set, `main.cpp` launches the
`EditorPathExerciser` via `QTimer::singleShot(500, ...)` after
`setProcessMode(Editor)` — the delay lets the initial IPC sync, mirror
state, and first paint settle.

### 5. The test harness binary

`PluginEditorAssertGuard.cpp` — nearly identical to
`PluginEditorSmoke.cpp`. It stands up a `HydrogenPlugin` (headless engine +
IPC server), spawns the real GUI with `--connect-via-ipc` +
`--exercise-editor-paths`, and checks the exit code:

| Exit code | Meaning | Test result |
|-----------|---------|-------------|
| 0 | All actions exercised, no assert fired | **PASS** |
| SIGABRT (signal) | `ASSERT_NO_EDITOR_MODE` triggered | **FAIL** — log shows which action |
| 3 (`EXIT_CODE_CLEAN_FAILURE`) | Editor could not connect to endpoint | **FAIL** — infrastructure problem |
| Other non-zero | Other crash or error | **FAIL** |

### 6. CTest registration

```cmake
# src/gui/CMakeLists.txt — debug-only, matching the macro's #if guard
if( H2CORE_HAVE_DEBUG AND NOT NDEBUG )
    add_executable( plugin_editor_assert_guard PluginEditorAssertGuard.cpp )
    target_link_libraries( plugin_editor_assert_guard
        hydrogen-plugin Qt${QT_VERSION_MAJOR}::Core )
    add_dependencies( plugin_editor_assert_guard hydrogen )

    add_test( NAME EditorModeAssertGuard
        COMMAND plugin_editor_assert_guard
                $<TARGET_FILE:hydrogen> ${CMAKE_SOURCE_DIR}/data )
    set_tests_properties( EditorModeAssertGuard PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_BINARY_DIR}/editor-assert-guard-test-home"
        TIMEOUT 180 )
endif()
```

The `H2CORE_HAVE_DEBUG AND NOT NDEBUG` guard mirrors the macro's own `#if`
condition. In release builds the macro is a no-op, so the test would always
pass trivially — registering it would be misleading.

### Coverage

The `Shortcuts::Action` enum has ~100 values. After the skip list (~20
file-dialog/external-browser actions), the exerciser covers ~80 actions
across:

| Category | Example actions | Why they matter for `ASSERT_NO_EDITOR_MODE` |
|----------|----------------|----------------------------------------------|
| Transport | `Play`, `Stop`, `PlayPauseToggle`, `PlayStopToggle`, `JumpToStart`, `JumpBarForward/Backward` | Go through `IpcCoreActionController` → IPC; any code that reaches the local audio driver hits the assert |
| Panic | `Panic` | Calls `Hydrogen::panic()` which accesses MIDI driver |
| BPM | `BPM`, `BPMIncrease/Decrease` (coarse/fine) | Tempo changes through `CoreActionController` |
| Mixer | `MasterMute/Unmute/Toggle`, `MasterVolumeIncrease/Decrease`, `StripVolume/Pan/FilterCutoff`, `StripMute/SoloToggle` | Audio parameter writes |
| Record | `RecordReady`, `RecordStrobe`, `RecordStrobeToggle`, `RecordExit` | Recording state changes |
| Timeline | `TimelineToggle`, `TimelineAddMarker`, `TimelineDeleteMarker`, `TimelineAddTag`, `TimelineDeleteTag` | Timeline modifications |
| Transport modes | `JackTransportToggle`, `JackTimebaseToggle`, `SongModeToggle`, `LoopModeToggle` | **JackTransportToggle/TimebaseToggle access the JACK driver directly** — prime assert candidates |
| Drumkit | `LoadNext/PrevDrumkit`, `NewDrumkit`, `SaveDrumkit`, `EditDrumkitProperties`, `AddInstrument`, `ClearAllInstruments`, `AddComponent` | Drumkit switching, instrument management |
| Views | `ShowMixer`, `ShowRack`, `ShowPlaylist`, `ShowDirector`, `ShowAutomation`, `ShowPlaybackTrack`, `ShowFullscreen` | Dockable view toggles — construction/destruction paths |
| Debug | `ShowAudioEngineInfo`, `ShowFilesystemInfo`, `LogLevel*` (5), `DebugPrintObjects` | **AudioEngineInfoForm queries driver state directly** — highest-risk path |
| Undo/Redo | `Undo`, `Redo`, `ShowUndoHistory` | Undo stack operations |
| Pattern/Song | `NewSong`, `EditSongProperties`, `SaveSong`, `SaveAsSong`, `ExportPattern` | Pattern/song management |
| Playlist | `PlaylistAddSong`, `PlaylistAddCurrentSong`, `PlaylistRemoveSong`, `NewPlaylist`, `SavePlaylist`, `SaveAsPlaylist`, `PlaylistRemoveScript` | Playlist operations |
| Virtual keyboard | `VK_36_C2` through `VK_59_B3` (24 notes) | Note triggering through `CoreActionController::handleNote` |
| Metronome/CountIn | `MetronomeToggle`, `CountIn`, `CountInPauseToggle`, `CountInStopToggle` | Audio engine state |
| BeatCounter/TapTempo | `BeatCounter`, `TapTempo` | Tempo detection |

### Incremental adoption

The refactoring of `handleKeyEvent` can be done in two steps:

1. **Phase 1 (minimal):** Extract the 0-arg and MainForm action cases into
   `executeShortcut`. The exerciser covers all 0-arg actions + all MainForm
   actions + virtual keyboard — ~70 of ~80 exercisable actions. This is the
   bulk of coverage and requires no `InputCaptureDialog` changes. Includes
   all high-risk paths: transport, panic, JackTransportToggle,
   AudioEngineInfo, view toggles, drumkit switching.

2. **Phase 2 (full):** Extract the 1-arg, 2-arg, and many-arg cases. Add
   `defaultArgs()` for each. Covers mixer strip operations, timeline marker
   editing, layer parameter editing, etc.

### Consequences

* **`handleKeyEvent` is refactored** to separate argument acquisition from
  action execution. The production path is behaviourally identical; the test
  path calls `executeShortcut` directly with predefined arguments.
* **New CLI flag `--exercise-editor-paths`** (hidden from help, like
  `--quit-after-startup`). Only meaningful in editor mode.
* **New test binary `plugin_editor_assert_guard`** and CTest
  `EditorModeAssertGuard`, registered only in debug builds.
* **Coverage grows automatically** as new `Shortcuts::Action` values are
  added — the exerciser iterates the enum, so new actions are covered
  without test changes.
* **The assert placement is incremental** — the test passes today and will catch
  regressions as more `ASSERT_NO_EDITOR_MODE` sites are added. A new guard that
  fires during an exercised action will show up as a SIGABRT with the action
  name in the log.
* **The test is debug-only** — it runs only when `H2CORE_HAVE_DEBUG` and
  `NDEBUG` is not defined, matching the macro. Release CI jobs skip it.
* **No standalone behaviour change** — the refactoring only adds a
  separation of concerns within `handleKeyEvent`; the production dispatch
  path is unchanged.

## More Information

* Macro definition: `src/core/Hydrogen.h:642-668`
* Shortcut enum: `src/core/Preferences/Shortcuts.h:44-240`
* Shortcut dispatch: `src/gui/src/MainForm.cpp:2915-3694`
* Existing editor-mode tests: `src/gui/PluginEditorSmoke.cpp`,
  `src/tests/ConnectViaIpcModeTest.cpp`, `src/tests/IpcTransportTest.cpp`
* Related: [ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0018](0018-plugin-editor-ipc-transport.md),
  [ADR 0027](0027-coreactioncontroller-single-write-surface.md),
  [ADR 0029](0029-audio-driver-access-across-editor-split.md),
  [ADR 0031](0031-decouple-engine-clock-from-audio-driver.md),
  [ADR 0032](0032-h2player-gui-connection-mode.md)
