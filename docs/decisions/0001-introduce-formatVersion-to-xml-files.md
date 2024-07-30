---
status: accepted
date: 2024-07-30
deciders: phil (theGreatWhiteShark)
---

# AD: Introduce `formatVersion` to XML files

## Context and Problem Statement

Our XML files (`.h2song`, `.h2pattern`, `.h2playlist`, `drumkit.xml`,
`hydrogen.conf` and since v2.0 `.h2map`) have already changed quite a bit
throughout the history of Hydrogen. Maximum backward compatibility is an
explicit goal but it is sometimes not easy as the legacy format is usually
detected by educated guesses based on the presence of this or that XML element.
We need a different approach. One that scales.

## Decision Drivers

* Easier and more robust backward (and occasionally forward) compatibility
* Allow to correlate an artifact with a particular version of Hydrogen
  (especially for files being changed often while not being covered by an XSD
  files, e.g. `.h2song` or `hydrogen.conf`, this is often hard)
* XML changes must be done intentional (pipeline should point of changes)

## Considered Options

1. Adopt version identifier for all XML files
2. Continue due things without it

## Decision Outcome

Introducing a new element in all XML files (called `formatVersion` in order to
distinguish it from an user-defined `version`) breaks the current format and XSD
validity. This in general is Bad (with a capital "B"). But doing so for v2.0 is
probably okay since both `drumkit.xml` and `.h2pattern` did change anyway with
the introduction of instrument types (see proposal
[0002-drumkit-independent-patterns_v2.md](../proposals/0002-drumkit-independent-patterns_v2.md)).
This leaves only `.h2playlist` files (as both `.h2song` and `hydrogen.conf` are
not covered by a XSD file and `.h2map` are introduced in v2.0 for the first
time). That's a price we can pay for a scalable versioning of our artifacts.

### Consequences

* The initial `formatVersion` introduced in the first beta version Hydrogen
  v2.0.0 will be set to `1` and in the actual v2.0.0 release to `2`. Afterwards
  any change in the file format will cause the number to be incremented for that
  particular XML file.
* Integration tests for _all_ XML files must be written which fail as soon as
  their format changes (in order to make breaking it an explicit and intentional
  action).
* Forward compatibility has to be assured by adjusting the XSD files of the
  latest 1.2.X release.
