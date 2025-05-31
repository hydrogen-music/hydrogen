---
status: accepted
date: 2025-05-23
deciders: phil (theGreatWhiteShark)
---

# AD: Introduce BaseEditor

## Context and Problem Statement

Originally, all our editors - `DrumPatternEditor`, `PianoRollEditor`,
`NotePropertiesRuler`, `SongEditor`, `AutomationPathView`, as well as pan and
volume automation in `TargetWaveDisplay` within the `SampleEditor` - had their
unique code base and their individual UX. Each part felt a little different.

Since version `1.1.0` Hydrogen features a common selection handling in song and
all pattern editors. It feels much more coherent this way. In the preparation
for the `2.0` all the editors in the pattern editor region have been align in
both UI and UX. But it still does not feel like one consistent application.

For `2.0` the UX of all remaining editors will be aligned as well. Interacting
with a pattern square in `SongEditor` should feel the same as interacting with
notes in `DrumPatternEditor`. In the same vain our horizontal editors should
behave the same, regardless of whether the user is interacting with a note in
`NotePropertiesRuler`, an automation node in `AutomationPathView`, or an
envelope point in `TargetWaveDisplay`.

## Decision Drivers

* UX should be as intuitive as possible.
* UX changes with time and Hydrogen could keep up (e.g. optimized for touch
  interaction).
* Maintenance should be as simple.

## Considered Options

1. We could take the UX of the pattern editor and implement similar code in all
   other editors.
2. We introduce a common base class, like `Editor::Base`.

## Decision Outcome

I tend towards 2. It's a lot of work. But implementing and testing 1. would be
an almost equivalent pile of work. In addition, codifying the requirement of a
shared UX makes maintaining more easy. Else, it would be very easy to forget to
apply a patch to a particular editor.

While at it, `AutomationPathView` and `TargetWaveDisplay` should be ported to a
shared code base. After all, automation of pan and volume is just a special case
of the more general automation view. The latter should also be made ready to
handle various automation targets while being touched. Just the overall
velocity - as up to `1.2.X` - does not really justify the presence of this
widget.

What should be part of the common base?

- Keyboard arrow movement (plus modifiers)
- Selection via keyboard and mouse
- Highlighting of selected elements
- Lasso rendering
- Hovering of elements via keyboard and mouse as well as a shared cursor margins.
- Mouse and keyboard based moving of selected elements
- Left-click moving of single elements (selected or not)
- Right-click popup menu (if applicable)
- Right-click drawing


### Consequences

* Based on the recent work in `PatternEditor` a common base class called
  `Editor::Base` - itself a child class of `Selection` - will be split off.
* First, all pattern editors will be ported to the new base class.
* Second, the `SongEditor` will be ported.
* The automation code base will be reviewed and ported to `Editor::Base`.
* `TargetWaveDisplay` code base will be dropped in favor for inheriting a shared
  automation code.
