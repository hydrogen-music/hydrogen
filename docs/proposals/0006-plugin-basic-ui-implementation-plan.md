# Proposal 0006 — Plugin basic UI: test-driven implementation plan

Status: proposed (plan) — 2026-10-05 — author: pm

Implements [ADR 0035](../decisions/0035-embedded-basic-plugin-ui.md) — the
embedded basic plugin UI (Dear ImGui) — and closes the LV2 findings from the
design review (zero UI-shim test coverage, off-contract null widget,
`ui:showInterface` misused as primary, `uiHide`/`uiCleanup` terminating the
editor, `uiIdle` returning 1 before any show). Methodology and conventions
follow [proposal 0004](0004-plugin-port-implementation-plan.md) §1; this plan
picks up where 0004's phases ended.

---

## 1. How we work (TDD methodology)

As 0004 §1: red → green → refactor; every behavior task lands its failing test
first; each task is a small reviewable commit that keeps the suite green.
Baseline today: **485** unit tests plus the ctest gate (**12/12**, incl. the
five plugin conformance tests `lv2-smoke`, `lv2lint`, `clap-validate`,
`vst3-smoke`, `vst3-validate`). Window smokes skip with a message when no
`DISPLAY`/window system is available — a skipped window smoke does not fail
the gate. Test vehicles: UI-0 behavior tests extend `lv2_smoke` (the shim is
only reachable via `dlopen`); model tests are CppUnit `*Test.h/.cpp` pairs per
0004 §1. Pure vendoring/build tasks (UI-1) are verified by building everywhere
plus the existing smokes rather than by new red tests. Per-phase status
markers are adopted as work lands, as in 0004.

---

## 2. Scope & dependency graph

In scope: the LV2 findings fixes, vendoring, the window/backend seam with its
de-risk spike, the engine event/meter fan-out, the three views, the LV2/CLAP
swap (VST3 via clap-wrapper), and hardening.

Out of scope: LV2 transport (`time:Position`) sync — a recorded follow-up
(`HydrogenLv2.cpp:172`); host parameter automation
([ADR 0021](../decisions/0021-no-host-parameter-automation-v1.md)); any change
to the editor GUI itself.

```
UI-0 (LV2 shim fixes)   independent — lands first, no toolkit needed
UI-1 (vendoring) ──┬──→ UI-2 (window seam + spike gate) ──┐
                   └──→ UI-3 (engine fan-out) ───────────┴──→ UI-4 (views)
                                                              └→ UI-5 (formats) ─→ UI-6 (hardening)
```

Critical path: **UI-1 → UI-2 → UI-4 → UI-5 → UI-6**. UI-3 is engine-side and
parallelizes with UI-2 — but UI-4's mixer view additionally requires UI-3's
sink. UI-0 can land immediately.

---

## 3. Phase UI-0 — LV2 shim findings: coverage + semantics fixes

**Status: ✅ DONE** (2026-10-05) — TU0.1–TU0.4 landed; lv2-smoke + plugin gate
(12/12) + unit suite (485) green.

*Objective:* close the LV2 review findings that stand alone, before any
toolkit exists. The interim behavior stays coherent: `show` spawns the editor
as today; the editor window is self-managed (the user closes it themselves);
the plugin instance owns the editor lifecycle
(`HydrogenPlugin::~HydrogenPlugin` → `closeEditor()`,
`HydrogenPlugin.cpp:190-198`).

**Tests first**

* **TU0.1** `lv2_smoke` resolves `lv2ui_descriptor` (`HydrogenLv2.cpp:302`),
  asserts the UI URI, checks `extension_data` serves `showInterface` +
  `idleInterface`, instantiates the UI headless (instance-access feature, no
  parent → no window) and cleans up. New coverage — green immediately against
  the current shim; it exists so every later UI task has a net.
* **TU0.2** red: `uiIdle` returns **0** before any `show()` — a host idling a
  freshly instantiated UI must not be told to destroy it. Today it returns 1
  whenever no editor process runs (`HydrogenLv2.cpp:269-277`).
* **TU0.3** red: `uiHide` does not close the editor (test seam:
  `openEditor(false)`; assert `isEditorOpen()` survives hide). Today
  `uiHide` → `closeEditor` (`HydrogenLv2.cpp:261-267`).
