---
status: accepted
date: 2026-10-27
deciders: pm
---

# AD: Plugin state carries the base-layer preferences slice

## Context and Problem Statement

[ADR 0022](0022-layered-plugin-configuration.md) composes each plugin
instance's `Preferences` as base ← override: the base layer comes from the
shared user config, the override layer from the host engine and "the plugin
state". But the plugin state itself carried no preferences at all: it was
just the song ([ADR 0017](0017-embed-song-in-plugin-state.md),
[ADR 0020](0020-plugin-state-sample-embedding.md)), latterly wrapped in the
`.h2project` archive ([ADR 0025](0025-h2project-self-contained-format.md)).

So every base-layer value an instance diverged on was session-local. The MIDI
instrument map (`m_pMidiInstrumentMap`) and the last-used directories the
plugin's own file browsers remember (`m_sLastOpenSongDirectory` & friends)
are base-layer fields, yet per-instance in nature: two plugin instances can
carry completely different maps, and a reopened host project should find each
of them as it was left — not flattened to whatever the shared user config
last held.

We need those values to travel with the plugin state, without violating the
layering of ADR 0022: the host-owned override layer must stay out, and the
restore must not fight the host.

## Decision Drivers

* A reopened host project must find the plugin as it was left (maps, browser
  memory) — not reset to the shared config.
* Per-instance base-layer state must be remembered per project, not lost at
  session end.
* The host owns audio/MIDI I/O (ADR 0022): nothing about the restore may
  restart or reconfigure drivers.
* Old hosts/projects (bare-XML state, archives without the entry) must keep
  loading.

## Considered Options

1. Serialize the instance's **base-layer slice** into the state blob and
   overlay it on load.
2. Serialize **all** preferences (base + override) into the state blob.
3. Keep the state preferences-free (status quo) and rely on the shared user
   config only.

## Decision Outcome

Chosen option: **base-layer slice (option 1).**

* **Container**: the state blob is always the `.h2project` archive container
  of [ADR 0025](0025-h2project-self-contained-format.md); the embed toggle of
  ADR 0020 affects only the `samples/*` entries and the manifest. A bare-XML
  blob is legacy input, read-only. The state is thus self-describing
  (`looksLikeArchive`) regardless of the embed setting.
* **Entry**: a `preferences.xml` entry rides in the archive, written by the
  new `Preferences::toXmlBuffer()` — the same schema rows as the user config,
  filtered by the instance's `FieldOwnership` (`rowEligible`), i.e. exactly
  the base layer for a plugin instance. The override layer (host-owned sample
  rate, buffer size, drivers, device subtrees, JACK/OSC, last-song/playlist
  pointers) never enters the blob.
* **Restore**: `H2Project::fromState()` reports the entry via an out-param;
  `HydrogenPlugin::loadState()` applies it with
  `Preferences::applyFromXmlBuffer()` — an in-memory overlay onto the
  instance's current `Preferences`, followed only by light side effects
  (metronome volume, a gated `UpdatePreferences` event). It deliberately does
  **not** call `setPreferences()`: that path restarts audio/MIDI drivers,
  which the host owns. The restore itself never writes to the shared user
  config; base-layer changes the user makes afterwards persist per ADR 0022.
* **Failure semantics**: a song-parse failure fails the load with preferences
  untouched; the preferences entry is applied before the song is set, so
  anything in the song setup that consults preferences (e.g. the MIDI
  instrument map) sees the restored values; an absent entry (state from an
  older build) keeps the current preferences.

Option 2 is rejected: the override layer is host-owned; storing it would
desync from the host on reload. Option 3 is rejected: it is the status quo
that loses per-instance state.

### Consequences

* `PreferencesSchema::writeRows()`/`readRows()` gained an optional
  `FieldOwnership` filter next to the owner filter, so the slice reuses the
  single-source-of-truth schema table (ADR 0023) — no second key list to
  drift.
* `H2Project::toState()`/`fromState()` gained the preferences argument /
  out-param; the song-only call forms are unchanged.
* The restore overlays *all* base rows, so the project's remembered value
  supersedes the shared config for that instance — the project is the more
  specific source. A later user-initiated save from the plugin then persists
  base rows per ADR 0022, including restored values.
* Round-trip coverage: `PluginLifecycleTest::
  testPluginStatePreferencesRoundTrip` (end-to-end, including override pins
  and legacy bare-XML blobs), `PluginStateTest` (codec-level entry pins),
  and `PreferencesSchemaTest::testOwnershipFilteredRoundTrip` (row-by-row
  layer oracle via the `tieAllMembers` index correspondence).

## More Information

* [ADR 0017](0017-embed-song-in-plugin-state.md),
  [ADR 0020](0020-plugin-state-sample-embedding.md),
  [ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0023](0023-concurrency-safe-config-persistence.md),
  [ADR 0025](0025-h2project-self-contained-format.md)
* `src/core/Preferences/Preferences.h` (`toXmlBuffer`/`applyFromXmlBuffer`,
  `FieldOwnership`), `src/core/Helpers/H2Project.h`,
  `src/plugin/HydrogenPlugin.cpp`
* [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
