---
status: accepted
date: 2026-06-23
deciders: pm
---

# AD: Routing CoreActionController over IPC (the editor-mode command carrier)

## Context and Problem Statement

[ADR 0027](0027-coreactioncontroller-single-write-surface.md) made
`CoreActionController` (CAC) the single GUI→engine write surface and **deferred**
the choice of *how* to carry CAC across the editor↔engine process split until the
surface was complete: *"the choice of how to carry CAC across the wire
(command-sink vs virtualisation) becomes a minor, well-informed implementation
detail decided afterwards."* Phase 4.5 ([proposal 0004 §8.5](/docs/proposals/0004-plugin-port-implementation-plan.md))
is now done and CI-enforced, so the surface is known. This ADR makes that
decision.

Today in editor mode the GUI is **half-connected**:

* Reads come off a local headless mirror via `IpcEngineAccess` (Phase 2/T5.2).
* A live `QLocalSocket` channel, an `IpcEngineBridge` that dispatches command
  opcodes into a real engine, telemetry shared-memory, and the editor state
  mirror all exist (T5.1/T5.2).
* **But** `IpcEngineAccess::getCoreActionController()` returns the **mirror's** CAC
  (`IpcEngineAccess.h:62-63`). So GUI commands mutate the editor's local mirror,
  **not** the host's engine. And the opcode vocabulary (`IpcOpcode`,
  `IpcEngineBridge::dispatchCommand`) covers only ~20 of CAC's **122** public
  methods.

So two things are unresolved: **(1)** the mechanism by which an editor-side CAC
call becomes an IPC command instead of a local mutation, and **(2)** the
request/response and return-value semantics, given 113 of the 122 methods return
`bool`, 6 return a live `shared_ptr<core object>`, and several GUI sites branch on
that return.

## Decision Drivers

* **The GUI must stay agnostic.** It already calls
  `pEngine()->getCoreActionController()->X(...)`; standalone and editor mode must
  both satisfy that surface, exactly as `IEngineAccess` does for reads.
* **Standalone unchanged.** The local path must keep doing precisely what it does
  today (ADR 0027). CAC is *not* on the audio thread — it is invoked on
  user actions / undo commands — so virtual dispatch on it is free in practice
  (ADR 0027's "don't make a hot class polymorphic" caution does not apply to a
  GUI-rate command surface; it was a caution against disturbing the engine).
* **Ordered, reliable, non-blocking commands** (ADR 0018): the editor must not
  block the UI waiting on the engine for the common case.
* **Return values:** most are `bool` success flags the GUI ignores; a few gate
  control flow; six return engine objects. The carrier must handle all three
  without a synchronous round-trip on the hot path.
* **Bounded, mechanical implementation.** ~122 methods is a large but mechanical
  sweep; the design should not also require per-method bespoke logic.

## Considered Options

1. **Virtualise the GUI-facing CAC methods; an `IpcCoreActionController : CoreActionController`
   subclass overrides each to marshal an `IpcMessage` and send it.** The base
   class is unchanged (standalone identical); the subclass is constructed in the
   editor over the mirror + channel and returned by
   `IpcEngineAccess::getCoreActionController()`.
2. **Extract a pure-virtual `ICoreActionController` interface** the GUI codes
   against, with `CoreActionController` (local) and `IpcCoreActionController`
   (proxy) implementations — the `IEngineAccess` pattern applied to commands.
3. **Command-sink:** rewrite every CAC body to build a command object and hand it
   to an injected sink (local sink executes; IPC sink serialises). Rejected:
   rewrites all 122 bodies and duplicates each as "the local sink's executor" —
   maximal churn for no extra benefit over (1).
4. **Generic/reflective marshalling** (one opcode carrying a method id + a
   `QVariantList`). Rejected: C++ has no reflection, so the engine side still
   needs a 122-entry dispatch; it only trades typed opcodes (debuggable, versioned)
   for an untyped blob, losing the `IpcProtocolTest` per-command guarantees.

## Decision Outcome

Chosen: **option 1 — virtualise the GUI-facing CAC methods and add an
`IpcCoreActionController` subclass that marshals each call to an `IpcMessage`.**
It keeps the standalone path byte-for-byte (the base class is unchanged and used
directly), mirrors the established `IEngineAccess` Local/Ipc split for the command
surface, and avoids the duplicate-declaration overhead of a separate interface
(option 2) while being far less churn than the command-sink (option 3).

`IpcEngineAccess::getCoreActionController()` returns the `IpcCoreActionController`
(constructed over the same mirror + `IpcChannel`). Each override sends its opcode
and **applies the editor-local view consequence to the mirror only where one is
needed for snappy UI** (the pattern already used by `IpcEngineAccess::sequencerStop`).