* **TU0.4** red: `uiCleanup` does not close the editor either (today
  `HydrogenLv2.cpp:241-249`). The editor dies with the DSP instance, not the
  UI ([ADR 0035](../decisions/0035-embedded-basic-plugin-ui.md)).

**Tasks:** TU0.1–TU0.4 as above; `uiIdle` returns 0 for the lifetime of the
handle — an **interim rule, superseded by TU5.2 at UI-5** (non-zero only
after the fallback window closes). Until then, `showInterface` hosts (jalv
& co.) idle a dead launcher forever after the user closes the editor
window — harmless (a cheap poll) and cheaper than the current
false-positive destruction path.

**Done when:** all four green; unit suite + ctest gate green.

---

## 4. Phase UI-1 — Vendoring & build scaffolding

**Status: ✅ DONE** (2026-10-06) — TU1.1–TU1.3 landed; Linux plugin gate 13/13
(incl. the new `PluginUiGuard`) + unit suite 485 green; the vendored TUs
compile warning-free under the strict flags (no relaxations needed);
Windows + macOS CI green.

*Objective:* Dear ImGui + widget add-ons vendored per the house pattern and a
`hydrogen-plugin-ui` library that builds on every platform and Qt ≥ 5.15.

**Tasks**

* **TU1.1** Submodules under `extern/` (the established vendoring pattern —
  our SDKs are submodules detected by path, root `CMakeLists.txt:452-487`;
  the only CPM/FetchContent use in the tree is the opt-in VST3 SDK fetch
  inside clap-wrapper, `src/plugin/CMakeLists.txt:176-183`): `extern/imgui`
  (ocornut, MIT, pin the current v1.92.x),
  `extern/imgui-knobs` (altschuler, MIT), `extern/ImGuiFileDialog` (aiekick,
  MIT).
* **TU1.2** `hydrogen-plugin-ui` static library (`src/plugin/ui/`): compiles
  the imgui core + platform backends (GL3 on Linux/Windows, the osx/Metal
  backend on macOS) + the two add-ons; links `hydrogen-plugin`; linked by
  `hydrogen-lv2`, `hydrogen-clap`, and `hydrogen-vst3-entry`. Warning-policy
  relaxations scoped to the vendored TUs only (clap-wrapper precedent,
  `src/plugin/CMakeLists.txt:286-313`).
* **TU1.3** Model/renderer split as the testability seam: view *models* are
  pure logic with no imgui types in their headers (they link into the unit
  suite headless); *renderers* are thin imgui draw calls covered by the
  smokes. Established before any view exists so no logic grows in the
  untestable layer. A `WriteSurfaceGuard`-style grep guard lands with it: no
  `QApplication` and no `src/gui` includes in `src/plugin/ui/` — ADR 0035's
  no-Qt-Widgets consequence, CI-enforced.

**Done when:** Linux + Windows + macOS CI builds green; suite green; no new
warnings from our own TUs.

---

## 5. Phase UI-2 — Window/backend seam + de-risk spike (gate)

**Status: ✅ DONE** (2026-10-07) — TU2.1–TU2.4 landed. TU2.1 red→green (window smoke),
TU2.3 X11 backend + LV2 `ui:parent` slice (lv2-smoke embedded section
red→green), TU2.2 Win32/macOS equivalents written (CI-gated); Linux plugin gate
14/14 (incl. the new `PluginUiSmoke`) + unit suite 485 green. First TU2.4 host
round exposed the shim scanning a non-spec parent feature (`parentWidget` —
no such URI in the LV2 UI ontology; hosts pass `ui:parent`): fixed red→green
and the embedded child window verified live in jalv (mapped in the X11 tree,
panel rendering). `ui:idleInterface` stays in requiredFeature (both UI faces
are idle-driven; lv2lint enforces the feature/extensionData pairing) — run
lv2lint with `-t 'UI*Widget'` (its widget check passes only required
features, so it cannot express ADR 0035's optional-parent fallback; documented
in the TTL) — the plugin gate's lv2lint ctest whitelists that plus the
pre-existing Plugin Run/Symbols trade-offs, and replaces LV2_PATH with the
build bundle parent + system spec dirs so a stale `~/.lv2` install can't
shadow the build bundle. The TU2.4 spike gate passed: the plugin loads and
embeds in all local hosts and all CI pipelines are green (after the imgui
1.92 Metal render-pass-descriptor fix, the GL-less-CI environmental skip,
and the ObjC++ compile of the shared smoke on APPLE). Spike follow-ups
(DPI/scale, focus/IME) recorded for TU6.2.

