# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journalCollection import JournalCollection
from jv2backend.journal import Journal, SourceType
import xml.etree.ElementTree as ElementTree
from pathlib import Path
import datetime
import pytest


@pytest.fixture
def _fake_server_data_dir() -> Path:
    """Return the path to the test data"""
    return Path(__file__).parent / "data"

@pytest.fixture
def _example_collection(_fake_server_data_dir):
    # Construct two example journals and make a collection
    journal1 = Journal("Journal A", SourceType.File, "FakeKey",
                       "/a/local/disk", "simpleRunData1.xml",
                       "/fake/data/root", datetime.datetime.now())
    with open(_fake_server_data_dir / "simpleRunData1.xml", "rb") as f1:
        runDataTree1 = ElementTree.parse(f1)
        journal1.set_run_data_from_element_tree(runDataTree1)

    journal2 = Journal("Journal B", SourceType.File, "FakeKey",
                       "/a/local/disk", "simpleRunData2.xml",
                       "/fake/data/root", datetime.datetime.now())
    with open(_fake_server_data_dir / "simpleRunData2.xml", "rb") as f2:
        runDataTree2 = ElementTree.parse(f2)
        journal2.set_run_data_from_element_tree(runDataTree2)

    return JournalCollection([journal1, journal2])


def test_constructor(_example_collection):
    assert len(_example_collection.journalFiles) == 2


@pytest.mark.parametrize("run_number,journal", [(1, "Journal A"), (2, "Journal A"), (4, "Journal B"), (5, "Journal B"), (10, None)])
def test_find_journal_containing_run_number(_example_collection, run_number: int, journal: str):
    if journal is None:
        assert _example_collection.journal_for_run(run_number) is None
    else:
        assert _example_collection.journal_for_run(run_number) is not None
        assert _example_collection.journal_for_run(run_number).display_name == journal


def test_retrieve_valid_journal(_example_collection):
    journal = _example_collection["simpleRunData1.xml"]
    assert journal is not None
    assert journal.display_name == "Journal A"


def test_retrieve_invalid_journal(_example_collection):
    journal = _example_collection["simpleRunData99.xml"]
    assert journal is None


@pytest.mark.parametrize("case_sensitive", ["true", "false", "notEither"])
def test_search_across_journals_for_title(_example_collection, case_sensitive):
    matches = _example_collection.search({"title": "science", "caseSensitive": case_sensitive})

    if case_sensitive == "true":
        assert len(matches) == 0
    else:
        assert len(matches) == 9


def test_search_across_journals_for_title_and_run_number_range(_example_collection):
    matches = _example_collection.search({"title": "science", "run_number": "6-10"})

    assert len(matches) == 3
    for run in [6, 7, 8]:
        assert run in matches


def test_search_across_journals_for_title_and_start_date(_example_collection):
    matches = _example_collection.search({"title": "again", "start_time": "<2023/02/05"})

    assert len(matches) == 1
    assert 3 in matches
