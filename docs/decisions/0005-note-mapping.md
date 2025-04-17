---
status: accepted
date: 2025-04-15
deciders: phil (theGreatWhiteShark)
---

# AD: Note mapping

## Context and Problem Statement

In version `2.0` we added the `type` property to both `Note` and `Instrument` in
order seamlessly map all notes when loading a different drumkit and finally
allow for drumkit-independent patterns.

But while mapping a note of type X to an instrument of type X is straight
forward, what to do with instruments holding no or an empty type? After all this
affects both custom kits created with Hydrogen prior to 2.0 as well as any new
drumkit and instrument created.

For historical context: prior to `2.0` notes have been mapped based on their
contained instrument ID. But since the instrument IDs of a drumkit were
rewritten on loading based on the instrument of the previous kit, notes were
actually mapped order-based. This means all notes created for the third
instrument of a kit will be assigned to the third instrument of the new kit when
loading a different another one from the sound library.

## Decision Drivers

It shouldn't be a pain but the simpler the better. Creating new
instruments/drumkits or importing old ones is probably something very seldom
done by the average user. Better to have a simple and easy to remember workflow
than sophisticated automation magic you have to get used to again every single
time.

It also needs to be easily maintainable and to support redo/undo in a resilient
way.

## Considered Options

1. Instrument ID-based mapping with ID resetting of unmapped notes.
2. Context-based switch between id-based and order-based mapping.
3. No automatic mapping between typed and id-only notes.

## Decision Outcome

The 1. option was used as the initial implementation. It included automated
mapping between typed and untyped drumkits based on their instrument IDs for
pattern load and based on their instrument order for drumkit switching. That
felt convenient when switching kits but when creating new drumkits it proved to
be very annoying. Image you use the GMRockKit and have notes present in the first three rows (Kick, Stick, Snare). When creating a new kit, all notes holding the `Kick` type will be mapped to the new empty instrument since it also holds an instrument id of `0`. Adding a new instrument or one for the `Snare` type will also cause the notes holding the `Stick` type to be mapped to the new instrument, as it hold id `1`. However, those instruments won't have types themselves and from the view of the pattern editor sidebar it seems those types have been lost. That felt totally wrong.

To overcome these problems I tried option 2. of enforcing the mapping to be
`Note::MapBy::[None, Id, Order]` and explicitly providing them on drumkit
interactions in the GUI. But things just got way more complicated and especially undo/redo of initially assigning a type to an instrument and all associated notes showed to be difficult.

Instead, I decided to make the automated mapping a lot more simple and adopt
option 3. Now, automated mapping will neither remove nor add a type to a note.
If a type is present, the note will be mapped to an instrument with the same
type, If note, it will be mapped classically based on the order of the
instrument corresponding to its instrument id in the current drumkit. The
initial assignment of a type to a note has to be done explicitly somewhere else.

### Consequences

* Switching from a typed kit to an unmapped one or vice versa will result in
  _all_ notes being unmapped and appended in the pattern editor. The recommended
  way to fix this is to assign types to all kits.
* If a user wants to associate a note of empty type with a typed instrument, she
  can move or copy the note in the corresponding row.
* If a user wants to associate a typed note with an untyped instrument, she can
  either move or copy the note in the corresponding row or assign the same type
  to the instrument to merge both rows.
* Initial type setting of a note has to be done explicitly in the GUI using the undo/redo action for note property editing.
* On loading a legacy (untyped) pattern from a `.h2pattern` or `.h2song` file,
  all notes have to be typed explicitly using the types of the associated
  drumkit.
