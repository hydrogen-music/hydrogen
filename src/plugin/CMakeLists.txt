# Placeholder for plugin targets.
# Real targets will be added in Phase 4 (CLAP + LV2) and Phase 6 (VST3).
# See docs/proposals/0004-plugin-port-implementation-plan.md
#
# T3.4 (ADR 0013) — when these targets are defined they must exclude the
# standalone-only entry points from the plugin build path:
#   * DiskWriterDriver  (audio export; never selected under a plugin host -
#                        the audio driver is always Preferences::AudioDriver::Plugin)
#   * NSM session client (NsmClient bootstrap lives in the standalone main();
#                         Hydrogen::isUnderPluginHost() keeps it inert at runtime)
#   * JACK session / JACK transport (only reachable with the JACK driver)
# At runtime these are already unreachable under a plugin host (gated via the
# Plugin audio/MIDI drivers and Hydrogen::isUnderPluginHost(), ADR 0026). The
# build-level exclusion is a property of the plugin target's source list, so it
# is applied here once that target exists rather than in the shared core library.

if(WANT_CLAP OR WANT_LV2 OR WANT_VST3)
    # Plugin targets will be defined here once the core is ready.
    message(STATUS "Plugin targets: LV2=${WANT_LV2}, CLAP=${WANT_CLAP}, VST3=${WANT_VST3}, Buses=${H2_PLUGIN_OUTPUT_BUSES}")
endif()
