---
status: accepted
date: 2026-06-09
deciders: pm
---

# AD: Remove LADSPA/LRDF effect hosting (deprecate in 1.2.7, remove in 2.0)

## Context and Problem Statement

Hydrogen can host **LADSPA effect plugins** as inserts (`src/core/FX/`,
`Song::getEffects()`), with **LRDF** providing LADSPA plugin metadata/categories.
This is distinct from the 2.0 work of turning Hydrogen *itself* into a plugin
([proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)).

Two things make this effect-hosting subsystem no longer worth carrying:

1. **Hydrogen is becoming a plugin.** As an LV2/CLAP/VST3 plugin it lives next to
   every other plugin in the host. The host owns the effects chain; there is no
   reason for Hydrogen to do sophisticated audio processing (effect hosting)
   internally anymore.
2. **It was already weak.** LADSPA is a Linux-only standard (it does not exist on
   the Windows/macOS targets), LRDF is an awkward extra dependency, and the
   in-app effect UX has long been lacking.

It is also a maintenance and build cost: `WANT_LADSPA`/`WANT_LRDF`, `FindLadspa`,
the LRDF probe, CI deps (`ladspa-sdk`, `liblrdf-dev`), `#ifdef H2CORE_HAVE_LADSPA`
branches in the audio path (`AudioEngine::setupLadspaFX`, the `processAudio` FX
loop, `Sampler.cpp:1828`), the GUI (`LadspaFXProperties`, `LadspaFXSelector`), and
the `<fx>` section of the song format. ~70 source files reference LADSPA.

## Decision Drivers

* Clear product profile: a plugin that produces audio and handles/emits MIDI
  events, not a host of other audio plugins.
* Cross-platform: LADSPA is Linux-only; it cannot anchor a multi-platform plugin.
* Shrink the surface the 2.0 plugin port must refactor (fewer files, fewer
  `get_instance()` sites, fewer build deps) **before** that work starts.
* Stop maintaining a sub-feature whose UX was already poor.

## Considered Options

1. **Deprecate LADSPA/LRDF/effect hosting in 1.2.7, remove it entirely in 2.0.**
2. Keep effect hosting in the standalone app; only exclude it from the plugin
   build.
3. Reimplement effects on a cross-platform engine.

## Decision Outcome

Chosen option: **deprecate in 1.2.7, remove entirely in 2.0 (option 1).**

* **1.2.7 (maintenance line):** mark LADSPA/LRDF effect hosting deprecated —
  release-notes notice and a user-facing hint — with no code removal. This is a
  communication step on the 1.2 branch, outside the 2.0 implementation plan.
* **2.0 (this port):** remove it completely:
  * delete `src/core/FX/` (`Effects`, `LadspaFX`) and the GUI
    `LadspaFXProperties` / `LadspaFXSelector` (+ `.ui`);
  * remove the `#ifdef H2CORE_HAVE_LADSPA` paths from `AudioEngine`
    (`setupLadspaFX`, `m_fLadspaTime`, the `processAudio` FX loop) and
    `Sampler.cpp`;
  * drop `Song::getEffects()` and the `<fx>` nodes from the song format (bump
    `formatVersion`; load older songs by **ignoring** `<fx>` without error, per
    the precedent of [ADR 0001](0001-introduce-formatVersion-to-xml-files.md));
  * remove the `WANT_LADSPA` / `WANT_LRDF` options, `FindLadspa`, the LRDF probe,
    and the status-list entries from CMake;
  * drop `ladspa-sdk` and `liblrdf-dev` (and `-DWANT_LRDF`/`-DWANT_LADSPA`) from
    the AppVeyor pipeline.

This supersedes the narrower statement in
[ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md) that LADSPA was merely
"dropped from the plugin build": it is removed from **all** builds, standalone
included.

Option 2 is rejected: keeping a Linux-only, poorly-maintained effect host in the
standalone app while the project's identity becomes "a plugin" muddies the profile
and keeps the maintenance/dependency cost. Option 3 (reimplement effects) is out
of scope and contrary to driver 1 — the host provides effects.

### Consequences

* **Users lose in-app effect inserts.** As a plugin they use the host's effects;
  as the standalone app they no longer have LADSPA inserts. Communicated via the
  1.2.7 deprecation notice and 2.0 release notes.
* The song format drops `<fx>`; old songs still load (the section is ignored).
* The codebase shrinks: ~70 files touched, a whole subsystem and two build deps
  gone — which **reduces** the de-singletoning and CI surface of the port and
  slightly eases the 1 GB CI cache pressure (proposal 0003 §10).
* Placement: best done **as a prerequisite** of the plugin port (before the
  de-singletoning), so later phases never refactor code that is being deleted;
  doing it at the very end is viable but wasteful.

## More Information

* `src/core/FX/`, `src/core/AudioEngine/AudioEngine.cpp` (`setupLadspaFX`,
  `processAudio`), `src/core/Sampler/Sampler.cpp:1828`, `Song::getEffects()`;
  CMake `WANT_LADSPA`/`WANT_LRDF` (`CMakeLists.txt:92`, `:115`); `.appveyor.yml`
  (`ladspa-sdk`, `liblrdf-dev`).
* Related: [ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md),
  [proposal 0004](/docs/proposals/0004-plugin-port-implementation-plan.md)
