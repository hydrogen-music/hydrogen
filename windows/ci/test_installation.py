#!/usr/bin/env python3

import pytest
import os
import os.path

FILES = [
    "hydrogen.exe",
    "data/hydrogen.default.conf",
    "data/i18n/hydrogen_fr.qm"
]

ROOT = os.getenv("INSTDIR", default=r"C:\Program Files\Hydrogen")


@pytest.mark.parametrize("file_name", FILES)
def test_installed_file(file_name):
    full_path = os.path.join(ROOT, file_name)
    assert os.path.exists(full_path)
