---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Run the plugin editor as an out-of-process UI

## Context and Problem Statement

We want the plugin to expose the **full existing Qt editor** (mixer, pattern
editor, song editor, drumkit dialogs — `src/gui/`, ~77k LOC), not a reduced
plugin panel. At the same time, multiple instances must run in one host process
([ADR 0015](0015-per-instance-engine-context.md)).

This collides with a hard Qt constraint: **`QApplication` is itself a process
singleton** with a single event loop. Embedding the Qt GUI *in-process* means all
plugin instances — and the host — share one `QApplication` and one event loop,
which the host also wants to own. That path is fragile and host-dependent, and it
compounds the de-singletoning work.

## Decision Drivers

* Reuse the entire existing Qt GUI rather than reimplementing it.
* Robust multi-instance behaviour without `QApplication`/event-loop conflicts.
* Isolate Qt (and any GUI crash) from the host process and the real-time audio
  thread.
* Align with the formats we ship: LV2's native UI model is *already*
  out-of-process / separate-binary ([ADR 0014](0014-plugin-format-strategy.md)).

## Considered Options

1. **Out-of-process UI** — the editor is a separate executable, one per
   instance, communicating with the in-host engine over IPC.
2. **In-process, shared `QApplication`** — editor widgets parented into the
   host-provided window, integrated with the host event loop.
3. **Per-format split** — in-process for CLAP/VST3, out-of-process for LV2.

## Decision Outcome

Chosen option: **out-of-process UI.**

The plugin binary (loaded by the host) contains only the engine and a thin
controller. When the user opens the editor, the plugin **launches a separate
editor process** that runs its own `QApplication` and the existing Hydrogen GUI,
connected to the engine instance via an IPC channel. Each plugin instance gets
its own editor process.

**The editor is not a separate codebase or binary — it is the existing
`hydrogen` GUI started in an editor mode** (`hydrogen --plugin-editor <endpoint>`).
The standalone startup (`src/gui/src/main.cpp`) creates an engine via
`Hydrogen::create_instance()` and opens a hardware audio driver, then builds
`MainForm`; editor mode **skips engine and driver creation** and instead attaches
an **IPC-backed access object** to the engine living in the host process, then
builds the *same* `MainForm`. This is a direct consequence of the de-singletoning
([ADR 0015](0015-per-instance-engine-context.md)): once the GUI reaches state
through an injected handle rather than `get_instance()`, that handle can be either
a **local `Hydrogen`** (standalone) or an **IPC proxy implementing the same
interface** (editor mode). One binary serves both roles.

This sidesteps the `QApplication` singleton entirely (each editor process has its
own), gives true multi-instance UI, isolates GUI crashes from the host and from
audio, and matches LV2's native UI model — so the LV2 and CLAP/VST3 builds share
one UI architecture rather than two.

In-process shared `QApplication` is rejected: multi-instance Qt in a host-owned
event loop is the single most fragile, host-specific failure mode in the whole
port. The per-format split is rejected as two UI integration paths to build and
maintain.

### Consequences

* A new **engine↔UI IPC layer** must be built:
  * the UI must observe engine state changes — `EventQueue`
    (`src/core/EventQueue.h`) becomes the natural thing to marshal across the
    IPC boundary instead of being read in-process;
  * UI commands (transport, edits, kit changes) must be serialised to the
    engine, ideally routed through the existing
    `CoreActionController`-style command surface.
* Editor-process lifecycle management: launch on "open editor", reconnect or
  respawn on crash, tear down with the instance, and degrade gracefully (engine
  keeps making sound) if the editor is closed or dies.
* Some latency between UI action and audible/visible effect, versus a direct
  in-process call. Acceptable for an editor UI.
* The GUI's ~1,292 `get_instance()` sites still need the per-instance refactor
  ([ADR 0015](0015-per-instance-engine-context.md)), but now resolve against the
  editor process's own single engine handle (local or IPC proxy) rather than a
  true global.
* **Per-instance services are also per-process across the split.**
  `SoundLibraryDatabase` is per-instance ([ADR 0015](0015-per-instance-engine-context.md)),
  so the editor process and the engine process each have their own. The practical
  model is a per-process, disk-backed copy: the editor manages library installs
  (e.g. online import) and then sends the engine a **"rescan sound library" IPC
  command** so it can load newly installed kits/songs. Loading a specific kit/song
  stays a normal `CoreActionController` command. Editor-side helpers that are not
  engine concerns — notably `OnlineImporter` (network + file install; also used
  headless by the CLI) — run entirely in the editor process and only touch the
  engine through that single rescan command. (The per-instance `Logger` likewise
  means the editor process logs to its own file, separate from the engine
  instance's.)
* Packaging ships **one binary** (`hydrogen`) per platform that serves both
  standalone and editor roles — a single Qt deployment, no GUI divergence. The
  cost is that this binary still links the engine and all hardware drivers that
  editor mode never uses (modest dead weight; a slimmer `WANT`-gated build is
  possible later).
* Editor mode shares the user's general config (theme, shortcuts, language, UI
  layout) for a consistent UX, but a plugin-specific subset (audio/MIDI I/O,
  JACK/OSC options, recent/last-file state) is overridden from the host engine and
  the plugin state and is never written back to `~/.hydrogen`. See
  [ADR 0022](0022-layered-plugin-configuration.md) for the layering rule.

## More Information

* IPC design and lifecycle details:
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
* LV2 UI model: <https://lv2plug.in/ns/extensions/ui>
* Related: [ADR 0014](0014-plugin-format-strategy.md),
  [ADR 0015](0015-per-instance-engine-context.md)

## Addition 2026-07-23

* The CLI option `--plugin-editor` was renamed to `--connect-via-ipc`.
* The unit test `EditorModeTest.cpp` was renamed to `ConnectViaIpcModeTest.cpp`.
