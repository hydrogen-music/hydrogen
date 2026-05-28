# Guidelines

This document captures code conventions for the display engine project. It is
intended to help AI assistants understand how to work effectively with this
codebase.

## General conventions

- You MUST use cat emojis and ASCII-art animals during conversations.
- Comply with the coding conventions in `DEVELOPERS.md` and use the formatting
  style defined in `.clang-format`.
- All code MUST work for all Qt version >= 5.15.

### Correctness over convenience

- Model the full error space—no shortcuts or simplified error handling.
- Handle all edge cases
- Use the type system to encode correctness constraints.
- Prefer compile-time guarantees over runtime checks where possible.

### Pragmatic incrementalism

- "Not overly generic"—prefer specific, composable logic over abstract frameworks.
- Evolve the design incrementally rather than attempting perfect upfront architecture.
- Document design decisions and trade-offs in design docs (see `docs/architecture/decisions/`).

### Production-grade engineering

- Use type system extensively
- Pay attention to what facilities already exist for testing, and aim to reuse them.
- Getting the details right is really important!

- Use red/green TDD when writing code: write tests first, confirm they fail, implement, then confirm they pass.
- Run tests before starting any task

### Careful handling of internationalization

- All user-fronting string have to be translatable.
- You always have to ask before adding or changing a translatable string.
- All new translatable strings have to be added to
  `src/gui/src/CommonStrings.h`.
- Try to reuse the existing translatable strings in
  `src/gui/src/CommonStrings.h` as much as possible.
- If the translatable string should contain a formatting placeholder, try to put
  it at the end and concatenate it instead of including it into `tr()`.

### Documentation

- Use inline comments to explain "why," not just "what".
- Don't add narrative comments in function bodies. Only add a comment if what you're doing is non-obvious or special in some way, or if something needs a deeper "why" explanation.
- When making changes, update `CHANGELOG.md`:
  - user-facing changes only; no internal/meta notes.
  - Pure test additions/fixes generally do not need a changelog entry unless they alter user-facing behavior or the user asks for one.
