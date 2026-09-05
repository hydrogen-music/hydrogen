---
status: accepted
date: 2026-06-08
amended: 2026-09-06
deciders: pm
---

# AD: Concurrency-safe persistence of the shared user config

## Context and Problem Statement

User changes to the general configuration (theme, shortcuts, language, layout)
**must** persist when Hydrogen runs as a plugin — anything less is a poor,
surprising UX ([ADR 0022](0022-layered-plugin-configuration.md)). But Hydrogen's
current persistence model makes that unsafe under multiple instances:

* `Preferences::save()` writes the **entire** config as one XML snapshot
  (`Preferences.cpp:1405` → `saveTo( Filesystem::userConfigPath() )`).
* It is called **on close** (`src/gui/src/main.cpp:534`,
  `src/gui/src/MainForm.cpp:2758`).

A DAW typically tears down all plugin instances **in parallel** on project/host
close. With per-instance `Preferences` ([ADR 0015](0015-per-instance-engine-context.md)),
that means several processes each rewriting the whole shared user config file at
the same moment: a last-writer-wins race that corrupts the file or silently
discards other instances' (and the standalone app's) changes. The same clobbering
can happen between a plugin instance and a standalone instance running
concurrently.

## Decision Drivers

* Config changes made in the plugin **must** be persisted.
* Parallel teardown of many instances must not corrupt or lose changes.
* Must also be safe against a concurrently running standalone instance.
* Avoid a fragile cross-process "single writer/coordinator" that the host may not
  keep alive.

## Considered Options

1. **Atomic, locked, field-level 3-way merge, written incrementally** (not a
   full snapshot on close).
2. Keep full-snapshot writes but serialize them with a lock (last-writer-wins on
   the whole file).
3. Per-instance separate config files (no sharing).
4. A single in-process config-owner that serializes writes.

## Decision Outcome

Chosen option: **atomic, locked, field-level 3-way merge, written incrementally
(option 1).**

Each per-instance `Preferences` keeps the **baseline** it loaded — the on-disk
state at load time. Persistence then works per *base-layer* field
([ADR 0022](0022-layered-plugin-configuration.md)), never as a whole-file dump:

* **Trigger** — when a base-layer field changes, schedule a *debounced*
  write-through during the session, rather than accumulating everything for a
  synchronized dump at teardown. On teardown only pending changes are flushed.
* **Persist procedure (atomic + locked merge):**
  1. acquire an **exclusive cross-process lock** on the config file
     (`QLockFile` / OS advisory lock);
  2. **re-read** the current on-disk config;
  3. for each base-layer field where *current-in-memory ≠ our baseline* (i.e. the
     user changed it this session), apply our value; **leave every other field as
     the freshly-read disk state** so concurrent edits from other instances
     survive;
  4. write **atomically** (`QSaveFile` / temp file + `rename`);
  5. update our baseline to the merged result; release the lock.
* The **override subset** (audio/MIDI I/O, JACK/OSC, recent/last-file state) is
  **excluded** from this write entirely — it lives in plugin state / host runtime
  values, not the shared config ([ADR 0022](0022-layered-plugin-configuration.md)).

Because writes are small, serialized on the lock, and merge rather than overwrite,
**parallel teardown is safe**: unrelated fields are never clobbered. If two
instances changed the *same* field, last-writer-of-that-field wins — an accepted,
bounded outcome, not file corruption.

This atomic-write-under-lock path is adopted for **any** context that writes the
shared config once plugin support exists (so a standalone instance cannot clobber
a plugin instance via the old snapshot path); the field-level 3-way merge and
write-through are the behaviour whenever `Preferences` is per-instance.

Rejected: option 2 (locked full snapshot) still loses other instances' changes
(whole-file last-writer-wins). Option 3 abandons the shared-UX goal of
[ADR 0022](0022-layered-plugin-configuration.md). Option 4 is fragile in a plugin —
there is no guaranteed long-lived coordinator process across hosts, and it does
not protect against a concurrent standalone instance.

### Consequences

* `Preferences` gains: a retained load baseline, a debounced write-through
  scheduler, and a merge-on-write path built on `QLockFile` + `QSaveFile`
  (replacing the unconditional full-snapshot `saveTo` on the shared-config path).
* **Change tracking is by diff-against-baseline, not a dirty-set.** "Fields the
  user changed" = base-layer fields where current ≠ the retained load baseline,
  compared at persist time. This needs **no setter instrumentation** (so no field
  can be silently missed), costs nothing (the config is tiny and persist is
  debounced), and *is* exactly the merge's input (apply the differing fields onto
  the freshly-read disk state). A dirty-set (flag in every setter) is rejected as
  invasive and fragile.
* The single source-of-truth field lists from
  [ADR 0022](0022-layered-plugin-configuration.md) (base vs override) drive both
  what is merged and what is excluded.
