---
status: accepted
date: 2026-06-08
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
that means several processes each rewriting the whole `~/.hydrogen` config file at
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

## More Information

* `src/core/Preferences/Preferences.cpp:1405` (`save`),
  `:1410` (`saveTo`); call sites `src/gui/src/main.cpp:534`,
  `src/gui/src/MainForm.cpp:2758`.
* Related: [ADR 0015](0015-per-instance-engine-context.md),
  [ADR 0022](0022-layered-plugin-configuration.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
