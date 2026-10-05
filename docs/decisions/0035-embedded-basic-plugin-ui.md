---
status: proposed
date: 2026-10-05
deciders: phil (theGreatWhiteShark)
---

# AD: Ship an embedded basic plugin UI built on Dear ImGui

## Context and Problem Statement

[ADR 0016](0016-out-of-process-plugin-ui.md) made the plugin binary
"engine and a thin controller" and moved the entire Qt editor out of
process. The consequence inside embedding hosts: the plugin has **no UI
at all**. Ardour — the dominant Linux LV2 host — embeds UIs via suil and
never queries `ui:showInterface`, so Hydrogen is invisible there; hosts
that do call `showInterface` only get the floating editor.

The current LV2 UI declaration is also off-contract: it declares a
platform-native UI type (`ui:X11UI` on Linux, `ui:CocoaUI` /
`ui:WindowsUI` elsewhere) but instantiates with a null widget (the
platform classes define the widget as a native window/view id), and it
treats `ui:showInterface` as the primary show mechanism, while the spec
defines it as a *fallback* for UIs a host cannot embed.

The plugin should be **self-contained**: loading a song, pattern, or
drumkit, setting MIDI options, and basic mixing must work without
opening the editor. Advanced editing stays in the out-of-process editor.

## Decision Drivers

* Self-contained plugin: usable without the editor
* Must render inside embedding hosts (Ardour/suil on X11 is the primary
  Linux path) — requires a real child window on the host-provided parent
* LV2 UI spec constraints: UI code MUST NOT use singletons or global
  state; the host drives rendering via idle (~30–60 Hz)
* One UI codebase across LV2, CLAP, and VST3 (which inherits the CLAP
  gui extension through clap-wrapper,
  [ADR 0014](0014-plugin-format-strategy.md))
* Low footprint, easy to use, GPL-2.0-or-later-compatible licensing
* Keep [ADR 0016](0016-out-of-process-plugin-ui.md)'s out-of-process
  editor for advanced editing

## Considered Options

1. **Dear ImGui** (MIT) — immediate-mode widgets; official Win32/macOS
   parent-window backends; X11 child-window backend adapted from
   DPF-Widgets (ISC)
2. **DPF/DGL + DPF-Widgets** (ISC) — the proven FOSS plugin-UI stack;
   draw-everything widget model; no file browser, no table/grid
3. **VSTGUI** (BSD-3) — purpose-built attach and a real widget set
   (knob, fader, native file selector); Linux labelled "Preview"; LV2
   attach needs an IRunLoop substitute
4. **LVGL** (MIT) — retained widgets incl. spinbox and file explorer,
   pure software rendering; C API; manual drag&drop
5. **Nuklear** (MIT/PD) — tiny, but no Cocoa/Metal backend and no
   plugin precedents
6. **Disqualified**: JUCE 8 (AGPLv3/commercial — incompatible with
   GPL-2.0-or-later), iPlug2 (no LV2 support), Slint (GPLv3-only /
   royalty-free / commercial), wxWidgets/Qt/GTK (application-singleton
   event loops in the host process — the exact in-host failure mode),
   FLTK (global state, multi-instance collisions, macOS reparenting),
   cycfi/Elements (not production ready). Pugl (ISC) is a windowing and
   event layer, not a widget toolkit — an ingredient, not a candidate.

## Decision Outcome

**Dear ImGui** was chosen as the widget layer of a new, small **basic
UI** embedded in the host's UI process. The existing per-format shims
(LV2 `lv2ui_descriptor` exporting `uiInstantiate`/`uiIdle`, CLAP
`clap_plugin_gui` — which VST3 inherits through clap-wrapper) become the
parent-handle + host-idle seam; no DPF adoption — Hydrogen keeps its
own shims, state codec, and IPC transport.

* Immediate mode fits the host-idle execution model (~30–60 Hz) and
  live meters/strips naturally. The LV2 no-singleton constraint holds
  by construction; the per-instance ImGui context discipline does not
  — contexts are thread-local and hosts idle multiple instances
  sequentially on one thread, so explicit context switching is an
  obligation, not a property.
