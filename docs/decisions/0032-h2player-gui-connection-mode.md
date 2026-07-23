---
status: accept
date: 2026-07-23
deciders: pm
---

# AD: h2player GUI connection mode

## Context and Problem Statement

The `h2player` CLI tool is a legacy headless version of Hydrogen controlled via keyboard input. It is rarely used and has accumulated technical debt, though no issues are reported on GitHub. Currently, `h2player` runs as a standalone headless engine with keyboard control (play/stop/rewind/quit commands).

Meanwhile, the plugin architecture ([ADR 0016](0016-out-of-process-plugin-ui.md), [ADR 0018](0018-plugin-editor-ipc-transport.md)) has established a robust pattern for headless engine instances to connect to the full Hydrogen GUI over IPC:
- The plugin creates a headless engine and starts an `EngineSession` to serve it over IPC
- The GUI is launched as `hydrogen --plugin-editor <endpoint>` and connects via `EditorSession`
- Commands flow through `IpcCoreActionController`, events through `EventQueue`, telemetry through shared memory

This architecture is mature, well-tested, and provides full GUI functionality to headless engines. However, `h2player` cannot currently leverage this infrastructure.

## Decision Drivers

* **Reuse existing infrastructure:** The plugin IPC architecture is production-ready and handles all engine↔GUI communication patterns.
* **Preserve existing functionality:** The current keyboard-controlled mode must remain available for users who depend on it.
* **Minimal code duplication:** Connection establishment logic should be shared between plugins and `h2player`.
* **Clear user interface:** Users should have an obvious way to choose between keyboard-only and GUI-connected modes.
* **Future-proofing:** As the plugin architecture evolves, `h2player` should benefit automatically.

## Considered Options

1. **Make h2player launch the GUI exactly like plugins do.**
   - h2player creates headless engine, starts `EngineSession`, spawns `hydrogen --plugin-editor <endpoint>`
   - Common connection code extracted to `src/core/IPC/`
   - Keyboard mode gated behind `--interactive` flag (default: off)

2. **Fork h2player into separate tools: `h2player` (keyboard-only) and `h2player-gui` (GUI-connected).**
   - Avoids CLI complexity but duplicates code and maintenance burden
   - Users must discover and choose between two binaries

3. **Make GUI connection the default and deprecate keyboard mode.**
   - Breaking change for existing users
   - Keyboard mode is still useful for simple playback testing

4. **Integrate h2player functionality into the main `hydrogen` CLI.**
   - Would require significant refactoring of the main binary
   - Blurs separation between GUI and CLI tools

## Decision Outcome

Chosen option: **Option 1 — Add `--gui` option to h2player with shared connection infrastructure.**

### CLI Interface

```bash
# Default: IPC server started, connection info printed, GUI connection optional
h2player song.h2song
# Output: IPC endpoint: hydrogen-headless-1234-0
#         Connect with: hydrogen -c hydrogen-headless-1234-0

# Disable IPC server (pure headless mode)
h2player song.h2song --no-ipc

# Keyboard-interactive mode (legacy behavior, can be combined with IPC)
h2player song.h2song --interactive
h2player song.h2song --interactive --no-ipc  # keyboard without IPC
```

**GUI connection process:**
1. `h2player` automatically starts `EngineSession` (unless `--no-ipc`)
2. `h2player` prints IPC endpoint and connection command
3. User manually runs `hydrogen -c <endpoint>` to connect GUI
4. GUI connects to h2player over IPC

**Rationale for design:**
- IPC enabled by default for maximum flexibility
- Manual GUI launch gives users control over when/how to connect
- `--no-ipc` for pure headless use cases (automation, testing)
- `--interactive` preserved for legacy keyboard control
- Clear separation between engine (h2player) and GUI (hydrogen) processes

### Hydrogen CLI Option Rename

The `--plugin-editor` option is renamed to `-c/--connect-via-ipc` for clarity:

```bash
# Old (deprecated):
hydrogen --plugin-editor hydrogen-headless-1234-0

# New:
hydrogen -c hydrogen-headless-1234-0
hydrogen --connect-via-ipc hydrogen-headless-1234-0
```

### Code Organization

**Extract common connection infrastructure to `src/core/IPC/`:**

```cpp
// New file: src/core/IPC/HeadlessEngineLauncher.h
class HeadlessEngineLauncher {
public:
    // Create headless engine configured for IPC serving
    static Hydrogen* createHeadlessEngine();

    // Generate unique IPC endpoint (reused from HydrogenPlugin)
    static QString makeEndpoint();

    // Format connection information message for user
    static QString formatConnectionInfo(const QString& sEndpoint);
};
```

**h2player integration:**

```cpp
// In h2player main.cpp:
bool bEnableIpc = ! bNoIpc;
if ( bEnableIpc ) {
    // Start IPC server automatically
    const QString sEndpoint = HeadlessEngineLauncher::makeEndpoint();
    m_pEngineSession = EngineSession::start( pHydrogen, sEndpoint );
    if ( m_pEngineSession != nullptr ) {
        // Print connection info for user
        cout << HeadlessEngineLauncher::formatConnectionInfo( sEndpoint ).toStdString();
    }
}

if ( bInteractive ) {
    // Keyboard mode (with or without IPC)
    runKeyboardLoop();
} else {
    // Headless mode (with or without IPC)
    runHeadlessLoop();
}
```

**Hydrogen CLI option rename:**

```cpp
// In src/gui/src/Parser.cpp:
// Old: QCommandLineOption pluginEditorOption( "plugin-editor", ... )
// New:
QCommandLineOption connectIpcOption(
    QStringList() << "c" << "connect-via-ipc",
    "Connect to headless engine via IPC endpoint",
    "endpoint", "" );
```

### Consequences

* **New dependency:** h2player gains Qt dependency for IPC (already present in core)
* **Shared code:** `HeadlessEngineLauncher` can be reused by plugins, h2player, and future tools
* **Testing:** IPC mode can be tested using existing `EditorModeTest` -> `ConnectViaIpcModeTest` infrastructure
* **Documentation:** h2player manpage and help text need updates; hydrogen CLI option rename requires documentation updates
* **Backward compatibility:** Existing scripts using h2player without flags now get IPC server (can be disabled with `--no-ipc`)
* **Breaking change:** `--plugin-editor` option (not part of any release) renamed to `-c/--connect-via-ipc` (affects automation/scripts)
* **User workflow:** Two-step process (start h2player, then connect GUI) gives users more control but requires manual intervention

## More Information

* Plugin IPC architecture: [ADR 0016](0016-out-of-process-plugin-ui.md), [ADR 0018](0018-plugin-editor-ipc-transport.md), [ADR 0030](0030-coreactioncontroller-over-ipc.md)
* EngineSession: `src/core/IPC/EngineSession.h`
* EditorSession: `src/core/IPC/EditorSession.h`
* HydrogenPlugin: `src/plugin/HydrogenPlugin.h`
* Current h2player: `src/player/main.cpp`
