---
status: accepted
date: 2024-08-12
deciders: phil (theGreatWhiteShark)
---

# AD: Move `author` and `license` into `<pattern>` definition

## Context and Problem Statement

Since version 1.0.0-beta2 we the `author` and `license` properties in each
`.h2pattern` file. But not in the actual `<pattern>` element - the one stored
in/imported into a `.h2song` - but in the surrounding `<drumkit_pattern>`
element. This causes both license and author being lost ones the pattern is
imported into a song. In addition, the license/author and only be set in the
export dialog and can only be accessed in the by reading the plain XML file and
not from within Hydrogen.

## Decision Drivers

Since starting with v2.0 of Hydrogen patterns and songs are independent of
drumkits and it finally makes sense to share them and e.g. provide a central
repo. For this to work we need to ensure ownership and copyright of the patterns
can be accessed from within Hydrogen and is not lost.

## Considered Options

1. Moving `author` and `license` from `<drumkit_pattern>` into `<pattern>`
2. Writing `<drumkit_pattern>` in `.h2song` files instead of `<pattern>`

## Decision Outcome

Since the pattern XSD file was touched for 2.0 anyway, there is no harm in
moving `author` and `license` into `<pattern>`. Storing `<drumkit_pattern>` in a
`.h2song`, however, would be more work and more difficult to handle.

The `drumkit_name` properties remains in `drumkit_pattern` to ease forward
compatibility between the latest 1.2.X release and 2.0.

All legacy patterns - both `.h2pattern` and those contained in `.h2song` files -
will fall back to undefined license in case non is explicitly set. Author and
license of a song will not automatically applied to all legacy patterns. We do
not assume any knowledge about ownership. Instead, all new patterns will inherit author and license of a song.

### Consequences

* `author` and `license` will moved from `<drumkit_pattern>` to `<pattern>` in
  `drumkit_pattern.xsd`.
* `Pattern::m_sAuthor` and `Pattern::m_license` will default to
  `Song::m_sAuthor` and `Song::m_license` but are kept and honored when set to a
  different value e.g. during pattern import.
* A new legacy loading function is required to load pattern created in v1.X.X.
* `PatternPropertiesDialog` will get additional elements to allow setting author
  and license on a per-pattern base.
* `SongPropertiesDialog` will get a table of all contained pattern licenses
  similar to the contained sample licenses in `DrumkitPropertiesDialog`.
  Mismatching licenses will be highlighted and there will be warning dialog when
  saving with a GPL-licensed pattern contained (similar to drumkit export with a
  GPL-licensed sample).