* Backends: official Win32 and macOS parent-window backends; the X11
  child-window backend is adapted from DPF-Widgets (ISC), proven in the
  DISTRHO/Carla lineage on our primary platform.
* Vendored widgets: ImGuiKnobs (MIT) for rotary encoders;
  ImGuiFileDialog (MIT) for the file browser.
* Renderer: platform GL/Metal/DX via the official backends; a
  software-render fallback (imgui_sw) stays open as a de-risk question.

### The basic UI: three views

1. **Landing** — load song, pattern, and drumkit (file browser); spawn
   and connect an editor process (the existing `--connect-via-ipc`
   flow); MIDI configuration (ignore note-offs, instrument vs. kit
   mode). Configuration writes (MIDI options, instrument map) go
   through the [ADR 0022](0022-layered-plugin-configuration.md)
   plugin-instance layer and are never written back to
   `~/.hydrogen`.
2. **MIDI mapping** — the Instrument Mapping tab of
   `MidiControlDialog` as a plugin view: a settings grid plus
   per-instrument rows of channel/note spinboxes.
3. **Mixer** — one strip per instrument (volume fader, pan encoder,
   mute, solo, meter), plus drag&drop bezier curves connecting
   instruments to the plugin's exposed audio busses
   ([ADR 0019](0019-plugin-output-bus-layout.md)). The curve editor
   is 0019's deferred remapping widget and inherits its obligations:
   the custom mapping round-trips in the embedded song state
   ([ADR 0017](0017-embed-song-in-plugin-state.md)) under format
   versioning, and mappings are never shared automatically between
   instances.

### One authoritative engine, two UIs

The basic UI talks to the local engine instance directly; the editor
attaches via IPC as today
([ADR 0018](0018-plugin-editor-ipc-transport.md)). Both UIs observe
and drive the same engine state — which adds a third actor to 0018's
event classification:

* **Event and meter fan-out**: the `EngineSession` bridge thread stays
  the **sole drainer** of the per-instance `EventQueue` and the sole
  consumer of the per-instrument peaks (read+reset, for shm
  telemetry). It fans events out to both consumers — the editor via
  IPC telemetry as today, the basic UI through an in-process sink —
  and copies peaks for the local UI on its pass. The basic UI never
  pops the queue or resets peaks itself, so two attached UIs split
  neither events nor meters. The basic UI also needs the error
  replay visibility of
  [ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md).
* **Threading seam**: the idle callback runs on the host's main/UI
  thread; commands go through the `CoreActionController` surface under
  the engine lock (the
  [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md) threading
  contract).

### Spec compliance (LV2)

* The UI instantiates with a **real child window** (the widget is a
  native window/view id) — fixing the off-contract null widget.
* `ui:showInterface` is demoted to its spec role: the fallback for
  hosts that cannot embed. It shows the basic UI in a self-managed
  window — not the editor. That fallback window is created on first
  `show` and destroyed on `hide` or cleanup; the embedded child window
  and the fallback window are the two faces of one UI instance,
  selected by how the host drives it.
* `ui:idleInterface` drives the frame. `uiHide` no longer terminates
  the editor process, and neither does `uiCleanup` (both do today):
  the editor lifecycle is owned by the plugin instance, not the UI —
  destroying the UI (embedded or fallback) leaves a running editor
  alive. Teardown of the GL/ImGui contexts and the fallback window is
  the UI's own.

### CLAP

The gui-api gains the embedded path — `create(…, is_floating=false)` +
`set_parent` — replacing the floating-only support. `is_floating=true`
remains supported and shows the basic UI in a self-managed window
(mirroring the LV2 fallback), not the editor.

### De-risk before implementation

* X11 child-window GL context creation on arbitrary host parents
* Keyboard focus and IME routing per host and OS
* macOS Metal backend is self-described "not well tested" upstream
* DPI/scale handling across hosts
* clap-wrapper's translation of the CLAP embedded gui (`set_parent`)
  into VST3 `IPlugView` attach, including resize/DPI behavior

