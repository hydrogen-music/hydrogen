#!/usr/bin/env python3

import pytest
import os
import os.path

# Files the installer must always place (the standalone app + its data).
FILES = [
    "hydrogen.exe",
    "data/hydrogen.default.conf",
    "data/i18n/hydrogen_hu_HU.qm",
]

# Plugin bundles. CLAP and LV2 are built on every Windows job (WANT_CLAP/WANT_LV2),
# so the installer must always ship them next to the app (H2_LIB_PATH="." on MINGW
# -> clap/ and lv2/ under the install root). The editor binary (hydrogen.exe) sits
# alongside so the plugins can launch it (ADR 0016).
PLUGIN_FILES = [
    "clap/Hydrogen.clap",
    "lv2/hydrogen.lv2/manifest.ttl",
    "lv2/hydrogen.lv2/hydrogen.ttl",
]

# VST3 is only built on artifact builds (it fetches the Steinberg SDK). When
# present it is installed under vst3/ as a single-file module on Windows.
VST3_FILE = "vst3/Hydrogen.vst3"

ROOT = os.getenv("INSTDIR", default=r"C:\Program Files\Hydrogen")


@pytest.mark.parametrize("file_name", FILES + PLUGIN_FILES)
def test_installed_file(file_name):
    full_path = os.path.join(ROOT, file_name)
    assert os.path.exists(full_path), f"missing from install: {file_name}"


def test_vst3_bundle_if_built():
    # Don't fail when VST3 wasn't part of this build; only assert it's installed
    # when the build produced it (artifact builds).
    full_path = os.path.join(ROOT, VST3_FILE)
    if not os.path.exists(full_path):
        pytest.skip("VST3 not built in this configuration")
    assert os.path.exists(full_path)
