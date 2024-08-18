---
status: accepted
date: 2024-08-16
deciders: phil (theGreatWhiteShark)
---

# AD: Drop `DrumkitComponent`s

## Context and Problem Statement

Since version `0.9.7` new drumkits and `drumkit.xml` files do contain
components. However, this feature is quite complex on code level, it's UI/UX
integration is close to non-existing, and it causes/caused a lot of issues (e.g.
[1], [2]). Most of the issues have been already addressed by the time of this
proposal but complexity and bad UX prevails. In all those 8 years I did not see
a single issue, log, or uploaded song/drumkit artifact which showed that this
feature was ever used.

Having independent components with corresponding layer in an instrument is good
thing and will be very useful to allow importing kits of other
formats/application at a latter point. That's not a bad idea. But we have to
streamline it and make it more easy to understand and use.

Also, integration has to be improved.

## Decision Drivers

Components could allow for much more realistic kits. But we have to improve
their UX by a margin to make them useful (and usable).

Also lesser complexity in our code base and XML artifacts makes maintaining and
contributing more easily.

## Considered Options

1. Keep the current component design and improve its integration
2. Drop `DrumkitComponent` and integrate the remaining `InstrumentComponent`

## Decision Outcome

I racked my brains for a while as to how I could improve the integration the
current component design in the `InstrumentEditor`. But `DrumkitComponent` just
won't fit in. So, after giving this some thought, I decided to drop it.

`InstrumentComponent` allow the user to have multiple distinct sets of
`InstrumentLayer`s (containing samples) covered by their own gain value within
an instrument. Using those you could e.g. use two microphones at two different
positions to create samples for your instrument and put those into two different
`InstrumentComponent`s. But right now each instrument has to have the same
amount of `InstrumentComponent`s and adding/removing one for one instrument will
alter all other instruments as well. This is totally weird and unexpected.

`DrumkitComponent`, on the other hand, do not carry any samples but just a
volume and mute/solo state. They are nothing really drumkit-related but aux
channels for components. The intention of introducing them was probably
something like "I used a direct mic and a room for each instrument. Now, I want
a slider in the mixer to control the overall mix in of the room". That's not a
bad but a way too concrete idea. It is nothing drumkit-related but a `Mixer`
thing that should rather be stored on song-level. But we do not have such a
`Mixer` and do not allow users to create aux channels. That's why it is very
hard to get the purpose of those "component" strips in `Mixer`.

By dropping `DrumkitComponent` we can put all the component handling into
`InstrumentEditor` and by decoupling `InstrumentComponent` we can cleanly
present a hierarchy `Instrument` > `InstrumentComponent` > `InstrumentLayer`
where changes in a child do not cause changes on the parent-level.

To keep the design clean and simple, `InstrumentComponent`s won't have a
dedicated id and their name is just a human-readable attribute but no
identifier. The component will, instead, be selected via its position in the
vector of the parent `Instrument`. Adding, removing, and moving will create a
different _copy_ of the parent `Instrument` in the redo/undo process and the
`Instrument` embedded in a `Note` for rendering in `Sampler` does not notice any
of those changes.

### Consequences

* `DrumkitComponent` will be dropped.
* `InstrumentComponent` will get a `m_sName` member and acts as the single
  representation of a component.
* All current GUI features are using `DrumkitComponent`. They have been ported
  to `InstrumentComponent`.
* `drumkit.xml` and `.h2song` file will be broken. But we already do this for
  `2.0` and introduce backward and forward compatibility (in the latest `1.2.4`
  release).
* Per-component drumkit export must die.
* For `2.0` `InstrumentEditor` will be redesigned to naturally incorporate
  `InstrumentComponent`.
* For `2.0` `SoundLibraryPanel` will be patched drag and drop insertion of
  components as well.

## More Information

[1] https://github.com/hydrogen-music/hydrogen/issues/1903
[2] https://github.com/hydrogen-music/hydrogen/issues/1901
