---
status: accepted
date: 2026-01-08
deciders: phil (theGreatWhiteShark)
---

# AD: MIDI Channel Handling

## Context and Problem Statement

The MIDI Standard 1.0 defines 16 channel, Channel 1 to Channel 16. But their
numerical value in MIDI files and messages are zero-based and thus `0` to `15`.

This inconsistency was previously handled using zero-based values for MIDI
channels within Hydrogen and all corresponding files. At that time (<=1.2.X)
only two channel value were exposed to the user in the GUI: the per-instrument
MIDI output channel in the instrument editor and the input channel in the
preferences dialog. Both were incremented by one to match the values the user
expected.

But starting from version 2.0 there are a lot more places to display channel information. Especially, the listing of incoming and outgoing MIDI messages.

## Decision Drivers

* UI/UX should be as intuitive and compliant as possible.
* UI/UX as well as data formats should be as consistent as possible.
* Maintenance and development should be simple.

## Considered Options

1. As an intermediate solution user-facing channel information were made
   zero-based as well. But this was too confusing and broke UX consistency.
2. Special handling to convert all user-facing channels to be one-based has to
   be introduced in all affected widget.
3. All channels within Hydrogen will be one-based while their values will be
   converted to be zero-based at the boundary to our own files, Standard MIDI
   files, and MIDI drivers.

## Decision Outcome

I opted for the third option. The compatibility layer to files and MIDI drivers
is rather small and not touched that frequently. On the other hand, changing
_all_ occurrences of MIDI channel within the core and GUI would be a massive
task and would quickly diverge (e.g. logged MIDI channels not corresponding to
values displayed in the GUI). In addition, doing those conversions - especially
on all logged messages - might sum up to a significant contribution to our CPU
load.

To ease channel handling, the strong type `Midi::Channel` has been introduced.
This will make it more explicit to future maintainers and developers that some
kind of conversion might be necessary.

### Consequences

* All MIDI channel information within Hydrogen will be one-based starting with
  version 2.0.
* Displayed values won't match those stored in `.h2song` and `drumkit.xml` file
  as well as those in MIDI messages sent and stored. But this problem already
  existed in version <= 1.2.X and no one complained so far.
* Both file format and UI/UX will be consistent with prior versions of Hydrogen.