* On teardown the plugin/editor flushes pending changes via the locked merge; it
  must **not** call the legacy full-snapshot `save()`.
* Slightly more I/O during a session (debounced, small) instead of one large write
  at close — an acceptable trade for correctness under parallel teardown.

## Amendment (2026-09-06): all writers merge; ownership masks; schema-driven fields

Since acceptance, a second use case appeared: running Hydrogen
**headless plus an editor GUI concurrently** (development/debug workflow,
[ADR 0016](0016-out-of-process-plugin-ui.md),
[ADR 0030](0030-coreactioncontroller-over-ipc.md)) — two processes, both
saving the shared config. This amendment generalizes the decision; the text
above is kept as the accepted baseline.

1. **Scope — every writer of the shared config merges.** The locked
   read-merge-write is the *only* write path for the shared user config, in
   every context (standalone GUI, headless engine, editor GUI, plugin
   instances). An explicit whole-document write remains available only for
   copies/exports (`saveCopyAs`), never for the shared config itself.
2. **Field ownership masks replace the blanket override exclusion.** *Which*
   fields an instance may write is a property of the instance, not of the
   write path:
   * **All** — standalone and headless-engine instances own every field
     (their driver/session settings must persist).
   * **Base layer only** — instances under a plugin host (ADR 0022
     unchanged: host/state-owned fields are never written).
   * **GUI-owned only** — an editor mirror writes only GUI fields
     (owner = Gui ∧ layer = Base); everything else arrives from the engine
     via IPC and is read-only for persistence.
   Within its mask, an instance still writes only fields where
   current ≠ baseline.
3. **One schema drives everything.** The field lists of ADR 0022 (base vs
   override) and the IPC core-props subset live as two columns (`Layer`,
   `Owner`) of a single schema table that also drives load, save, merge, and
   diff. `PluginConfig` (parallel path lists plus a schema-blind generic XML
   merge) is dissolved into `Preferences`. The retained baseline is the raw
   XML bytes as loaded: immune to the in-place mutation of shared
   sub-objects (the GUI mutates e.g. the shortcuts map inside the same
   `shared_ptr` the baseline would hold) and independent of deep-copy
   support in the composite classes.
4. **Lock and failure strategy.** The cross-process lock is acquired with a
   bounded timeout and retried — contention may delay a save, never silently
   drop it. Unparseable on-disk XML is an error log plus a self-healing
   snapshot write, never a silent success. The merge creates missing
   elements (schema drift/upgrades) and replaces list/composite containers
   wholesale (covering both creation and deletion).
5. **Explicitly deferred:** the debounced write-through scheduler (existing
   triggers — dialog OK, NSM/OSC, teardown — already bound the crash-loss
   window) and in-process serialization of concurrent `save()` calls
   (current callers save single-threaded per process).

### Enforcement consequence

Adding a `Preferences` member without integrating it into the schema is a
**build failure**: serialized members live in a plain `PreferencesData`
aggregate; an exhaustive structured-binding tie over that aggregate must name
every member (C++17); and a `static_assert` ties the tie's size to the schema
row count. Unknown elements in a config file are reported at load time.

### Implementation notes (2026-09-06, Phase 2)

* **Diff semantics.** A row counts as changed when its serialized footprint
  (compact re-write from the current state) differs from the baseline's
  element(s). Both sides are normalized by stripping whitespace-only text
  nodes, so a freshly written empty element (`<a></a>`) compares equal to a
  parsed one (`<a/>`). Rows the baseline lacks entirely, or holds bare-empty
  while empty is not a legitimate value for the row (`empty_ok`), loaded as
  defaults and compare against a default-constructed footprint: unchanged
  defaults are not written and cannot clobber a concurrent edit.
  Known edges of the normalization (both require pathological input and
  lose no data): a whitespace-only value in a row where empty is not
  legitimate is stripped to bare on both comparison sides and rewrites
  idempotently on every save; a hand-emptied never-bare row (e.g. the
  online repo list) self-heals to its defaults after one spurious rewrite.
* **The baseline is never refreshed after a save.** Adopting the merged
  document would absorb other processes' values; this instance's untouched
  fields would then register as "changed" on the next save and clobber those
  values. Changed rows are re-written idempotently on every save
  (last-writer-wins per field). Consequence: reverting a field to its
  loaded value in memory is *not* written back (accepted; matches the
  legacy `PluginConfig` semantics).
* **Self-heal writes a full snapshot.** On a corrupt, missing, or
  foreign-rooted disk document the save logs an error and writes every
  ownership-eligible row onto a fresh root. A partial (changed-rows-only)
  write would produce an incomplete config file.
* **Layer column = the ADR 0022 set, exactly.** 27 `Override` rows (audio
  driver trio, MIDI driver identity, per-driver device subtrees, JACK and
  OSC state, last-opened pointers, recent songs), pinned by an exact-set
  unit test.
