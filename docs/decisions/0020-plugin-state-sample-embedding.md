---
status: accepted
date: 2026-06-08
deciders: pm
---

# AD: Embed samples as a libarchive bundle decoded in memory

## Context and Problem Statement

When self-contained, the artifact carries the song plus all referenced sample
audio and reconstructs on load without depending on installed drumkits. This is
the default plugin state ([ADR 0017](0017-embed-song-in-plugin-state.md)) **and**
the new standalone `.h2project` format ([ADR 0025](0025-h2project-self-contained-format.md));
this ADR fixes the single mechanism both use. The current code gets us partway and
no further:

* The song already serialises its drumkit inline as `<drumkit_info>`
  (`bSongKit=true`, `Song.cpp`), but stores **only sample paths/filenames, never
  the audio bytes**.
* `Sample::load()` (`Sample.cpp:215`) reads a **file path only** via
  `sf_open(getFilePath())` (libsndfile). **There is no in-memory load path.**
* Hydrogen already bundles drumkits as **libarchive** tar(.gz): write via
  `Drumkit::exportTo`, read via `Drumkit::install` (`Drumkit.cpp:737`).

So to be self-contained we must carry binary audio, but the loader only reads
files. Two further constraints: the **engine** process (not the editor) owns
playback and must do the loading; and a plugin should **not depend on writable
temp space** — hosts may sandbox or otherwise restrict filesystem access, and
per-instance temp directories add lifecycle and cleanup burden under
multi-instance use.

## Decision Drivers

* Self-contained, machine-portable state.
* Avoid dependence on writable temp space and per-instance temp-dir lifecycle.
* Reuse the existing libarchive + XML machinery; keep audio binary (not base64).
* Save/load runs on the main/worker thread in all formats — never the audio
  thread.

## Considered Options

1. **Archive bundle + in-memory sample loading** (libsndfile virtual I/O).
2. Archive bundle + temp-directory extraction, reusing the file-based loader.
3. Base64-inlined samples inside the drumkit XML.

## Decision Outcome

Chosen option: **archive bundle + in-memory loading (option 1).**

This mechanism **is** the `.h2project` codec ([ADR 0025](0025-h2project-self-contained-format.md)):
the same reader/writer serves the standalone `.h2project` menu action and the
plugin's default sample-embedding state.

* The bundle is a **libarchive bundle** containing the song XML, the unique
  sample files, and the kit image — reusing the existing `.h2drumkit`-style
  packing.
* On load, samples are **decoded directly from memory** via libsndfile **virtual
  I/O** (`sf_open_virtual` with read/seek callbacks over the in-memory buffer).
  This requires extending `Sample` with a memory-backed load path alongside the
  existing file path one. No temp files are written — **sandbox-safe** and free
  of any per-instance temp-dir lifecycle.
* Samples are **de-duplicated by content hash** when packing, since the playback
  track and multi-layer instruments can otherwise repeat the same file.

Temp-directory extraction (option 2) is rejected as the v1 mechanism because it
depends on writable temp space that sandboxed hosts may deny, and it adds
per-instance temp-dir lifecycle management; it remains a viable fallback if the
in-memory loader slips. Base64-in-XML (option 3) is rejected: ~33 % size bloat,
slow parsing, heavy memory, and it *still* needs a temp file or in-memory loader.

### Consequences

* `Sample` gains an in-memory decode path (libsndfile virtual I/O callbacks). The
  existing file-based path stays for the standalone app.
* State save assembles the bundle (collect unique samples, hash-dedup, archive +
  XML); state load reads the archive into memory and loads each sample from its
  buffer.
* Blobs can be large for big multi-sample kits — accepted by
  [ADR 0017](0017-embed-song-in-plugin-state.md); all of this happens off the
  audio thread.
* The same bundle is format-neutral, so LV2/CLAP/VST3 share one state codec — and
  the standalone `.h2project` reuses it too ([ADR 0025](0025-h2project-self-contained-format.md)).
* The in-memory decode also matters for the standalone `.h2project`: a large
  bundle can be opened without extracting gigabytes of samples to disk first.

## More Information

* `src/core/Basics/Sample.cpp:215` (loader), `src/core/Basics/Drumkit.cpp:737`
  (libarchive read), `Drumkit::exportTo` (libarchive write), `Song.cpp`
  (`<drumkit_info>` embedding).
* libsndfile virtual I/O: `sf_open_virtual`.
* Related: [ADR 0017](0017-embed-song-in-plugin-state.md),
  [ADR 0019](0019-plugin-output-bus-layout.md),
  [ADR 0025](0025-h2project-self-contained-format.md),
  [proposal 0003](/docs/proposals/0003-hydrogen-as-an-audio-plugin.md)
