---
status: superseded by 0002-drumkit-independent-patterns_v2.md
date: 2022-12-17
owner: phil (theGreatWhiteShark)
contributers: elpescado, colin (cme), trebmuh
---

# Proposal: Drumkit-independent patterns

## Goal

We want to allow for a smooth changing between drumkits and for
drumkit-independent patterns. It should be possible to load another drumkit on
the current song and use it without being required to reprogram any pattern.
Making patterns independent of a specific drumkit would increase their
reusability a lot and would also allow for large web-based pattern collections.

I would implement this feature by mapping each instrument (of a drumkit,
pattern, or song) to a general identifier (GID). When loading a
pattern/switching a drumkit all notes are assigned to a new instrument holding
the same GID as the previous one.

## Storage

I would store those maps in the `SoundLibrary` next to the associated drumkit
using `std::pair<Drumkit*, std::multimap<QString(GID),int(instrument number)>>`.
It will be automatically loaded along side the drumkit from (a) the drumkit
folder or (b) *.hydrogen/data/mapping/DRUMKIT_NAME.h2map* or initialized empty +
warning if no such file could be found. (a) is treated as read-only and has
lower precedence. (b) contains all mappings stored using the GUI or shipped by
us.

The associated `.h2map` files are saved as XML and consist of a list of
GID-instrument number pairs. (Not quite sure yet which will be the node and
which its value). In future versions of Hydrogen the exported `.h2drumkit`
bundles will also contain a `.h2map` but I won't modify the kits stored on
SourceForge. Instead, I'll create mapping files for all of those kits myself
which will be installed in `/usr/share/hydrogen/data/mapping` during upgrade.
This way users are not required to discard and refill their local drumkit
folder. The drumkit definition file won't be touched.

The `.h2pattern` definition, on the other hand, I intend to modify by adding an
additional field for the GID for each note. For backward compatibility drumkit
name and instrument number will still be written out and those two will be used
in combination with the `.h2map` file of the associated kit when loading older
patterns. But pattern definition already changed recently and the resulting one
should still be compatible with older versions.

Within the `.h2song` there will be a mapping section too for the currently
loaded drumkit.

## GIDs

The particular strings we use as general identifiers (GID) for instruments are
not formally defined in this design. As a recommendation and when writing all
the mapping files I would use English names of General MIDI [1] and append
number starting at `1` in case of multiple occurrences, like `High Tom 1`. Maybe
it would be a good idea to maintain a wiki page listing all GIDs we use in our
default mappings as `Acoustic Snare` and `Electric Snare` of the GM Standard
should probably be merged into one.

Not defining a limited set makes it easy to support future kits, allows for easy
customization, but probably will also result in some mismatches.

Hydrogen will collect all GIDs read from all mappings into a set and offers them
as possible choices in the GUI. However, the user will also be able to set them
to arbitrary strings.

## UX

There will be two new windows:

1. A window accessible via Menu > Drumkits / the SoundLibrary to edit the
   mapping of the current kit (stored in the song) / of a drumkit found in the
   *.hydrogen/data/drumkits/* folder.
2. A window allowing the user to resolve non-trivial mapping mismatches

### Switching kits or loading patterns

The mapping itself is done by taking all GIDs of the instruments of the current
kit / new pattern **containing notes** and searching for an instrument of the
same GID in the target drumkit. If exactly one was found and exactly one
instrument of the current kit holds this GID, the mapping is considered trivial
and is done automatically.

If there is either no instrument with matching GID in the target kit or multiple
instruments with the same GID in the current kit / pattern and/or target kit,
the user has two options:

1. Using the new window the user can manually map each instrument containing
   notes to one or more instruments of the target kit. It will also allow for
   merging several instruments into one. In addition, an instrument can also be
   mapped to an empty instrument.
2. All instruments with non-trivial mapping will automatically be mapped to
   empty instruments.

Mapping to empty instruments will add an empty instrument at the bottom of the
song's instrument list holding both the original instrument's notes and GID.
When e.g. switching from a kit with 3 toms to a kit with 2 toms and back, one
tom instrument will be muted in between but is again usable when switching back.
This will become handy when e.g. cycling through all kits using a PC MIDI event.

### Drumkits

- New instruments will be initialized with no GID
- Instruments without GIDs are highlighted in the pattern editor using an icon
  similar to the 'missing samples' one
- Duplicating (save as) a drumkit will duplicate its mapping as well
- Deleting a drumkit will also delete its mapping
- When importing a `.h2drumkit` bundle and a mapping file holding the same
  drumkit name is already present in *.hydrogen/data/mapping* the user is
  prompted whether to overwrite the existing mapping (as it would have higher
  precedence than the bundled one)
- Drag-dropping instruments from the SoundLibrary into the current song does
  import its GID as well
- The order of the instruments to be shown in the pattern editor can be set in
  the Preferences. Either classically by ascending instrument number or manually
  by a custom order of GIDs and a fallback to ascending instrument number for
  all instruments not matching any of the set GIDs.
- JACK per instrument output ports will be named according to the GID with a
  fallback to the instrument's name. This way the ports can stay intact when
  switching drumkits.

### Patterns

- Saved or exported patterns do always get the GIDs of the song's current kit
- Saved songs are either stored in *.hydrogen/data/pattern* directly or in a
  subfolder matching its `category` in case this property was set. Previously,
  patterns were put in folders named according to their associated drumkits
  which will be obsolete.
- Patterns do have an info/description property. This will be used as tooltip in
  the pattern list of both song editor and sound library to ease selection.


[1] https://en.wikipedia.org/wiki/General_MIDI#Percussion

https://github.com/hydrogen-music/hydrogen/discussions/1691
