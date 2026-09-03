---
status: accepted
date: 2026-06-09
deciders: pm
---

# AD: Introduce the `.h2project` self-contained project format (song + samples)

## Context and Problem Statement

[ADR 0017](0017-embed-song-in-plugin-state.md) decided the plugin state should be
self-contained (song + sample audio), and [ADR 0020](0020-plugin-state-sample-embedding.md)
specified the mechanism (a libarchive bundle decoded in memory). Two observations
push this beyond a plugin-internal concern:

* A **self-contained, portable project file has been requested repeatedly for the
  standalone app** — today `.h2song` stores the song plus the drumkit *definition*
  but only **references** samples by path, so moving a project to another machine
  breaks unless the kit is installed.
* Drumkits can be **absurdly large** (SFZ kits exceeding 1 GB). Forcing *every*
  project file — or every DAW project's plugin state — to embed the full kit is
  overkill and would bloat saves massively.

So self-containment must be **available but not mandatory**, and the bundling
should be a real, named artifact shared by both standalone and plugin rather than
an opaque plugin-only blob.

## Decision Drivers

* Serve the long-standing standalone request for portable projects.
* Don't force gigabyte kits into files that don't need them.
* One mechanism/codec for standalone and plugin — no divergence.
* A single, simple open path for users (don't make them pick a loader).

## Considered Options

1. **A named `.h2project` bundle format** (song + samples), used by both the
   standalone app (menu) and the plugin (state), with a single unified open path
   that accepts `.h2song` or `.h2project`.
2. Keep self-containment as a plugin-only opaque state blob (status quo of
   ADR 0017/0020).
3. Always embed everywhere (no opt-out).

## Decision Outcome

Chosen option: **a named `.h2project` format (option 1).**

* **`.h2project`** is a libarchive bundle — the song XML (with its inline
  `<drumkit_info>`) plus the kit's **actual sample data** and image — built with
  the mechanism of [ADR 0020](0020-plugin-state-sample-embedding.md) (content-hash
  dedup; loaded via in-memory libsndfile virtual I/O). It coexists with
  `.h2song`, which stays song + kit *definition* + sample **paths** (kit must be
  installed). In short: `.h2project` = `.h2song` content **+** the sample bytes.
* **Standalone app:** a new menu action **creates a `.h2project`** (export /
  "save as self-contained project"). Classic Save stays `.h2song`. This delivers
  the portable-project feature independently of the plugin.
* **Plugin:** a **preference toggle "store drumkit samples in plugin state"
  (ON by default).** ON → the state is a full `.h2project` bundle (portable across
  machines). OFF → the state is **song-only** (the `.h2song` equivalent,
  referencing the installed kit) — the escape hatch for projects built on
  gigabyte kits, where the user accepts that the kit must be installed.
* **Unified open:** a **single open API endpoint / menu action loads either
  `.h2song` or `.h2project`**, detected by container type (XML document vs
  archive). The plugin's state reader likewise accepts both song-only and
  full-bundle states. Users and hosts never choose a loader.

Option 2 wastes a mechanism that standalone users have asked for and keeps state
opaque. Option 3 is rejected precisely because of the >1 GB-kit case.

### Consequences

* One codec serves three uses: standalone `.h2project` export, plugin state
  embedding (toggle on), and the reader for both — no second implementation.
* **Portability is a property of the chosen container, not the app:**
  `.h2project` (and embedded plugin state) is portable; `.h2song` and song-only
  plugin state require the drumkit to be installed. This is now an explicit,
  user-visible choice.
* New extension to register across file dialogs / MIME / filters; the *archived*
  song keeps its existing `formatVersion`
  ([ADR 0001](0001-introduce-formatVersion-to-xml-files.md)).
* The plugin default (embed on) preserves the portability intent of
  [ADR 0017](0017-embed-song-in-plugin-state.md); the opt-out is the new part.
* `.h2project` can itself be large by design when the user bundles a big kit —
  acceptable because it is an explicit, opt-in artifact, unlike forcing it into
  every save.

## More Information

* `src/core/Basics/Song.h` (`.h2song` save/load), `Drumkit` libarchive code
  (`Drumkit.cpp:737`), `src/core/Helpers/Filesystem.*` (extension/paths).
* Related: [ADR 0017](0017-embed-song-in-plugin-state.md),
  [ADR 0020](0020-plugin-state-sample-embedding.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md),
  [proposal 0004](/docs/proposals/0004-plugin-port-implementation-plan.md)

## Amendment (2026-09-02): extraction-based reconstruction

The original decision had the reconstruction decode samples **in memory**
(libsndfile virtual I/O, `Sample::loadFromMemory()`). This proved too
limiting in practice:

* Reconstructed samples had **no file path**, so everything relying on one
  was broken: unloading/reloading samples, and re-saving a loaded project
  (the re-saved bundle lost its sample data and wrote empty `<filename>`
  elements).
* The bundle's XML was **not reproducible**: `<filename>` elements embedded
  local drumkit or cache paths, so the same project saved twice - or on two
  machines - did not compare equal.

What changed:

* **Reconstruction extracts the bundle** to a per-origin folder below
  `Filesystem::cacheDir()` (`projects/<key>/`) via the new shared
  `H2Core::Archive` back end, which also replaces the extraction code
  formerly duplicated in `Drumkit::install()` and the unit test assertions.
  The extracted sample files are assigned with `Sample::setFilePath()` and
  decoded through the regular, synchronous file-based code path.
* **Cache keys:** a `.h2project` file is keyed by
  `<sanitized basename>-<sha1 of its absolute path>`; a plugin state by
  `plugin-<PID>-<Hydrogen instance id>`. Re-loading the same origin replaces
  its folder wholesale.
* **Lifecycle:** extraction folders are registered with the owning
  `Hydrogen` instance (`Hydrogen::registerExtractedProjectDir()`) and
  removed again in `~Hydrogen()` - no stale cache accumulates.
* **Deterministic XML:** while building a bundle, a path→entry map
  (`Xml::ProjectSampleEntries`) is threaded through the whole `saveTo()`
  chain so that `<filename>` elements reference manifest entry names
  (`samples/<sha1>.<ext>`) instead of local paths. A loaded project re-saves
  with identical content, and no cache or drumkit path leaks into the
  portable bundle.

Everything else - the container itself, the content-hash dedup, the
standalone menu action, the plugin toggle, and the unified open path - is
unchanged.