The LV2 UI shim currently has zero test coverage (`lv2_smoke.cpp`
never resolves `lv2ui_descriptor`); the new seam gets smoke coverage
from the start.

### Positive consequences

* The plugin is usable end-to-end without the editor, and visible in
  Ardour
* One small UI codebase across all three formats; no Qt Widgets and
  no `QApplication` event loop in the host process (the plugin keeps
  linking Qt Core/Xml/Gui/Network for the IPC transport,
  [ADR 0018](0018-plugin-editor-ipc-transport.md))
* MIT dependencies, tiny footprint, the most active maintenance in
  the field
* The off-contract LV2 UI declaration is fixed as part of the change

### Negative consequences

* Three new vendored dependencies (imgui, ImGuiKnobs, ImGuiFileDialog)
  and their third-party notices to maintain
* The X11 child-window backend is ours to own (adapted, not upstream)
* Immediate mode is a new paradigm for a Qt-background codebase
* The basic UI's user-facing strings need a translation story
  independent of `src/gui` (`CommonStrings.h` lives in the Qt Widgets
  application, which the plugin must not link). The Qt translation
  stack itself remains available — the plugin links Qt Gui, so
  `Q_DECLARE_TR_FUNCTIONS` and the existing `data/i18n` catalogs could
  serve a plugin-side string table; whether they do is an open
  implementation question.

## Pros and Cons of the Options

### Option 1 — Dear ImGui (chosen)

* `+` Immediate mode is the native fit for host-idle driving, live
  meters, and per-row input grids; built-in drag&drop and curve drawing
* `+` Tiniest footprint; MIT; the most active maintenance in the field
* `+` Linux/X11 path proven via the DPF-Widgets reference backend
* `−` Knobs, file browser, and faders come from vendored third parties
* `−` The X11 child-window backend is adapted code we own

### Option 2 — DPF/DGL + DPF-Widgets

* `+` The most battle-tested FOSS plugin-UI integration stack
  (LV2/CLAP/VST3 first-class); `ExternalWindow` documents exactly the
  embed / semi-external / standalone split
* `−` No file browser and no table/grid — the most assembly for the
  three views
* `−` Adopting DPF's plugin framework would rework our existing shims

### Option 3 — VSTGUI

* `+` The only candidate with knob, fader, and native file selector
  built in; retained mode familiar to Qt developers
* `−` Linux labelled "Preview"; LV2 attach needs an IRunLoop substitute
  — the risk sits on our primary platform
* `−` No spinbox; mapping grid, meters, and drag&drop are manual there
  too

### Option 4 — LVGL

* `+` Spinbox, file explorer, and sliders built in; pure software
  rendering (no GPU dependency)
* `−` C API in a C++ codebase; manual drag&drop and per-row widget
  grids; few audio-plugin precedents

### Option 5 — Nuklear

* `+` The tiniest of all; permissive licensing
* `−` No Cocoa/Metal backend, low upstream velocity, zero plugin
  precedents — pioneer tax

## More Information

* LV2 UI spec: <https://lv2plug.in/ns/extensions/ui>
* Dear ImGui: <https://github.com/ocornut/imgui>
* ImGuiKnobs: <https://github.com/altschuler/imgui-knobs>
* ImGuiFileDialog: <https://github.com/aiekick/ImGuiFileDialog>
* imgui_sw (software renderer): <https://github.com/emilk/imgui_sw>
* DPF-Widgets (X11 backend reference):
  <https://github.com/DISTRHO/DPF-Widgets>
* Related: [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md),
  [ADR 0014](0014-plugin-format-strategy.md),
  [ADR 0015](0015-per-instance-engine-context.md),
  [ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0017](0017-embed-song-in-plugin-state.md),
  [ADR 0018](0018-plugin-editor-ipc-transport.md),
  [ADR 0019](0019-plugin-output-bus-layout.md),
  [ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md),
  [ADR 0032](0032-h2player-gui-connection-mode.md)
