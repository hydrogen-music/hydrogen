# Placeholder for plugin targets.
# Real targets will be added in Phase 4 (CLAP + LV2) and Phase 6 (VST3).
# See docs/proposals/0004-plugin-port-implementation-plan.md

if(WANT_CLAP OR WANT_LV2 OR WANT_VST3)
    # Plugin targets will be defined here once the core is ready.
    message(STATUS "Plugin targets: LV2=${WANT_LV2}, CLAP=${WANT_CLAP}, VST3=${WANT_VST3}, Buses=${H2_PLUGIN_OUTPUT_BUSES}")
endif()
