---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Plugin state embeds a self-contained project by default, with a song-only opt-out

## Context and Problem Statement

A plugin must save and restore its complete state as part of the host project
(DAW save/load) and as presets. Hydrogen's working document is a `Song`
(`src/core/Basics/Song.h`) plus its drumkit and samples, normally persisted as an
`.h2song` file referencing kit/sample files on disk. We must decide what the
plugin writes into the host-provided state blob (LV2 state, `clap_plugin_state`,
VST3 `IBStream`).

## Decision Drivers

* DAW projects are expected to be **self-contained and portable** across
  machines — an instrument that breaks when files move is a support burden.
* But drumkits can be **very large** (SFZ kits > 1 GB); forcing every DAW project
  to embed the full kit would bloat saves unacceptably.
* Round-tripping back to standalone Hydrogen is desirable but secondary.

## Considered Options

1. **Always embed the full song + kit in the state.**
2. **Reference an external `.h2song`/kit path only.**
3. **Embed a self-contained project by default, with a user opt-out to store
   song-only** (kit by reference).

## Decision Outcome

Chosen option: **embed a self-contained project by default, with a song-only
opt-out (option 3).**

By default the plugin state is a full, self-contained **`.h2project`** bundle —
the song plus the kit's sample data ([ADR 0025](0025-h2project-self-contained-format.md)
for the format, [ADR 0020](0020-plugin-state-sample-embedding.md) for the
mechanism) — so the DAW project is portable across machines, as users expect of a
sampler instrument. A **preference toggle "store drumkit samples in plugin state"
(ON by default)** lets the user switch a project to **song-only** state (the
`.h2song` equivalent, referencing the installed kit) when the kit is too large to
embed sensibly. The same state reader loads either form.

Always-embed (option 1) is rejected because of the >1 GB-kit case. Reference-only
(option 2) is rejected as the *default* (fragile across machines) but is exactly
what the opt-out provides for users who want it.

### Consequences

* The song/kit serialisation must support a **fully self-contained** form (sample
  audio embedded, not only paths) — this is the `.h2project` codec of
  [ADR 0020](0020-plugin-state-sample-embedding.md) / [ADR 0025](0025-h2project-self-contained-format.md).
* Default-on state blobs can be **large** for big multi-sample kits; accepted, and
  handled off the audio thread (state save/load is main-thread/worker in all
  target formats). The opt-out exists precisely for the extreme cases.
* **Song-only state (opt-out) is not portable** — it requires the drumkit to be
  installed on the target machine, exactly like a classic `.h2song`. This is the
  documented trade the user makes to avoid embedding a gigabyte kit.
* Preset interchange with standalone Hydrogen is natural: embedded state *is* a
  `.h2project`, song-only state *is* a `.h2song`.

## More Information

* Serialisation entry points: `src/core/Basics/Song.h`, `Drumkit`,
  `src/core/Helpers/` XML helpers.
* Related: [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md),
  [ADR 0020](0020-plugin-state-sample-embedding.md),
  [ADR 0025](0025-h2project-self-contained-format.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
