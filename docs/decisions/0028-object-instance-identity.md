---
status: accepted
date: 2026-06-18
deciders: pm
---

# AD: Object-level instance identity (`Object::m_uuid`)

## Context and Problem Statement

The engine identifies core objects by **raw `shared_ptr` pointer comparison** in
many places, and these comparisons are spread across `src/core/Basics/`:

* note queue / sampler: `note->getInstrument() == pInstr`
  (`AudioEngine::clearNoteQueues`, `Sampler::releasePlayingNotes`)
* instrument death row: `*it == pInstr`
  (`Hydrogen::removeInstrumentFromDeathRow`, lifetime via `isQueued()`)
* container location by pointer: `InstrumentList::index/del`,
  `PatternList::index/del/virtualPatternDel`, `Instrument::index(component)`,
  `InstrumentComponent::index(layer)`, `Pattern::removeNote/references/
  purgeInstrument`, `Playlist::remove(entry)`

Two forces make pointer identity untenable going forward:

1. **The editor/host split** ([ADR 0016](0016-out-of-process-plugin-ui.md)): a
   pointer held in the editor process is meaningless to the engine process.
2. **No safe value fallback today.** `Instrument::Id` is only kit-unique
   (zero-based ascending), so the site already matching by `getId()`
   (`Sampler.cpp:427`) is a *latent bug*: during a drumkit switch, old-kit
   instruments on the death row coexist with the new kit and can be mis-matched
   by `Id`.

## Decision Drivers

* **Uniformity** — a single identity mechanism usable by every `Object`, so
  "compare by identity" means the same thing everywhere (no per-site reasoning).
* **Real-time safety** — `Note` is an `Object` and is **copied on the audio
  thread** during playback (`AudioEngine.cpp:1693,3023,3137,3188`,
  `Sampler.cpp:1987`). Whatever mints identity must be safe to call there.
* **Negligible cost** — it sits on the hottest objects in the program.
* **Serialisable / cross-process-capable** — so the same handle can later name an
  object across the split.

## Considered Options

1. **Strong-typed UUIDs per class -`Instrument::Uuid`** — scales badly and
   doesn't unify.
2. **`Object`-level id backed by `QUuid::createUuid()` per object** — uniform,
   but `createUuid()` draws from `QRandomGenerator::global()`, a process-shared,
   internally-synchronised generator. Calling it while copying notes on the audio
   thread takes a lock shared with non-RT threads → **priority-inversion / xrun
   hazard.** Rejected for the RT path, independent of its (otherwise negligible)
   average cost.
3. **`Object`-level id backed by a process-tagged atomic counter** *(chosen)* —
   uniform, lock-free/wait-free, ~nanoseconds, and globally unique.

## Decision Outcome

Add an **immutable instance identity to the `Object`/`Base` base class**:
`m_uuid` with `getUuid()`, available to every derived class. It is minted in
**every constructor, including the copy constructor**, and never changes after
construction.

* **Backing: a process-tagged atomic counter, not `QUuid::createUuid()`.** The
  value is `{ epoch, counter }`: a per-process `epoch` seeded once at startup
  (a single RNG draw, off the RT path) and a `counter` advanced by a wait-free
  `fetch_add`. This is lock-free (RT-safe on the note path), ~nanoseconds, and
  unique across processes and time (so it can serve the split later). The value
  type is a small POD shared by all classes — **not** a per-class strong type
  ("no strong type this time").
* **Mint on every construction, including copy.** Each object — original, clone,
  queued note copy — gets its own fresh identity. This is the key simplification:
  it is exactly "pointer identity, but as a value," so in-process behaviour is
  unchanged, *and* it hands instruments their distinct-clone identity for free
  (the clone-for-replace differs from the death-rowed original, so death-row /
  note-queue matching stays unambiguous) **with no per-class special-casing.**
* **Centralise equality in one comparator — don't hand-edit every site.**
  Overload equality on the `Object` pointee to compare `m_uuid`, and provide a
  null-safe `shared_ptr` comparator (e.g. `sameObject(a, b)`); the raw `a == b`
  identity checks (death row, note queue) route through it, so the rule lives in
  one place and no site can be forgotten. (`std::shared_ptr`'s own `==` is left
  untouched — it legally can't be overloaded; the comparison moves to the
  pointee / the helper.)
* **`index()` locates by uuid** `InstrumentList`/`PatternList`/… expose
  `index(const Uuid&)` returning **the same position/index as before** (callers
  holding a pointer pass `p->getUuid()`); `del`/`remove` likewise locate by
  uuid.
* **Runtime-only.** The id is a runtime instance identity, **not** serialised to
  disk (regenerated on load); no file-format / `XmlTest` churn, and in-process
  behaviour is preserved (a unique id makes `getUuid()==` exactly `ptr==`). The
  cross-process *correspondence* question (how the editor and engine agree on
  the same object's id — likely the engine transferring its ids in the state
  snapshot, a preserve-on-deserialize path distinct from the mint-on-copy rule)
  is **deferred** to editor mode. The epoch already guarantees a foreign-process
  id never *spuriously* matches a local object — fail-safe by construction.

### Cost (the question that drove this)

* **RAM:** 16 bytes/object (`{epoch,counter}`; 8 if a bare counter). Dominated by
  notes — a large song ≈ 20k stored notes ≈ ~320 KB; **<0.1 %** of sample PCM
  (tens of MB). Negligible.
* **CPU:** wait-free `fetch_add` (~ns). Playback mints ≲1000 notes/s →
  single-digit µs/s. Negligible, and — unlike `QUuid` — **safe on the audio
  thread.**

### Consequences

* **One uniform identity rule** replaces every pointer-identity site (note queue,
  death row, `InstrumentList`/`PatternList`/`Instrument`/`InstrumentComponent`
  `index`, `Playlist::remove`, `Pattern::removeNote`/`references`/
  `purgeInstrument`). No more per-site "is this one split-safe?" reasoning.
* **Fixes the latent cross-kit `Id` bug** (`Sampler.cpp:427`).
* **Index-addressing still works but is no longer load-bearing for
  correctness.** Position/index remains how the GUI names a pattern/component/
  layer *slot* across the split (cheap, stable under a mirror-ordering
  invariant), but identity comparisons no longer depend on it.
* In-process behaviour is unchanged; the new identity only does additional work
  once the split needs to name objects across processes.
* **Open (deferred):** persisting/transferring the id for editor↔engine
  correspondence; the exact epoch width; whether any class caches a peer's id.
  None block the in-process sweep.

## Implementation notes (2026-06-19)

One site listed above as "container location by pointer" turned out **not** to
be pointer-identity comparisons and were correctly left unchanged:

* **`Playlist::remove`** matches entries by a **value** `operator==`
  (song path / script path / enabled), not by pointer identity — two distinct
  `PlaylistEntry` objects describing the same song are intentionally "equal"
  for removal. Identity (uuid) would change that behaviour.

Otherwise the sweep matches this ADR: a `sameObject(a,b)` helper centralises
equality, and `index`/`del` gained uuid overloads (positional return preserved).
The atomic counter is `static_assert`-ed lock-free to guarantee the RT property.

## More Information

* Write-surface decision and the identity-coupling review:
  [ADR 0027](0027-coreactioncontroller-single-write-surface.md)
* Pointer-identity catalogue rows:
  [proposal 0005](/docs/proposals/0005-gui-engine-write-surface-catalogue.md)
* Phase 4.4 in the plan:
  [proposal 0004 §8.4](/docs/proposals/0004-plugin-port-implementation-plan.md)
* Out-of-process editor: [ADR 0016](0016-out-of-process-plugin-ui.md)
