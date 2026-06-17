# Plugin directory

Native plugin targets (ADR 0013/0014) built on the format-agnostic
`HydrogenPlugin` engine wrapper.

See [Proposal 0004](../../docs/proposals/0004-plugin-port-implementation-plan.md)
for the implementation plan.

- `HydrogenPlugin.{h,cpp}` — format-agnostic core: a headless Hydrogen instance
  driven by the host (audio buffers, output buses, transport, MIDI) plus state
  save/load via the `.h2project` codec. Always built (`hydrogen-plugin`); unit
  tested by `PluginLifecycleTest`.
- `clap/HydrogenClap.cpp` — native CLAP plugin (`WANT_CLAP`).
- `lv2/HydrogenLv2.cpp` + generated `hydrogen.ttl` — native LV2 plugin
  (`WANT_LV2`).

## Build options

- `WANT_CLAP` — Build CLAP plugin (default: OFF)
- `WANT_LV2` — Build LV2 plugin (default: OFF)
- `WANT_VST3` — Build VST3 plugin via clap-wrapper (default: OFF, Phase 6)
- `H2_PLUGIN_OUTPUT_BUSES` — Number of stereo output buses (default: 32)

All plugin builds are OFF by default to keep the standard build/CI lean. Enable
with:

```
cmake -DWANT_CLAP=ON -DWANT_LV2=ON ..
```

## SDKs

The CLAP and LV2 SDKs live under `extern/` (`extern/clap`, `extern/lv2`).

## Conformance tests

When the plugins are enabled, the validators are wired into CTest:

- `clap-validate` — runs `clap-validator validate` on the built `.clap`. The
  validator is found from `extern/clap-validator/target/release` (build it with
  `cargo build --release` in that directory) or `PATH`.
- `lv2-smoke` — an in-process LV2 host (`lv2_smoke`) that drives the built
  module through its full lifecycle. If `lv2lint` is installed, a stricter
  `lv2lint` test is added too.

```
cmake -DWANT_CLAP=ON -DWANT_LV2=ON ..
make hydrogen-clap hydrogen-lv2 hydrogen-lv2-smoke
ctest -R "clap-validate|lv2-smoke|lv2lint" --output-on-failure
```
