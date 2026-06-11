# Plugin directory

Placeholder for plugin targets (CLAP, LV2, VST3).

See [Proposal 0004](../../docs/proposals/0004-plugin-port-implementation-plan.md) for the implementation plan.

## Build options

- `WANT_CLAP` — Build CLAP plugin (default: OFF)
- `WANT_LV2` — Build LV2 plugin (default: OFF)
- `WANT_VST3` — Build VST3 plugin via clap-wrapper (default: OFF)
- `H2_PLUGIN_OUTPUT_BUSES` — Number of output buses (default: 32)

All plugin builds are OFF by default to keep CI lean. Enable them with:

```
cmake -DWANT_CLAP=ON -DWANT_LV2=ON ..
```