### Command semantics — three tiers

1. **Fire-and-forget (the 113 `bool` mutators).** The override sends the opcode
   and returns `true`. Editor-side validation cannot meaningfully fail (the engine
   is authoritative), and genuine engine-side failures are rare and already
   surface as state/events the mirror consumes. Ordering is guaranteed by the
   single `QLocalSocket` (ADR 0018), so a later command never overtakes an earlier
   one.

2. **Control-flow gates that the GUI checks** (`setSong`, `saveSong`/`saveSongAs`,
   `savePlaylist*`, `locateToColumn`/`locateToTick`, `setPatternSize`, …). These
   keep returning `true` optimistically; the authoritative result comes back as an
   **engine-origin event** the GUI already handles (e.g. `SongSizeChanged`,
   error events). Where the GUI today shows an error dialog on a synchronous
   `false`, that moves to the event path. **File save/load is engine-side** in
   editor mode (the host process owns the song + state file); the editor issues
   the command and reflects the outcome from the resulting event — this also
   closes the ~200 "GUI passes `Hydrogen*` to `Song::load`/…" sites that Phase 2
   (proposal 0004 §6) deferred to here.

3. **Object-returning methods (6) → synchronous request/response.** The handful
   returning `shared_ptr<Instrument|Drumkit|InstrumentComponent|InstrumentLayer|
   Pattern|Preferences>` cannot be fire-and-forget. They use a **blocking
   request/response** round-trip on the channel (a `reply` frame keyed to a
   request id) at user-action rate — acceptable latency, and rare. The returned
   object is reconstructed into / resolved against the mirror by value identity
   ([ADR 0028](0028-object-instance-identity.md)). Candidates to instead restructure
   into a fire-and-forget command + a follow-up event are noted per-method during
   implementation; request/response is the fallback that always works.

### Opcode vocabulary

Extend `IpcOpcode` + `IpcEngineBridge::dispatchCommand` from the current ~20 to the
**full set of CAC methods the GUI actually calls** (the ~118 call sites resolve to
~60–70 distinct methods; ~20 already covered). Opcodes stay **explicit and typed**
(one enum value + one dispatch case + one `IpcProtocolTest` round-trip per
command) — the cost option 4 was rejected to preserve. Args reuse the existing
`QVariant`/`QByteArray` framing; large payloads (song/drumkit/state) stay XML
`QByteArray` as already done.

### Consequences

* **`CoreActionController` gains `virtual` on its GUI-facing methods** + a virtual
  destructor; the engine/standalone path calls the base implementations exactly
  as today (negligible dispatch cost — not the audio thread).
* **New `IpcCoreActionController`** (in `src/core/IPC/`) — ~one thin override per
  GUI-used method; `IpcEngineAccess` constructs and returns it.
* **`IpcEngineBridge` grows** a dispatch case per new opcode; **`IpcOpcode` and
  `IpcProtocolTest`** grow in lockstep (round-trip per command).
* **A request/response path** is added to `IpcChannel` for the 6 object-returning
  methods (request id + reply frame); most commands never use it.
* **Editor-mode error reporting shifts** from synchronous `bool` to engine-origin
  events for the control-flow gates; the GUI's existing event handlers absorb it.
* **Standalone is unchanged**; `IpcProtocolTest`/`IpcTransportTest` extend to the
  new opcodes, and an integration `EditorModeTest` (T5.3/T5.4, Linux) asserts a
  GUI command actually mutates the *engine* (not the mirror) across the socket.
* This unblocks **T5.3 (`--plugin-editor` bootstrap)** and **T5.4 (lifecycle)**:
  once commands reach the engine, editor mode is functionally complete.

## More Information

* Deferred-from: [ADR 0027](0027-coreactioncontroller-single-write-surface.md)
  ("command-sink vs virtualisation … decided afterwards").
* Transport / event / telemetry split: [ADR 0018](0018-plugin-editor-ipc-transport.md).
* Editor architecture & injected handle: [ADR 0016](0016-out-of-process-plugin-ui.md).
* Value identity for object-returning replies: [ADR 0028](0028-object-instance-identity.md).
* Code: `src/core/CoreActionController.h` (122 methods),
  `src/core/IPC/IpcEngineAccess.{h,cpp}`, `src/core/IPC/IpcEngineBridge.cpp`,
  `src/core/IPC/IpcMessage.h` (`IpcOpcode`), `src/core/IPC/IpcChannel.{h,cpp}`.
* Plan: [proposal 0004 §9](/docs/proposals/0004-plugin-port-implementation-plan.md)
  (T5.2-cont / T5.3 / T5.4).
