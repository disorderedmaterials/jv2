# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

# """Provide common test fixtures for pytest"""
from typing import Callable
from pathlib import Path
import pytest


@pytest.fixture()
def test_data_dir() -> Path:
    """Return the path to the test data"""
    return Path(__file__).parent / "data"


@pytest.fixture()
def sample_journal_xml(test_data_dir) -> Callable:
    """Provide a function to return sample journal data.
    The fixture accepts a filename for the journal"""

    def _fixture(filename: str = "journal_21_1.xml") -> bytes:
        with open(test_data_dir / filename) as handle:
            return handle.read().encode("utf-8")

    return _fixture


@pytest.fixture()
def sample_journallist_xml(test_data_dir) -> bytes:
    """Provide a sample XML list of journal files data"""
    with open(test_data_dir / "journal_main.xml") as handle:
        return handle.read().encode("utf-8")


@pytest.fixture()
def sample_nexus_filepath(test_data_dir) -> Path:
    """Sample NeXus file"""
    return test_data_dir / "ALF85423.nxs"
