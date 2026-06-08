---
status: proposed
date: 2026-05-20
deciders: phil (theGreatWhiteShark)
---

# AD: Network access architecture for the core module

## Context and Problem Statement

The online import functionality currently resides entirely in the GUI layer
(`src/gui/src/Rack/SoundLibrary/SoundLibraryOnlineImportDialog.cpp` and
`src/gui/src/Widgets/DownloadWidget.cpp`). We want to move the download logic
into `src/core/` so it can be reused by the CLI tool and covered by the existing
CppUnit test infrastructure.

The core module cannot assume that `QApplication` (the GUI event loop) is
present. However, `QNetworkAccessManager` — Qt's HTTP client — is inherently
asynchronous and requires event processing to function.

How do we provide reliable HTTP downloading in `src/core/` using Qt >= 5.15
without depending on the GUI?

## Decision Drivers

- Reuse by both the GUI application and the CLI tool (`src/cli/`).
- Testable under the existing CppUnit harness (`src/tests/`).
- Must not introduce new external dependencies beyond what Qt provides.
- Must work without calling `QCoreApplication::exec()` (i.e. the global event
  loop need not be running).
- Must be compatible with Qt >= 5.15.

## Considered Options

1. **libcurl** — Use libcurl for all HTTP operations. No Qt dependency for
   networking at all.
2. **QNetworkAccessManager with local QEventLoop** — Require only that a
   `QCoreApplication` instance has been created (which is already a documented
   precondition of `Filesystem.cpp` in core). Use a scoped `QEventLoop` to
   drive the async network operations synchronously at the call site.
3. **Abstract interface with pluggable backends** — Define an
   `INetworkClient` interface; provide a Qt implementation for the app and
   a minimal implementation for edge cases.

## Decision Outcome

Chosen option: **2 — QNetworkAccessManager with local QEventLoop**.

**Rationale:**

- `QCoreApplication` is already required by the core module (`Filesystem.cpp`
  asserts it). Both the CLI tool (`src/cli/main.cpp`) and the test harness
  (`src/tests/main.cpp`) already instantiate it. This is a lightweight, non-GUI
  object.
- A local `QEventLoop` processes only the signals needed to complete a single
  HTTP transaction. It does **not** require the global `exec()` loop to be
  running, making it safe for CLI, tests, and being called from arbitrary GUI
  threads.
- No new external dependencies are introduced; `Qt::Network` is already linked
  by the test target.
- The existing pattern is proven: `Parser.cpp` already conditionally creates
  `QCoreApplication` when none is present.

**Option 1 was rejected** because:
- It introduces a new C dependency and complicates the build.
- It duplicates proxy handling, SSL, and redirect logic already provided by Qt.
- Qt::Network is already a transitive dependency of the project.

**Option 3 was rejected** because:
- It adds indirection and complexity (YAGNI) when all real callers already
  satisfy the `QCoreApplication` precondition.
- Testing the abstraction itself becomes an additional burden.

### Implementation Details

The new `OnlineImporter` class (in `src/core/OnlineImporter.{h,cpp}`) will:

1. Create `QNetworkAccessManager` instances **on the stack** within each
   download method invocation. This avoids thread-affinity issues entirely
   (consistent with `AppveyorRestClient.cpp`'s approach).
2. Expose a **synchronous** download method for CLI/test use:
   ```cpp
   QByteArray downloadBlocking(const QUrl& url, int timeoutMs = 30000,
                               QString* pError = nullptr);
   ```
   Internally this spins a local `QEventLoop`, connects `QNetworkReply::finished`
   to `QEventLoop::quit`, and uses a `QTimer` for the timeout. Returns the
   response body.
   **WARNING:** This method MUST NOT be called from the main GUI thread — the
   local event loop would cause reentrancy (processing input/paint events).
   It is designed for CLI tools and unit tests only.
3. Expose an **asynchronous** download method for GUI use:
   ```cpp
   void downloadAsync(const QUrl& url);
   // Emits: downloadProgress(qint64 done, qint64 total)
   // Emits: downloadFinished(const QByteArray& data, const QString& error)
   ```
   The GUI's `OnlineImportDialog` connects to these signals for progress bars.
   This method is safe from any thread.
4. Handle redirects via `QNetworkRequest::RedirectPolicyAttribute` set to
   `QNetworkRequest::NoLessSafeRedirectPolicy`. This works on both Qt 5.15+
   and Qt 6.x (unlike the deprecated `RedirectionTargetAttribute` which is
   removed in Qt 6). Maximum redirects configured via
   `QNetworkRequest::setMaximumRedirectsAllowed(10)`.
5. Respect HTTP proxy settings from environment variables (`http_proxy`,
   `https_proxy`) — consistent with existing `DownloadWidget` behavior.

### Preconditions (documented in class header)

- A `QCoreApplication` instance MUST exist before calling any download method.
  This is asserted at runtime in debug builds.

## Consequences

- `src/core/CMakeLists.txt` must link `Qt::Network` (add to
  `target_link_libraries`).
- The CLI tool gains the ability to download online libraries without any GUI
  code.
- The old `DownloadWidget` in `src/gui/` becomes a thin GUI wrapper or is
  deprecated entirely.
- Unit tests can exercise the full download→parse→verify pipeline.
