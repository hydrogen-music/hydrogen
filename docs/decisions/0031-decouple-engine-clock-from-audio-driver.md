---
status: accepted
date: 2026-06-27
deciders: pm
---

# AD: Decouple the engine clock from the audio driver (run without audio)

## Context and Problem Statement

The `AudioDriver` (`src/core/IO/AudioDriver.h`) conflates two unrelated roles:

1. **Audio output sink** — it owns the `getOut_L()/getOut_R()` buffers that the
   engine renders into.
2. **The engine clock** — it invokes `audioEngine_process(nframes)` at the rate
   `bufferSize / sampleRate`. That single call advances the transport by exactly
   `nframes` (`AudioEngine::incrementPlayhead`, `AudioEngine.cpp:1951`), runs note
   scheduling and inbound MIDI (`updateNoteQueue(nframes)`, `:1928`), and only
   *then* renders audio (`processAudio(nframes)`, `:1930`).

Because the clock is welded to the audio driver, **"no audio driver" means "no
clock"** and the engine freezes. Concretely:

* `audioEngine_process` early-returns when `m_pAudioDriver == nullptr`
  (`AudioEngine.cpp:1799`); the engine needs *a* driver object to run at all.
* `NullDriver` is inert — no thread, no callback, `getSampleRate()/getBufferSize()
  == 0`, `getOut == nullptr`. It is only the safe fallback when a real driver
  fails `init/connect`.
* `FakeAudioDriver` already *is* "a clock without real audio": a timer thread
  drives the callback at `bufferSize/sampleRate` into scratch buffers
  (`FakeAudioDriver.cpp:71`, `:148-166`). It is test-flavoured and self-clocks
  from `Preferences`.
* `MidiDriver::None` is handled (leaves MIDI null); `AudioDriver::None` is **not**
  — it is only a parse-error sentinel. So "MIDI but no audio" is unrepresentable.

This blocks two capabilities:

* **A true missing feature:** Hydrogen can run with an audio driver and no MIDI
  driver (it processes notes, writes peaks/audio, but neither sends nor handles
  MIDI). The reverse is impossible — it cannot run with a MIDI driver and **no
  audio driver**, processing notes on its own clock and sending/handling MIDI
  while touching no audio buffers.
* **The editor mirror** ([ADR 0016](0016-out-of-process-plugin-ui.md),
  [0030](0030-coreactioncontroller-over-ipc.md)) needs a *running* engine so the
  GUI can display an **advancing** transport position and playing-pattern state
  (`SongEditorPositionRuler` reads `pHydrogen->getAudioEngine()`), and so the
  mirror can **apply inbound host transport changes**. Transport itself —
  play/stop and playhead relocation — is host-owned and read-only in the editor
  ([ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md)); the editor never
  *issues* a relocation, it only *follows* the host's. With the inert `NullDriver`
  the mirror's transport never advances. (The shared-memory telemetry block of
  [ADR 0018](0018-plugin-editor-ipc-transport.md) carries the host's position but
  is not yet consumed editor-side.)

The clock is already decoupled in spirit by
[ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md) (plugin transport is
host-driven) and [ADR 0029](0029-audio-driver-access-across-editor-split.md)
(sample rate / buffer size are *config*, host-provided, not driver-derived). This
ADR makes the decoupling explicit and usable outside the plugin.

A prerequisite holds: the `Sampler` cleanly separates the MIDI emission from the
audio render within `handleNote()` — the `renderNote()` call (`Sampler.cpp:1026`)
produces audio, the `bSendMidiNoteOn` block (`:1033-1063`) and the note-off queues
emit MIDI, gated only on `getMidiDriver() != nullptr`. So MIDI can be produced
without producing audio.

## Decision Drivers

* Run the engine on its own clock with **no audio** (MIDI-only) and with **neither
  audio nor MIDI** (editor mirror), without touching real output buffers.
* Reuse the proven timer-pump mechanism rather than inventing a new one.
* Reduce near-duplicate driver classes and improve their test coverage.
* Never freeze the engine for lack of an audio device — degrade to headless
  (clocked, no output) instead.
* Bound transport drift in the editor mirror over long playback.
* Clean thread lifecycle (a rogue driver thread logging during teardown caused a
  use-after-free segfault — see the `--plugin-editor` fix, proposal 0004 §10).

## Considered Options

