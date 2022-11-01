# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
# """Provide common test fixtures for pytest"""
from pathlib import Path

import pytest

TEST_DATA_DIR = Path(__file__).parent / "data"


@pytest.fixture()
def sample_journal_xml() -> bytes:
    """Provide a sample XML Journal of Run data"""
    with open(TEST_DATA_DIR / "journal_21_1.xml") as handle:
        return handle.read().encode("utf-8")


@pytest.fixture()
def sample_journallist_xml() -> bytes:
    """Provide a sample XML list of journal files data"""
    with open(TEST_DATA_DIR / "journal_main.xml") as handle:
        return handle.read().encode("utf-8")
