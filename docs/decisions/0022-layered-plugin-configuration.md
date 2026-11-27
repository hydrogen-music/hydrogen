---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Layered plugin configuration — overrides over the shared user config

## Context and Problem Statement

When run as a plugin, Hydrogen still has a `Preferences` object per instance
([ADR 0015](0015-per-instance-engine-context.md)). An earlier draft of
[ADR 0016](0016-out-of-process-plugin-ui.md) said the plugin/editor should simply
**not** read or write the user's `~/.hydrogen` config. That is wrong: most of
`Preferences` governs *general* behaviour — theme/colours/fonts
(`m_pTheme`), keyboard shortcuts (`m_pShortcuts`), language
(`m_sPreferredLanguage`), UI layout, window properties, MIDI action mappings.
Discarding all of it would mean a user who runs Hydrogen both standalone and as a
plugin gets two divergent, inconsistent setups — poor UX.

Yet some `Preferences` fields make no sense, or are actively harmful, inside a
plugin: the host owns audio/MIDI I/O, and the "current song" is the plugin state,
not a file the user opened. Writing those back to the shared config would clobber
the user's standalone setup and let multiple instances race on one file.

We need a rule for which settings come from where.

## Decision Drivers

* Consistent UX across standalone and plugin (shared look, shortcuts, language).
* Host owns audio/MIDI I/O — those settings cannot come from the user config.
* The plugin's song is its state, not a recent-file path.
* Multiple instances in one process must not race on, or pollute, the shared
  `~/.hydrogen` config.

## Considered Options

1. **Layered config**: shared user config as a read base, with a plugin-specific
   subset overridden from the host engine and the plugin state.
2. Ignore the user config entirely in plugin mode (the earlier draft).
3. Use a fully separate plugin config file.

## Decision Outcome

Chosen option: **layered config (option 1).**

Each plugin instance's `Preferences` is composed as **base ← override**:

* **Base layer** — loaded from the shared `~/.hydrogen` config, giving the plugin
  the same general behaviour as the standalone app: **theme/colours/fonts
  (`m_pTheme`), shortcuts (`m_pShortcuts`), preferred language
  (`m_sPreferredLanguage`), UI layout, window properties, and MIDI action
  mappings.**
* **Override layer** — a plugin-specific subset that **supersedes** the base,
  sourced from:
  * the **host engine** — `m_nSampleRate`, `m_nBufferSize`, and the fact that
    audio/MIDI I/O is host-driven (so `m_audioDriver`, `m_midiDriver`,
    PortAudio/CoreAudio device fields, and JACK options `m_bJackTrackOuts` /
    `m_bJackTimebaseEnabled` / `m_bJackTimebaseMode` are forced to the plugin's
    own values and the corresponding Preferences UI is hidden/disabled);
  * the **plugin state** — recent/last-file state (`m_recentFiles`,
    `m_sLastSongPath`, `m_sLastPlaylistPath`) and per-instance settings such as
    the output-bus mapping ([ADR 0019](0019-plugin-output-bus-layout.md)); OSC
    (`m_bOscServerEnabled` / `m_nOscServerPort`) is per-instance, not taken from
    the shared config.

**Persistence rule:** base-layer changes made in the plugin **are persisted** to
the shared `~/.hydrogen` config — running as a plugin must not silently lose the
user's theme/shortcut/language changes. The override subset is **never written
back** to the shared config; it lives in the host-provided runtime values and the
plugin state. Because hosts tear instances down in parallel, the shared-config
write must be concurrency-safe (atomic, locked, field-level merge — **not** a
full-file snapshot on close); that mechanism is specified in
[ADR 0023](0023-concurrency-safe-config-persistence.md).

Ignoring the user config (option 2) is rejected for the UX split described above.
A fully separate plugin config (option 3) is rejected because it would *also*
duplicate and diverge the general settings we explicitly want shared.

### Consequences

* `Preferences` (now per-instance) gains a notion of an override layer: a defined
  set of fields whose values come from the host/state rather than the file, and
  which are excluded from any save to the shared config.
* The Preferences UI in editor mode hides/disables the overridden, host-owned
  controls (audio/MIDI driver, buffer size, sample rate, JACK options) so the
  user is not offered settings the host dictates.
* The shared `~/.hydrogen` is read by the base layer and base-layer changes are
  written back through the concurrency-safe path of
  [ADR 0023](0023-concurrency-safe-config-persistence.md), so parallel instances
  persist their changes without corrupting the file.
* The exact field membership of each layer must be enumerated in code and kept in
  sync as `Preferences` evolves (a single source-of-truth list).

## More Information

* "`~/.hydrogen`" above is shorthand for the **shared user config**, whose actual
  location is abstracted by `Filesystem::userConfigPath()`: XDG-derived on Linux
  (`$XDG_CONFIG_HOME/hydrogen/hydrogen.conf`, default
  `~/.config/hydrogen/hydrogen.conf`) and platform-specific on macOS/Windows;
  `~/.hydrogen` is only the legacy fallback. The base layer reads, and base-layer
  changes are written to, whatever path `userConfigPath()` resolves to.
* `src/core/Preferences/Preferences.h` — field definitions referenced above.
* Related: [ADR 0015](0015-per-instance-engine-context.md),
  [ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0019](0019-plugin-output-bus-layout.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