**Driver structure**
1. Add a fourth driver alongside `NullDriver`/`FakeAudioDriver`.
2. **Consolidate** `NullDriver` + `FakeAudioDriver` + the new role into one
   parameterised software driver. *(chosen)*
3. Deeper refactor: separate the "clock source" from the `AudioDriver`
   abstraction entirely. *(rejected for now — too invasive; the driver already
   is the clock seam and `createAudioDriver`/`m_AudioProcessCallback` wiring fits
   option 2 with minimal blast radius.)*

**Audio-failure / "no audio" fallback**
* Keep a frozen, unclocked `NullDriver` (current behaviour) — clear "broken"
  signal, but the engine does nothing (no transport, no MIDI).
* **Always-clocked headless fallback** — degrade to the software driver with no
  output; the engine keeps running (transport, notes, MIDI), the GUI flags "no
  audio device". Lets us drop the inert mode entirely (one behaviour, simpler,
  better-tested). *(chosen — usage review showed the only genuine inert users were
  this fallback and the editor, and the user-selectable "no audio" device is
  better served as the MIDI-only headless mode.)*

**Editor mirror transport sync**
* Free-running local pump only — drifts unboundedly from the host.
* Telemetry-slaved only — no drift, but the playhead only advances as fast as
  telemetry arrives (coarse, less smooth than a local clock).
* **Hybrid** — free-running local pump for a smooth local playhead, corrected on
  discrete inbound transport events, *and* a periodic forced re-sync from
  telemetry to bound drift. *(chosen)*

## Decision Outcome

### 1. One always-clocked software audio driver; only the output is optional

Collapse `NullDriver` and `FakeAudioDriver` (and the new headless/MIDI-only/editor
role) into a single **software driver** that is *always* clocked by an internal
timer (at `bufferSize / sampleRate`). There is **no "clock off" mode** — only the
audio **output** is optional:

| role | sample rate / buffer | audio output |
|---|---|---|
| self-clocked (was FakeAudioDriver) | from config / fallback | scratch |
| headless / MIDI-only / editor / **audio-failure fallback** | host-provided or fallback | none |

Parameters: `{ sampleRate, bufferSize, producesAudio }`. The timer configuration
subsumes `FakeAudioDriver`; the inert `NullDriver` is **dropped**.
`createAudioDriver` maps the relevant `Preferences::AudioDriver` values onto this
one class.

**Headless is the new fallback.** When a real audio driver fails `init/connect`,
`startAudioDriver` falls back to this software driver (clocked, `producesAudio =
false`) instead of the frozen `NullDriver`. The engine keeps running — transport
advances, notes process, MIDI flows — with no audio output, rather than freezing.
Likewise the user-selectable "no audio" device becomes a headless (clocked,
output-less) instance, which is the standalone MIDI-only mode. Because every
instance therefore always holds a (software or real) driver, the
`m_pAudioDriver != nullptr` early-return in `audioEngine_process` (`:1799`)
effectively never fires; it stays as a defensive guard.

The GUI distinguishes "no real audio output" via the existing
[`AudioDriverInfo`](0029-audio-driver-access-across-editor-split.md) descriptor —
`isPresent == true` (a driver object exists), `isRunning == false` (it is not a
real audio device) — so a headless/fallback context is shown clearly, not as a
silent surprise.

`PluginAudioDriver` (clocked by the host's `process()` callback, host-supplied
output buffers, `followHostTransport`), `DiskWriterDriver` (offline render loop to
a file), and the hardware drivers stay separate — they are genuinely different
*clock sources*, not parameterisations of the software pump. They may later share
an interface/base, but that is out of scope here.

### 2. `audioEngine_process` becomes audio-optional

