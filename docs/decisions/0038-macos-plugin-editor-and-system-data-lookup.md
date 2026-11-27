---
status: accepted
date: 2026-11-20
deciders: pm
---

# AD: Resolve the editor and the system data folder via the installed app on macOS

## Context and Problem Statement

A plugin host passes no command line options, so a plugin instance has to find
two things on its own (ADR 0013, ADR 0016):

1. the **editor binary** it launches in a separate process
   (`HydrogenPlugin::resolveEditorBinary`), and
2. the shared **system data folder** — shipped drumkits, demo songs, the
   default `hydrogen.conf`, translations (`HydrogenPlugin::resolveSystemDataPath`
   feeding `Filesystem::bootstrap`).

On Linux both have a fixed system location (`<prefix>/bin/hydrogen`,
`<prefix>/share/hydrogen/data`), and the plugin finds the editor by walking up
from its own install location. On macOS neither works:

* Hosts load plugins from `~/Library/Audio/Plug-Ins/{CLAP,VST3,LV2}` (or
  `/Library/...`). The editor lives in an app bundle somewhere else, typically
  `/Applications/Hydrogen.app`, so the walk-up never reaches it and the plugin
  falls back to `hydrogen` on `PATH`, which apps started from Finder do not
  have.
* The existing macOS candidates cannot match anyway: they look for
  `Hydrogen.app/Contents/MacOS/Hydrogen` while the executable is named
  `hydrogen` (it only works on case-insensitive file systems), and the install
  layout places the app at `<prefix>/bin/hydrogen.app`, which no candidate
  covers.
* `Filesystem::bootstrap` derives the macOS system data folder from
  `QCoreApplication::applicationDirPath()`. Inside a plugin that is the
  *host's* bundle, not Hydrogen's, so the shipped data is never found unless
  `$HYDROGEN_SYS_DATA_PATH` is set — something a user cannot reasonably do for
  a DAW started from the Dock.

On macOS the shipped data does not live in a shared system folder at all: the
app bundle is self-contained, and `Hydrogen.app/Contents/Resources/data` is
the one place it exists.

## Decision Drivers

* Shared data must exist in exactly one place; the plugin never searches
  relative to its own install location (ADR 0013, `src/plugin/README.md`).
* The editor runs in its own process, so its Qt never meets the host's Qt.
  It can — and should — be the regular, dynamically linked application
  (DEVELOPERS.md: the `hydrogen` binary must not be built statically).
* Plugin and editor must run the same Hydrogen version (version handshake,
  `EditorVersionMismatch`).
* Resolution must stay unit-testable without touching the real
  `/Applications` or LaunchServices database.
* macOS-specific code is kept behind `#ifdef __APPLE__`; other platforms keep
  their current behaviour.
* The system data folder only holds the shipped *starting point*. Most
  drumkits, songs and patterns are downloaded or created by the user and live
  in the user data folder. Not finding the system folder must therefore never
  stop the plugin from running on valid user-level data.

## Considered Options

1. **Anchor both lookups on the installed `Hydrogen.app`** — locate the app
   bundle, launch its executable as the editor and use its
   `Contents/Resources/data` as system data folder.
2. **Ship the data inside every plugin bundle.** Three copies (CLAP, VST3,
   LV2) violate the single-location rule; the macOS `.clap` is currently a
   flat Mach-O file without a bundle to carry resources.
3. **A shared `/Library/Application Support/Hydrogen/data`.** Clean in
   principle, but it changes how the app is distributed (today a
   self-contained bundle installed by drag-and-drop) and requires an admin
   installer (`.pkg`).
4. **Require `$HYDROGEN_EDITOR_PATH` / `$HYDROGEN_SYS_DATA_PATH`** via
   `launchctl setenv`. Status quo workaround; not something to ask users for.

## Decision Outcome

Chosen option: **1, anchor both lookups on the installed app bundle**, as
the only option that keeps the shipped data in one place without changing the
app's packaging.

### Editor resolution (`resolveEditorBinary`)

Precedence (unchanged first steps, new macOS step before the `PATH`
fallback):

