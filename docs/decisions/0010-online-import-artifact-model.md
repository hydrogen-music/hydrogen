---
status: proposed
date: 2026-05-20
deciders: phil (theGreatWhiteShark)
---

# AD: Online import artifact model and index parsing

## Context and Problem Statement

The new online import system consumes JSON index files (specified in ADR
`0002-index-file-format.md` of the `hydrogen-index` project) which contain
metadata for patterns, songs, and drumkits. We need an in-memory representation
of this data that:

- Can be built without downloading any actual artifacts.
- Supports querying by type (pattern/song/drumkit), searching by name and tags,
  and determining whether an artifact is already installed locally.
- Is usable from both the GUI dialog and CLI tool.

How do we model the index content and where does the parsing logic live?

## Decision Drivers

- The model must faithfully represent the JSON schema from ADR 0002 of
  `hydrogen-index`.
- Parsing must handle malformed/incomplete data gracefully (partial load is
  acceptable).
- The model must be lightweight — index files can contain 300+ entries.
- Must be testable with local fixture files (`src/tests/data/onlineImport/`).
- Must support determining whether an artifact is already present locally and
  whether it differs from the remote version.

## Considered Options

1. **Flat QVariantMap / QJsonObject** — Keep parsed JSON as-is, query
   dynamically.
2. **Typed C++ structs** — Define `OnlineArtifact`, `OnlineIndex` value types.
3. **Reuse existing SoundLibraryInfo** — Extend the existing
   `SoundLibraryDatabase` info objects.

## Decision Outcome

Chosen option: **2 — Typed C++ structs**.

**Rationale:**

- Type safety catches schema mismatches at compile time rather than runtime.
- Explicit fields make the code self-documenting and IDE-friendly.
- The existing `SoundLibraryInfo` objects (option 3) carry too much
  local-filesystem context; conflating remote metadata with local state would
  create confusion.
- Flat QVariantMap (option 1) is fragile, hard to refactor, and obscures the
  data's structure.

### Data Model

```cpp
namespace H2Core {

// Shared metadata present in every artifact block.
struct OnlineArtifact {
    enum class Type { Pattern, Song, Drumkit };

    Type type;
    QString sName;
    QUrl url;
    QString sHash;          // sha256 hex string
    QString sAuthor;
    QString sDescription;
    int nVersion;           // user-set revision
    int nFormatVersion;     // Hydrogen format revision
    QStringList tags;
    qint64 size;           // bytes
    QString sLicense;

    // Pattern-specific
    int nNotes;                     // -1 if N/A

    // Song-specific
    int nPatternCount;              // -1 if N/A (named to avoid clash with top-level)

    // Drumkit-specific
    int nInstruments;               // -1 if N/A
    int nComponents;                // -1 if N/A
    int nSamples;                   // -1 if N/A

    // Shared by patterns AND drumkits (used for instrument↔pattern mapping)
    QStringList instrumentTypes;   // empty if N/A

    // Computed locally (not from JSON)
    enum class LocalStatus { NotInstalled, Installed, Modified, UpdateAvailable };
    LocalStatus localStatus;
};

struct OnlineIndex {
    QString sVersion;         // hydrogen-index version
    QString sCreated;         // ISO timestamp
    QUrl sourceUrl;          // URL this index was fetched from

    QVector<OnlineArtifact> patterns;
    QVector<OnlineArtifact> songs;
    QVector<OnlineArtifact> drumkits;

    // Optional top-level hash for integrity check
    QString sHash;
};

} // namespace H2Core
```

### Parsing

- `OnlineImporter::parseIndex(const QByteArray& jsonData, const QUrl& sourceUrl)`
  returns an `OnlineIndex`.
- Uses `QJsonDocument` / `QJsonObject` / `QJsonArray` (available in Qt::Core,
  no GUI dependency).
- Unknown fields are silently ignored (forward compatibility with future
  `hydrogen-index` versions).
- Missing required fields (`name`, `url`, `hash`) cause the individual artifact
  to be skipped with a warning log, not a hard failure.

### Local Status Determination

`OnlineImporter::resolveLocalStatus(OnlineArtifact& artifact)`:

In production, local status is resolved via `SoundLibraryDatabase` which
maintains an up-to-date view of all installed artifacts (patterns, songs,
drumkits). Each `SoundLibraryInfo` entry carries a `m_nVersion` field parsed
from the artifact's XML on disk.

1. Look up the artifact by name and `Filesystem::Context::User` in
   `SoundLibraryDatabase`.
2. If not found → `NotInstalled`.
3. **For patterns and songs** (single files):
   - Hash the local file (sha256) and compare against the index hash.
   - If hashes match → `Installed`.
   - If hashes differ and remote `version` > local version → `UpdateAvailable`.
   - If hashes differ and remote `version` ≤ local version → `Modified`
     (the user has edited the local copy).
4. **For drumkits** (installed as directories):
   - Drumkits are extracted after download, so the original `.h2drumkit` archive
     is not available for hash comparison.
   - Compare the `version` field from the index against the version stored in
     `SoundLibraryInfo` (originally read from `drumkit.xml`).
   - If remote version > local → `UpdateAvailable`.
   - Otherwise → `Installed`.
   - Hash verification for drumkits only applies at download time (before
     extraction), not for local status resolution.

This runs **without** downloading the remote artifact. The sha256 of the remote
is taken from the index JSON; only local files are hashed (for patterns/songs).

A `setLocalSearchPath()` test helper is retained for unit tests that need to
bypass `SoundLibraryDatabase` with temporary directories.

## Consequences

- New files: `src/core/OnlineImporter.h`, `src/core/OnlineImporter.cpp`
- Struct definitions can live in `OnlineImporter.h` or a separate
  `src/core/Basics/OnlineArtifact.h` if the header grows large.
- The test fixture `src/tests/data/onlineImport/index.json` serves as the
  golden reference for parser tests.
- The GUI layer receives fully typed, pre-parsed data — no JSON handling in
  dialog code.