*Objective:* parent handle → child window → GL/Metal context → per-instance
ImGui context; the five [ADR 0035](../decisions/0035-embedded-basic-plugin-ui.md)
de-risk items answered with evidence *before* the views are built. The spike
gate is manual — an accepted deviation from 0004 §1: no CI host can answer
X11-GL-on-host-parent.

**Tests first**

* **TU2.1** red (Linux, xvfb): UI smoke creates a child window on a
  test-provided X11 parent, idles N frames, destroys — no crash, child
  mapped. Skip-with-message when no `DISPLAY` (CI guard).
* **TU2.2** Win32/macOS equivalents, CI-gated the same way.

**Tasks**

* **TU2.3** `PluginUiWindow` backend (`src/plugin/ui/`): X11 child window
  adapted from DPF-Widgets (ISC — attribution in the header), Win32 child
  `HWND`, `NSView` child; context creation; per-instance ImGui context with
  explicit `SetCurrentContext` discipline at idle entry (hosts idle multiple
  instances sequentially on one thread — ADR 0035's recorded obligation).
* **TU2.4** **Spike gate** (manual, Ardour + Qtractor + Carla + REAPER):
  embed a trivial panel; check X11 GL context creation on host parents,
  keyboard focus/IME routing, DPI/scale, macOS Metal ("not well tested"
  upstream), and clap-wrapper's `set_parent` → `IPlugView` translation
  (`vst3-validate` + a manual host). **Kill/pivot:** if X11-GL-on-host-parent
  proves unworkable, pivot per ADR 0035 (imgui_sw software renderer first)
  and record the outcome as an ADR 0035 amendment.

**Done when:** spike evidence recorded; gate passed or pivot decided; window
smokes green.

---

## 6. Phase UI-3 — Engine seam: event & meter fan-out (ADR 0035)

**Status: ✅ DONE** (2026-10-07) — TU3.1–TU3.5 landed; unit suite (491) +
plugin gate (14/14) green. TU3.4's side ticket dropped: the toggle crossing
already works composed over the `setStripIsMuted`/`setStripIsSoloed`
overrides; the dedicated-opcode atomicity may still land independently
(§12).

*Objective:* both UIs observe the same engine state without splitting events
or halving meters.

**Tests first**

* **TU3.1** red: with `EngineSession` serving (test seam `openEditor(false)`)
  **and** a registered in-process sink, events pushed into the `EventQueue`
  reach **both** the editor client and the sink; one peak-consume pass, copies
  to both (meter values equal, not halved). The same fan-out holds with **no**
  editor session at all (basic-UI-only mode).
* **TU3.2** the sink's own fan-out queue (not the `EventQueue` — the bridge
  thread stays its sole drainer) is drained by the UI idle; nothing lost on
  teardown.

**Tasks**

* **TU3.3** `EngineEventSink` interface: `onEvent` + `onMeterSnapshot`,
  registered on `HydrogenPlugin`. The bridge thread stays the **sole
  drainer** of the `EventQueue` (`EngineSession::forwardEvents` /
  `discardEvents`, `EngineSession.cpp:198-273`) and fans out to the sink;
  `buildTelemetrySnapshot` (`EngineSession.cpp:288-383`) copies the snapshot
  to the sink alongside `publishTelemetry`. Error replay
  ([ADR 0026](../decisions/0026-plugin-mode-feature-disablement-and-gui.md)
  amendment) is visible to the sink.
* **TU3.4** Command path: UI models call the local `CoreActionController`
  under the engine lock. The inventory already exists — `setStripVolume`
  (`CoreActionController.h:88`), `setStripPan` (`:100`), `setStripIsMuted`
  (`:362`), `setStripIsSoloed` (`:373`), `loadSong` (`:413`), `setDrumkit`
  (`:772`), `loadPattern` (`:930`), `setMidiControlSettings` (`:506`),
  `setMidiInstrumentMap` (`:492`), `setInstrumentMidiOutNote`/`Channel`
  (`:282`/`:288`). Side ticket: override `toggleStripIsMuted`/
  `toggleStripIsSoloed` in `IpcCoreActionController` (currently mirror-only
  in editor mode).