* **Atomic writes.** `XMLDoc::write` stages the payload in a `QSaveFile`
  and commits via rename — every XML writer benefits, and a crash mid-write
  can no longer tear a document.
* **Fresh installs.** A lock file cannot be created in a nonexistent
  directory; `save()` creates the config directory before acquiring the
  lock.
* **Order-preserving metadata refresh.** The `formatVersion`/`version`
  header elements are updated in place; a save that changes no row leaves
  the document order — and, modulo Qt's empty-element serialization, the
  file bytes — untouched.
* **Ownership tagging.** `FieldOwnership` is instance state, set once at
  process startup: editor mirror → GuiOwned, in-DAW plugin engine →
  BaseLayer, standalone GUI / headless engine → All (default). The GUI
  process never runs inside a host (its UI is out-of-process,
  [ADR 0016](0016-out-of-process-plugin-ui.md)), so a leftover `Plugin`
  audio driver in the config must not mask the override rows in the
  standalone GUI — the stuck value would be unrepairable from there.

### Implementation notes (2026-09-07, Phase 3)

* **Snapshots skip default-equal rows.** The "full snapshot" written on
  self-heal (corrupt/missing/foreign disk document) or by a
  never-loaded instance is *everything that differs from a
  default-constructed instance*, not literally every row: rows whose
  current footprint equals their default footprint are skipped by the
  same comparison that powers the incremental merge. A self-healed file
  can therefore legitimately lack rows (e.g. the bare `<shortcuts/>` of
  a fresh install); loading it treats those rows as fresh-install
  state. This is coherent because absent and bare converge for such
  rows (next point).
* **Shortcuts defer to a GUI application.** Creating the default
  shortcuts involves `QKeySequence::keyBindings`, which requires a
  platform theme — a `QGuiApplication`. A bare `QCoreApplication`
  (h2cli, the unit-test runner) used to pass the old guard and
  segfault. `Shortcuts::loadFrom` now defers (`m_bRequiresDefaults`)
  unless a `QGuiApplication` instance exists; the GUI bootstrap
  consumes the flag after `QApplication` construction. A *missing* and
  a *bare* `<shortcuts>` element both converge to "deferred defaults",
  and a deferred instance's save leaves the row absent (empty map ==
  default footprint → skipped), so the signal survives round-trips.
  Consequence: an *empty* shortcuts list is not expressible — a user
  who deletes every binding saves a bare row, which the next load
  treats as deferred defaults (legacy semantics, unchanged).
* **Deterministic test seeds.** The persistence/concurrency suites seed
  from the shipped `data/hydrogen.default.conf` (version header
  normalized to the current build) instead of a snapshot of the ambient
  user config — fixtures are identical on every machine. (The load-time
  Rubberband auto-detection still replaces the in-memory path with the
  local binary; the no-op test restores the on-disk value to keep its
  premise.)
* **Cross-process hammer.** The test binary re-executes itself
  (`--config-hammer`) as a lean headless child — no test environment,
  no Hydrogen instance, close to an h2cli footprint — and three such
  processes hammer the shared config concurrently. The child's exit
  code reports how many saves the bounded retry budget dropped: a
  small positive count is the budget doing its job under contention,
  not a failure; corruption or a lost sibling edit is.
* **Save failures are surfaced.** The Preferences dialog warns the
  user when `save()` returns false instead of silently dropping their
  changes.
* **Canary residual hole.** The structured-binding tie covers the
  `PreferencesData` aggregate (121 members) and the row-count assert
  pins the schema; `InterfaceTheme`/`FontTheme` members are tied via
  their schema rows instead — they derive from `Object<T>` whose base
  has non-static data members, so C++17 single-class decomposition
  cannot enumerate them directly.

## More Information

* The "shared user config file" is **not** literally `~/.hydrogen`. Its location
  is abstracted by `Filesystem::userConfigPath()`: on Linux it is XDG-derived
  (`$XDG_CONFIG_HOME/hydrogen/hydrogen.conf`, defaulting to
  `~/.config/hydrogen/hydrogen.conf`), and platform-specific on macOS/Windows;
  `~/.hydrogen` is only the legacy fallback (used when that directory already
  exists). The locked merge-write operates on whatever path `userConfigPath()`
  resolves to — it is path-agnostic.
* `src/core/Preferences/Preferences.cpp` (`save`, `saveCopyAs`),
  `src/core/Preferences/PreferencesSchema.cpp` (`persistRows`); call sites
  `src/gui/src/main.cpp`, `src/gui/src/MainForm.cpp`,
  `src/core/CoreActionController.cpp`.
* Related: [ADR 0015](0015-per-instance-engine-context.md),
  [ADR 0022](0022-layered-plugin-configuration.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