When the driver produces no audio, the cycle still runs transport advance,
`updateNoteQueue`, and the `Sampler` `handleNote` path (note lifetime + MIDI-out +
peaks), but nothing is written to a real output sink. The minimal, correct v1 is
**render-to-scratch** (the `FakeAudioDriver` model): the sampler renders into its
internal main-out so note lifetime (signalled by `renderNote`'s return) and
peaks remain correct, and the driver's `getOut` buffers are scratch/none. A true
"advance note lifetime without sample math" silent path is a later optimisation,
noted because render-to-scratch still spends CPU on discarded audio.

### 3. Three runtime modes

| mode | audio driver | MIDI driver | clock | rate/buffer | output |
|---|---|---|---|---|---|
| audio (today) | hardware | optional | hardware | from driver | written |
| **MIDI-only (new)** | software (timer) | real (ALSA/…) | timer | fixed fallback | not touched |
| **editor / neither** | software (timer) | none | timer | host-provided | not touched |
| plugin (today) | `PluginAudioDriver` | `PluginMidiDriver` | host | from host | host's |

MIDI runs independently of audio: `startAudioDriver` and `startMidiDriver` already
have no hard dependency, and `MidiDriver::None` already yields a null MIDI driver.

### 4. Sample rate / buffer-size sourcing

Per [ADR 0029](0029-audio-driver-access-across-editor-split.md) these are config,
not driver-derived. Editor/plugin: **host-provided** through the override layer
([ADR 0022](0022-layered-plugin-configuration.md)), passed to the editor at
bootstrap. Standalone headless/MIDI-only: a **fixed fallback** (e.g. 48000 Hz /
1024 frames) from `Preferences` defaults. (`NullDriver`'s 0/0 was unusable for the
transport math; the software driver always reports valid values.)

### 5. Editor mirror transport sync — hybrid

The mirror's software driver **free-runs** at the host-provided rate so the local
playhead and playing-pattern display advance smoothly between corrections. The
editor does not drive transport (play/stop/relocate are host-only,
[ADR 0026](0026-plugin-mode-feature-disablement-and-gui.md)); correctness is
maintained by following the host via two inbound correction signals:

* **Event-driven** — discrete host transport changes (the host relocating its
  playhead, play/stop, tempo) arrive over the control socket and relocate the
  mirror immediately (the `followHostTransport` pattern, applied editor-side).
* **Periodic forced re-sync** — to stop a free-running clock accumulating
  intolerable drift over long playback, the mirror snaps its transport to the
  telemetry block's authoritative `frame` on a fixed cadence — **every ~30 000
  frames (~5 s at 48 kHz)**. This wires up the currently-unconsumed telemetry
  (ADR 0018) as the drift anchor.

### Consequences

Positive:
* Hydrogen can run **MIDI-only** (own clock, sends/handles MIDI, no audio) — the
  missing feature — and **headless** (editor mirror) with a live transport.
* **One always-clocked software driver replaces both `NullDriver` and
  `FakeAudioDriver`** — fewer classes, no "is it clocked?" branching, and the
  tests exercise the *production* clock path rather than a test-only sibling.
* Audio-device failure degrades to a **responsive headless engine** (transport +
  MIDI keep working) instead of a frozen one.
* The editor's local transport/playing-pattern display advances smoothly and
  follows the host with bounded drift (the editor's dual-applied CAC edits act on
  a live mirror engine; transport remains host-driven).

Negative / risks:
* **Behaviour change:** audio-init failure now runs headless (no sound) instead of
  freezing with an error. Mitigated by the `AudioDriverInfo` "no audio device"
  indication so it is not a silent surprise.
* Every standalone/editor instance now runs a clock thread (plugin instances use
  the host-clocked `PluginAudioDriver`, so a host loading many instances is
  unaffected). That thread MUST be **joined before** the Logger/engine teardown to
  avoid the use-after-free class of bug (proposal 0004 §10) — the consolidated
  driver must get this right where the old `NullDriver` (no thread) did so
  trivially.
* Render-to-scratch spends CPU rendering discarded audio in no-audio mode until
  the silent-render optimisation lands.
* Between corrections the mirror's position is approximate (bounded by the ~5 s
  re-sync and event corrections); acceptable for a UI mirror.

## More Information

* Builds on [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md) (host-driven
  transport), [ADR 0016](0016-out-of-process-plugin-ui.md) /
  [0018](0018-plugin-editor-ipc-transport.md) (editor split + telemetry),
  [ADR 0029](0029-audio-driver-access-across-editor-split.md) (rate/buffer are
  config; driver stays engine-side), [ADR 0030](0030-coreactioncontroller-over-ipc.md)
  (dual-apply on the mirror).
* Implementation phasing to be tracked in proposal 0004 (the plugin-port plan).

## Addition 2026-07-23

* The CLI option `--plugin-editor` was renamed to `--connect-via-ipc`.
* The unit test `EditorModeTest.cpp` was renamed to `ConnectViaIpcModeTest.cpp`.
