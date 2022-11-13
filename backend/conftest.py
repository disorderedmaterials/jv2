# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
# """Provide common test fixtures for pytest"""
from typing import Callable
from pathlib import Path

import pandas as pd
import pytest

TEST_DATA_DIR = Path(__file__).parent / "data"


@pytest.fixture()
def sample_journal_xml() -> Callable:
    """Provide a function to return sample journal data.
    The fixture accepts a filename for the journal"""

    def _fixture(filename: str = "journal_21_1.xml") -> bytes:
        with open(TEST_DATA_DIR / filename) as handle:
            return handle.read().encode("utf-8")

    return _fixture


@pytest.fixture()
def sample_journallist_xml() -> bytes:
    """Provide a sample XML list of journal files data"""
    with open(TEST_DATA_DIR / "journal_main.xml") as handle:
        return handle.read().encode("utf-8")


@pytest.fixture()
def sample_journal_dataframe() -> pd.DataFrame:
    """Provide a sample dataframe"""
    return pd.read_xml(TEST_DATA_DIR / "journal_21_1.xml", dtype=str)
