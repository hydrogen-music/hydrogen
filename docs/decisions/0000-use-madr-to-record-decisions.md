---
status: accepted
date: 2024-07-26
deciders: phil (theGreatWhiteShark)
---

# AD: Use MADR to record decisions

## Context and Problem Statement

The Hydrogen project has grown quite old and already has seen a couple of
generations of different maintainers. However, up to now the only means of
passing information about design decisions - like changes in file formats or
UX - were comments in code or commit messages as well as Github issues. We need
a more sustainable way to record such knowledge.

## Decision Drivers

* Design decision should be more easy transcribed, shared, and challenged
* Lower entrance barrier for new contributors
* Handover between maintainers should be as smooth as possible

## Considered Options

1. [MADR](https://www.ozimmer.ch/practices/2022/11/22/MADRTemplatePrimer.html)
2. [Y-Statements](https://medium.com/olzzio/y-statements-10eb07b5a177)
3. [Doxygen](https://www.doxygen.nl/index.html)

## Decision Outcome

I decided to use **MADR** as it is very easy to use, lightweight, and works
splendid with Github's markdown rendering.

**Y-Statements** didn't looked that nice. Admittedly I never used them. But it
just feels not "free" enough.

**Doxygen** would have the benefit that we already _have_ an existing developer
documentation powered by that very tool. But it requires compiling, requires
people to introduce them to it, and does not provide an established way to
record decision (we would need to come up with a solution on our own).

### Consequences

* Future decisions will be documented in [docs/decisions](/docs/decisions)
* We will have yet another thing to maintain and care about. Decisions fallen
  into despair and not reflected by code anymore will do harm.


## More Information

* https://adr.github.io/
* https://adr.github.io/madr/