1. explicit override (persisted via `setEditorBinary`, ADR 0022),
2. `$HYDROGEN_EDITOR_PATH`,
3. walk up from the plugin's install location (`setEditorSearchDir`). On
   macOS this also checks `hydrogen.app`, `Hydrogen.app` and
   `bin/hydrogen.app` at every level, so the `<prefix>/bin/hydrogen.app`
   install layout is found,
4. **macOS only:** the installed application bundles — `/Applications`,
   `~/Applications` and every copy LaunchServices knows for the bundle
   identifier `org.hydrogenmusic.Hydrogen`. A bundle whose
   `CFBundleShortVersionString` equals the plugin's version is preferred over
   one that does not, so a stale release next to a development build is not
   picked by accident; the version handshake remains the hard guarantee,
5. `hydrogen` on `PATH`.

The executable inside a bundle is read from its `Info.plist`
(`CFBundleExecutable`) instead of being hard-coded. The installed-bundle list
is injectable, so tests can feed fixture bundles instead of the real system.

### System data resolution (`resolveSystemDataPath`)

1. `$HYDROGEN_SYS_DATA_PATH`,
2. **macOS only:** `Contents/Resources/data` of the app bundle containing the
   resolved editor binary, if readable,
3. empty — the `Filesystem` platform default (unchanged on Linux/Windows).

Tying the shipped data to the editor bundle is by design: Hydrogen is expected
to seamlessly handle shipped resources of other versions anyway, and the shipped
data is only the starting point and fallback; user content lives in the user
data folder.

### First boot and an unresolvable system data folder

* If the system data folder cannot be resolved, the plugin logs a warning and
  keeps running on the user data folder. Missing shipped data is never a
  reason for the plugin to fail instantiation or exit.
* On first boot (no user data folder yet) the plugin creates the same user
  folder tree and the user-level `hydrogen.conf` the editor creates, through
  the regular locked, merged `Preferences::save()` path (ADR 0023). Without
  this the shared config only appeared once a standalone editor had run.

### Bundle identifier

The app's `CFBundleIdentifier` changes from the generic `Hydrogen` to
`org.hydrogenmusic.Hydrogen`, matching the Linux desktop-file ID. A unique
identifier is required for the LaunchServices lookup. Hydrogen stores no
state keyed by the bundle identifier (no `QSettings`/`NSUserDefaults`), so
the change does not lose user settings. macOS privacy permissions (TCC, e.g.
microphone access) are keyed by it, though, and are requested once more.

### Consequences

* Good: a plugin installed in the standard macOS folders finds the editor and
  the shipped data without any environment setup.
* Good: the regular app doubles as the plugin editor; no separate editor
  install is needed.
* Good: first boot of the plugin leaves the same user-level state as first
  boot of the editor.
* Neutral: the editor override chosen in the plugin UI is stored in the user
  config, which is read after `Filesystem::bootstrap`. The system data folder
  is therefore derived from the environment or the installed app bundles, not
  from that override. `Filesystem` is bootstrapped once per process, so a
  changed override takes effect for the data folder only in a new host
  process.
* Neutral: without an installed `Hydrogen.app` (and no environment override)
  the plugin runs on user data only; the shipped kits and the default config
  are unavailable until the app is installed.
* Bad: the plugin now depends on LaunchServices (CoreServices) on macOS.

## Validation

* `PluginLifecycleTest` (unit tests, fixture bundles in a temporary
  directory): the `<prefix>/bin/hydrogen.app` layout, the injected installed
  bundle list incl. `CFBundleExecutable` and version preference, and the
  derivation of `Contents/Resources/data` from the editor bundle — macOS only.
* `PluginFirstBoot*` CTests: the plugin core is instantiated in a fresh
  process with an empty `HOME`, with and without a resolvable system data
  folder, and with user-level data but no system data. Each must run, and
  first boot must create the user folder tree and `hydrogen.conf`.

## More Information

* [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md),
  [ADR 0016](0016-out-of-process-plugin-ui.md),
  [ADR 0022](0022-layered-plugin-configuration.md),
  [ADR 0023](0023-concurrency-safe-config-persistence.md)
