---
status: accepted
date: 2024-08-11
deciders: phil (theGreatWhiteShark)
---

# AD: Introduce `userVersion` to XML files

## Context and Problem Statement

None of our XML files allows the user to introduce some proper way of
versioning. In the `.h2drumkit` files we host on SourceForge this is done by
adding a "_VX" suffix to file and kit name. But this does not allow for an
automated way of checking for a newer version.

## Decision Drivers

Since starting with v2.0 of Hydrogen patterns and songs are independent of
drumkits and it finally makes sense to share them and e.g. provide a central
repo. There are no plans to do so (yet) but since all XML definitions are
touched anyway, this is a good time to introduce a more solid base for such
endeavor. An upgrade path to the latest version of a provided artifact would be
of much help.

## Considered Options

1. No versioning
2. Integer version
2. String version

## Decision Outcome

It feels wrong (at least a little) to introduce a feature that is currently not required. But in the long run versioning is most probably a lot more helpful than no versioning (and we do not break anything).

I decided to go for integer versions as they are easy to understand and come
with an inherent way of comparison. Strings would be allow for a more flexible
way, like including topic, style, using semantic versioning, but since we can
not compare them in a meaningful way, this would be effectively just another
"name" element.

I also decided to include `userVersion` into `.h2song`, `.h2pattern`, and
`drumkit.xml` file but not into `.h2playlist`, `hydrogen.conf`, and `.h2map`
files. `hydrogen.conf` and `.h2map` are somewhat internal and not user-fronted
files. These are more for us to ship and not for the user to share.
`.h2playlist` is more tricky. I think there is less use in sharing those. But
mostly it was an UX-driven decision to not include it there. For songs, kits,
and patterns we already provide a properties dialog. But not for the playlist
and it would be awkward to introduce one just to set the version.

The name `userVersion` instead of plain `version` is required as there is
already `version` element present in both `.h2song` and `hydrogen.conf` file
indicating the semantic version of the Hydrogen installation the file was
created with.

### Consequences

* `userVersion` elements will be introduced to `.h2song`, `.h2pattern`, and
  `drumkit.xml` files and the corresponding XSD files will be tweaked.
* The initial version will be set to `0` and higher values will indicate newer
  versions.
* `PatternPropertiesDialog`, `SongPropertiesDialog`, and
  `DrumkitPropertiesDialog` have to be extended with a corresponding spin box.