* **TU3.5** Decouple the drain/fan-out loop from editor serving: today the
  bridge thread exists only inside `EngineSession`, started by `openEditor()`
  (`HydrogenPlugin.h:116-124`) — a host that embeds the basic UI and never
  opens the editor would have no drainer at all. The drain/fan-out runs in
  plugin mode regardless of editor attachment — either by always starting
  the `EngineSession` bridge (its no-editor branch already drains +
  publishes, `EngineSession.cpp:122-132`) or by splitting the drain loop
  out; decided at implementation with TU3.1's no-editor case as the red
  test.

**Done when:** fan-out tests green; a UI-model command round-trips into
engine state (mirror assertions).

---

## 7. Phase UI-4 — The three views

**Status: ✅ DONE** (2026-10-08) — TU4.0–TU4.4 landed; unit suite (502) +
plugin gate (14/14) green, the window smoke PASSED with the views rendering
on a real embed. TU4.0's gate: 36 approved `PluginUiStrings` strings
(feedback + transport deliberately unexposed — engine-side, settable via
the editor; the class was later absorbed into
`src/core/CommonStrings.h` at UI-6, see TU6.1); the catalog wiring
lands at UI-6 (TU6.1). TU4.4 added
`Instrument::m_nOutputBus` through the full members-changes checklist
(ADR 0019: explicit-only persistence). The REAPER spike gate found the
deferred input translation still missing (the X11 backend only counted
events); the full X11 translation — mouse, wheel, keys, focus — landed
with it, synthetic-event-checked in the smoke; Win32/macOS input wiring
moved to TU6.2. A second REAPER spike round found three more UI-4 gaps,
all fixed red→green: the mixer's bus-indicator drag source asserted on
its ID-less `Text()` item (imgui requires `SourceAllowNullID` for
those), every mixer row loop ended in a trailing `SameLine()` so all
rows collapsed onto one horizontal line (now between-only), and the
channel widgets showed raw "-1"/"0" where the GUI's spinboxes read
"all"/"Off" (now dropdowns fed by the model's `channelDisplayName` —
unit-tested, suite 503). The smoke gained the coverage that reproduced
the crash headless: a warm-up frame before the held press (imgui
suppresses hovering for presses submitted before any mouse-position
event), a y-dense sweep grid, and a mixer layout check. A third REAPER
spike round found seven more UI-4 gaps, all fixed: the landing view's
action channel was still a raw `InputInt` (now the same all/Off/1..16
dropdown — `channelDisplayName` moved from the mapping model to
`PluginUiStrings`, both views need it, and the invalid -2 of unmapped
rows now reads "Off" like the GUI's disabled widgets), the first tab
"Song" renamed "General", the mixer's six-row layout replaced by
fixed-width strip columns (names elided to the column width, the
routing béziers at full alpha so the established connections read as
more than a watermark), the embedded window's 300×200 initial footprint
grown to 960×600 (the host resizes the child with its FX window
anyway), and the smoke gained a scrollability check — the overflowing
strips must stay reachable through imgui's automatic horizontal
scrollbar, verified headless (suite 503, plugin 14/14). macOS CI
follow-up (2026-10-09): the first AppVeyor macOS run of the smoke
(Xcode 14.2 / MacOSX13.1 SDK) failed inside the SDK — `IOLLEvent.h`
skips all its `NX_*` definitions when `EVENT_H` is predefined (a
dpsclient/event.h interlock), and `Basics/Event.h`'s bare `EVENT_H`
include guard tripped it in the ObjC++ smoke TU. The guard is now
`H2C_EVENT_H`, locked tree-wide by the header-guard hygiene tests
(`src/tests/HeaderGuardHygieneTest.cpp` — every include guard must be
`H2C_`-prefixed and unique, red→green); the macOS pipeline re-run is
the remaining CI-gated proof. The MIDI configuration foldable section
moved (2026-10-18) from the landing view to the mapping view — its
model logic (the scalar MIDI control applies/reads and the wholesale
map apply) moved with it from `PluginUiLandingModel` to
`PluginUiMappingModel`, leaving the landing model with the
load/install/editor commands; the section still renders with the
engine defaults when no engine is available. The landing view was
reworked (2026-10-18): song/pattern loading is now a mutually
exclusive tab pair (each tab keeps its keyed browser), the drumkit
file browser became a sound-library picker (`SoundLibraryDatabase`
listing — name/author/license/version columns, the kit's notes on
hover, row select loads the kit), and the editor line at the bottom
gained an executable path field prefilled with the resolved default
(an empty field restores the default resolution).

*Objective:* landing, MIDI mapping, mixer — model-first, each view's logic
unit-tested headless, renderers thin.

**Tests first** (model level)

* **TU4.a** landing: load commands invoked with the chosen paths; editor
  button → `openEditor`.
* **TU4.b** mapping: `MidiInstrumentMap` round-trips (input/output modes,
  global channel, per-instrument rows) — mirroring the Instrument Mapping tab
  (`MidiControlDialog.cpp:397-671`); MIDI config → `setMidiControlSettings` /
  `setMidiInstrumentMap` through the
  [ADR 0022](../decisions/0022-layered-plugin-configuration.md) plugin-instance
  layer — never written back to `~/.hydrogen` (moved from TU4.a together with
  the section).
* **TU4.c** mixer: strip commands; meter values from sink snapshots; bus
  connections round-trip in the embedded song state
  ([ADR 0017](../decisions/0017-embed-song-in-plugin-state.md) /
  [0019](../decisions/0019-plugin-output-bus-layout.md)). **If new serialized
  members are needed, the members-changes checklist applies** (serialization,
  stringification, `RoundTripAssertions`, `IpcRoundTripTest`, `data/` +
  `src/tests/data` artifacts).

**Tasks**

* **TU4.0** **i18n decision gate (ask first — house rule), before any view
  string exists:** decide the plugin-side string mechanism. Proposed
  default: Qt `tr` reuse — the plugin already links Qt Gui, so
  `Q_DECLARE_TR_FUNCTIONS` on a plugin-side string class joining the
  existing `data/i18n` catalogs (not `CommonStrings.h` — that lives in
  `src/gui`, which the plugin must not link). The wiring itself lands at
  UI-6 (TU6.1).
* **TU4.1** landing view (song/pattern load tabs with keyed
  ImGuiFileDialog browsers, sound-library drumkit picker, editor line
  with executable path field).
* **TU4.2** MIDI mapping view (settings grid + per-instrument rows + the
  MIDI config widgets, moved from TU4.1).
* **TU4.3** mixer strips (fader, ImGuiKnobs pan encoder, mute/solo, meters).
* **TU4.4** bus drag&drop curves (imgui `BeginDragDropSource`/`Target` +
  `DrawList` béziers; the connection model is ADR 0019's deferred remapping
  widget — no automatic sharing between instances).

**Done when:** all three views render in the spike hosts; model tests green;
song-state round-trip green.

---

## 8. Phase UI-5 — Format integration: LV2 swap, CLAP embedded, VST3

**Status: ✅ DONE** (2026-10-08) — TU5.2–TU5.7 landed (TU5.1 was pulled into
UI-2); suite (503) + plugin gate (14/14) green, both smokes PASSED, lv2lint
and clap-validate (11/0) green. The seam gained `shouldClose()` + the
parentless top-level mode on all three backends (X11 smoke-checked; Win32
WM_CLOSE via a class WndProc, macOS `windowShouldClose:` via a delegate —
CI-gated) and `getSize`/`setSize` (smoke-checked). LV2: `show` creates /
`hide` destroys the fallback window, `idle` returns non-zero only after the
user closed it (`bEmbedded` distinguishes the modes); the lv2_smoke
unguarded section now asserts the always-on-session semantics and a new
X11 section drives the full fallback contract (root-tree diff + WM_CLASS
walk + synthetic WM_DELETE_WINDOW). CLAP: both dispositions over the one
seam — `guiSetParent` embeds, floating `guiShow` self-manages — with the
 frame pump on `clap.timer-support` (16 ms; the gui extension has no idle of
 its own), `guiGetSize`/`guiSetSize` real, and `guiDestroy`/`guiHide` sparing
 the editor.
 
 **Host gates (2026-10-08):** REAPER loads all three formats cleanly, but
 Qtractor (LV2+CLAP inserted, VST3 on insert) and Ardour (VST3 alone, on UI
 open) SIGSEGV'd. Root cause: symbol interposition — every module exported
 its whole statically linked interior (core, Qt, ImGui, and the VST3's
 wrapped `clap_entry`), so with one Hydrogen binary in the host's global
 symbol scope the VST3 module's references resolved into the other binary's
 copies (22k+ bindings observed under `LD_DEBUG=bindings`); the host's
 scanner `dlclose()` then left them dangling. Fixed with Linux version
 scripts exporting exactly the format entry points
 (`clap/hydrogen-clap.map`, `lv2/hydrogen-lv2.map`,
 `vst3/hydrogen-vst3.map` — the VST3 one keeps the SDK's `ModuleEntry`/
 `ModuleExit` contract); locked in by the `symbol-hygiene-*` nm gates and
 `vst3-interpose-smoke` (CLAP `RTLD_GLOBAL` load + scanner `dlclose` + full
 VST3 `ModuleEntry`→`createInstance`→`initialize`→`terminate`→`ModuleExit`
 drive). Plugin gate 18/18, lv2lint green without the `*Symbols` skips.
 Manual re-test of Ardour/Qtractor pending.

*Objective:* the shims become the parent-handle + host-idle seam per
[ADR 0035](../decisions/0035-embedded-basic-plugin-ui.md).

**Tests first**

* **TU5.1** red: `lv2_smoke` (xvfb): `uiInstantiate` with the `ui:parent`
  feature → `*widget != nullptr` (a real child window id). Pulled forward
  into UI-2 for the spike (done); the first cut scanned a non-spec
  `parentWidget` URI — the ontology's parent feature is `ui:parent`.
* **TU5.2** `showInterface` fallback: `show` creates the self-managed
  window, `hide` destroys it; `idle` returns 0 while alive, non-zero only
  after the fallback window closes; `uiCleanup` tears down the GL/ImGui
  contexts and the fallback window without leaking or crashing (ADR 0035's
  teardown obligation).
* **TU5.3** CLAP: `guiIsApiSupported` true for embedded too; `guiSetParent`
  → child window; `guiShow`/`guiHide` present/hide; `guiGetSize`/`guiSetSize`
  real for the embedded path; floating shows the basic UI window (not the
  editor); `guiDestroy`/`guiHide` spare the editor (today both close it,
  `HydrogenClap.cpp:237-242, 269-274`).

**Tasks**

* **TU5.4** LV2 swap: `H2Lv2Ui` → the basic UI; TTL rewrite
  (`hydrogen.ttl.in:41-54` — real embedding, `showInterface` documented as
  fallback; `requiredFeature instance-access + ui:idleInterface` and
  `extensionData showInterface + idleInterface` stay); `lv2lint` green
  **including the X11 UI test**.
* **TU5.5** CLAP swap (`HydrogenClap.cpp:208-281`): embedded + floating per
  ADR 0035; `guiDestroy` tears down the UI, not the editor.
* **TU5.6** VST3: `vst3-validate` + manual host check of the
  clap-wrapper `IPlugView` translation.
* **TU5.7** Behavior change + **CHANGELOG** (user-facing): `show` no longer
  spawns the editor directly — the landing view's button owns that.

**Done when:** Ardour embeds and drives the full UI; the Qtractor/Carla
fallback path works; `lv2lint` (incl. X11), `clap-validate`, `vst3-validate`
green.

---

## 9. Phase UI-6 — Hardening, i18n, packaging

**Status: ✅ DONE** (2026-10-12) — TU6.1–TU6.4 landed; unit suite (508)
+ plugin gate 18/18 green. The Win32/macOS input wiring is
compile-gated (AppVeyor); real-host behavior stays under the manual
spike gate.

**Tasks**

* **TU6.1** i18n wiring — **landed (2026-10-12)**:
  `HydrogenPlugin::installTranslator()` loads the hydrogen catalog for
  the user's preferred languages onto the process's QCoreApplication
  (the host's if it runs one, else the ensured minimal one), with a
  remove-probe making it idempotent and self-healing;
  `testTranslatorInstallation` covers it (suite 507). Same-day
  follow-up superseding the TU4.0 default: `CommonStrings` moved from
  `src/gui/src/` to `src/core/` (namespace `H2Core`; Qt keys catalogs
  on the stringified class name, so all 327×21 existing translations
  survive byte-identical) and absorbed the plugin's string class — 13
  texts reuse existing GUI entries, 25 new getters plus
  `getChannelDisplayName()`; `PluginUiStrings` is deleted and its
  catalog context never enters git. GUI and plugin share one
  translation source now.
* **TU6.2** DPI/scale and focus/IME fixes from the spike findings, plus
  the Win32/macOS input translation wiring (X11 landed with UI-4 after
  the REAPER spike found the gap — the backend had only counted events;
  their `pumpEvents` stays a no-op until then) — **landed (2026-10-12)**:
  X11 gained XIM (`XOpenIM`/`XCreateIC`, `XFilterEvent`,
  `Xutf8LookupString` with a no-IM `XLookupString` fallback); Win32
  records native events in the WndProc into a queue that `pumpEvents()`
  drains under the per-instance context (per-window DPI via
  `GetDpiForWindow`, WM_CHAR UTF-16, IME composition through
  `DefWindowProcW`); macOS does the same from an NSView responder
  subclass implementing NSTextInputClient (composed text via
  `insertText:`, app-active notifications clear keys stuck across app
  switches). One cross-platform fix surfaced by the wiring: imgui
  orders mouse buttons 0 left / 1 right / 2 middle — X11 and Win32 had
  middle and right swapped.
* **TU6.3** Third-party notices for the three new submodules — **landed
  (2026-10-12)**: `src/plugin/THIRD-PARTY-NOTICES` carries the verbatim
  MIT texts; cmake ships it inside the LV2 bundle, in the VST3 bundle's
  `Contents/Resources/`, and next to the installed CLAP binary
  (`hydrogen.third-party-notices`).
* **TU6.4** ADR 0035 status flip to accepted + spike-outcome amendments;
  CHANGELOG entry if not already landed at TU5.7 — **landed
  (2026-10-12)**: ADR 0035 accepted and amended with the de-risk
  outcomes; the CHANGELOG gained the embedded-basic-UI entry (TU5.7
  had not landed one).

**Done when:** the Definition of Done below holds.

---

## 10. Test traceability (ADR 0035 → phase)

| ADR 0035 obligation | Task |
|---|---|
| Real child window (null-widget fix) | TU5.1, TU5.4 |
| `showInterface` as fallback only | TU5.2, TU5.4 |
| `uiHide`/`uiCleanup` spare the editor | TU0.3, TU0.4 |
| `uiIdle` drives the frame, returns 0 while alive | TU0.2, TU5.2 |
| Event/meter fan-out, bridge thread sole drainer | TU3.1–TU3.3, TU3.5 |
| Error replay visible to the sink (ADR 0026) | TU3.1, TU3.3 |
| Threading seam + context discipline | TU2.3, TU3.4 |
| Three views + 0019 remapping obligations | TU4.1–TU4.4 |
| CLAP embedded + floating disposition | TU5.3, TU5.5 |
| VST3 via clap-wrapper | TU5.6, TU2.4 |
| Smoke coverage from the start | TU0.1 |
| i18n open question | TU4.0, TU6.1 |
| De-risk list | TU2.4 (gate) |

---

## 11. Cross-cutting practices

* Suite green at each commit; each task a small reviewable commit (0004 §1).
* Vendored code never gates our warning policy — relaxations scoped
  per-target.
* Members-changes checklist whenever song-state members are touched (TU4.4).
* Translatable-string gate: ask before adding any (TU4.0).
* `WANT_VST3` stays default OFF (heavy SDK fetch); UI work must not flip it
  silently on CI.

---

## 12. Open implementation details

* imgui_sw software-renderer fallback — decide after the TU2.4 spike.
* xvfb availability on the Linux CI runners; else the window smokes stay
  skip-gated and the spike gate covers them manually.
* The `IpcCoreActionController` toggle gap (TU3.4 side ticket) may land
  independently of this plan.
* Exact pin versions for the three submodules (latest stable at TU1.1 time).

**Definition of Done:** the plugin is usable end-to-end in Ardour without the
editor — load song/pattern/drumkit, MIDI config, instrument mapping, mixing
with meters, bus curves, and the editor button — with the unit suite and the
ctest gate (incl. `lv2lint` X11, `clap-validate`, `vst3-validate`) green,
ADR 0035 accepted and amended with the spike outcomes, and the CHANGELOG
entry landed.
