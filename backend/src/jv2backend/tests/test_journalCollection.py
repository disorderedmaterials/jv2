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
    collection = JournalCollection(SourceType.InternalTest,
                                   "FakeKey",
                                   "/a/local/disk",
                                   "index.xml",
                                   str(_fake_server_data_dir))

    # Construct two example journals and make a collection
    journal1 = collection.add_journal(
        "Journal A", "simpleRunData1.xml",
        str(_fake_server_data_dir), datetime.datetime.now()
    )
    with open(_fake_server_data_dir / "simpleRunData1.xml", "rb") as f1:
        runDataTree1 = ElementTree.parse(f1)
        journal1.set_run_data_from_element_tree(runDataTree1)

    journal2 = collection.add_journal(
        "Journal B", "simpleRunData2.xml",
        str(_fake_server_data_dir), datetime.datetime.now()
    )
    with open(_fake_server_data_dir / "simpleRunData2.xml", "rb") as f2:
        runDataTree2 = ElementTree.parse(f2)
        journal2.set_run_data_from_element_tree(runDataTree2)

    return  collection


def test_constructor(_example_collection):
    assert _example_collection.get_journal_count() == 2


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


@pytest.mark.parametrize("journal_and_run", [("Journal A", 1), ("Journal A", 3), ("Journal B", 4)])
def test_data_file_can_be_found_in_journal(_example_collection, _fake_server_data_dir, journal_and_run):
    expected_journal, run_number = journal_and_run
    journal = _example_collection.journal_for_run(run_number)
    assert journal is not None
    assert journal.display_name == expected_journal


@pytest.mark.parametrize("run_number", [1,2,3,4,5,7])
def test_data_file_can_be_found_in_collection(_example_collection, _fake_server_data_dir, run_number):
    assert _example_collection.locate_data_file(run_number) == str(_fake_server_data_dir / f"JVTEST0000000{run_number}.nxs")


@pytest.mark.parametrize("run_number", [1001,1002])
def test_data_file_not_found_in_collection(_example_collection, _fake_server_data_dir, run_number):
    assert _example_collection.locate_data_file(run_number) is None
