---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: No host-automatable plugin parameters in v1

## Context and Problem Statement

Plugin formats let hosts automate exposed parameters (CLAP params, VST3
parameters, LV2 control ports). We must decide which Hydrogen controls to expose
as host-automatable parameters in the first plugin version.

Hydrogen, however, has **almost no automation surface of its own today** — there
is effectively only overall (master) volume/velocity; instrument and mixer
controls are driven through `CoreActionController` and the GUI rather than as a
parameter set designed for automation.

## Decision Drivers

* Don't invent a large automation parameter model the application itself does not
  yet have.
* Keep v1 scope focused on the structural work (de-singletoning, transport,
  out-of-process UI, state) where the real risk lies.
* Avoid committing to a parameter layout that would later have to change as
  Hydrogen grows real automation.

## Considered Options

1. **No host-automatable parameters in v1.**
2. Expose a minimal set now (e.g. master volume, per-strip volume/pan/mute).
3. Build a full parameter model mirroring mixer + instrument controls.

## Decision Outcome

Chosen option: **no host-automatable parameters in v1.**

The plugin is driven by host transport ([ADR 0013](0013-provide-hydrogen-as-an-audio-plugin.md)),
MIDI notes, and the editor; it does not expose an automation parameter set.
Master volume and similar controls remain reachable through the editor and MIDI,
not as automatable plugin parameters.

This is revisited when Hydrogen itself gains a meaningful automation model;
designing the plugin parameter layout then will reflect the application's real
control surface rather than a guess.

### Consequences

* DAW automation lanes for Hydrogen controls are not available in v1.
* No parameter-id stability contract to maintain yet — a future automation model
  can be designed cleanly.
* The plugin formats' parameter extensions are simply left unpopulated for now.

## More Information

* `src/core/CoreActionController.h` (the command surface the GUI/MIDI use today).
* Related: [proposal 0003 §8](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
