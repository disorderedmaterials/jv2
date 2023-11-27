# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.classes.journal import Journal, SourceType
from jv2backend.utils import url_join
import jv2backend.main.selector
import datetime
import pytest
from pathlib import Path
import xml.etree.ElementTree as ElementTree

# Journal Data
JOURNAL_NAME = "MyName"
JOURNAL_SOURCE_TYPE = SourceType.Network
JOURNAL_LIBRARY_KEY = "FakeKey"
JOURNAL_DIRECTORY = "/my/root/location/directory"
JOURNAL_FILENAME = "file.name.xml"
JOURNAL_FILE_URL = url_join(JOURNAL_DIRECTORY, JOURNAL_FILENAME)
JOURNAL_DATA_ROOT = "/my/data/directory"
JOURNAL_MODTIME = datetime.datetime.now()


@pytest.fixture
def _data_dir() -> Path:
    """Return the path to the test data"""
    return Path(__file__).parent / "data"


@pytest.fixture
def _example_journal(_data_dir):
    journal = Journal(JOURNAL_NAME, JOURNAL_SOURCE_TYPE,
                      JOURNAL_LIBRARY_KEY,
                      JOURNAL_DIRECTORY, JOURNAL_FILENAME,
                      JOURNAL_DATA_ROOT, JOURNAL_MODTIME)

    # Load test run data
    with open(_data_dir / "simpleRunData1.xml", "rb") as f:
        runDataTree = ElementTree.parse(f)

    journal.set_run_data_from_element_tree(runDataTree)

    return journal


def test_basic_generation(_example_journal):
    assert _example_journal.get_run_count() == 6
    assert _example_journal.get_last_run_number() == 9


@pytest.mark.parametrize("run_number", [1, 2, 3, 6, 8, 9])
def test_run_data_contains_run_number(_example_journal, run_number):
    assert run_number in _example_journal
    assert _example_journal.get_run(run_number) is not None


@pytest.mark.parametrize("run_number", [0, 4, 5, 7, 99, 12301])
def test_run_data_does_not_contain_run_number(_example_journal, run_number):
    assert run_number not in _example_journal
    assert _example_journal.get_run(run_number) is None


@pytest.mark.parametrize("case_sensitive", [True, False])
def test_search_by_user_name_field_uses_a_contains_check_not_exact_match(_example_journal, case_sensitive):
    search_results = jv2backend.main.selector.select(_example_journal.run_data, "user_name", "devlin", case_sensitive)

    if case_sensitive:
        assert len(search_results) == 0
    else:
        assert len(search_results) == 6


@pytest.mark.parametrize("case_sensitive", [True, False])
def test_search_by_experiment_identifier_uses_exact_string_matching(_example_journal, case_sensitive):
    search_results = jv2backend.main.selector.select(_example_journal.run_data, "experiment_identifier", "123456", case_sensitive)
    assert len(search_results) == 0

    search_results = jv2backend.main.selector.select(_example_journal.run_data, "experiment_identifier", "1234567", case_sensitive)
    assert len(search_results) == 3


@pytest.mark.parametrize("case_sensitive", [True, False])
def test_search_by_title_uses_a_contains_check_and_not_exact_match(_example_journal, case_sensitive):
    search_results = jv2backend.main.selector.select(_example_journal.run_data, "title", "science", case_sensitive)
    if case_sensitive:
        assert len(search_results) == 0
    else:
        assert len(search_results) == 5


def test_search_by_run_number_uses_a_range_check_assuming_user_gives_start_end(_example_journal):
    search_results = jv2backend.main.selector.select(_example_journal.run_data, "run_number", "10-1000")
    assert len(search_results) == 0

    search_results = jv2backend.main.selector.select(_example_journal.run_data, "run_number", "1-5")
    assert len(search_results) == 3

    assert 1 in search_results
    assert 2 in search_results
    assert 3 in search_results


def test_search_by_run_number_uses_a_range_check_given_range_operator(_example_journal):
    search_results = jv2backend.main.selector.select(_example_journal.run_data, "run_number", ">6")
    assert len(search_results) == 2

    assert 8 in search_results
    assert 9 in search_results


def test_search_by_start_time_treating_input_as_datetime_and_equivalent_to_start_date(_example_journal):
    search_results = jv2backend.main.selector.select(_example_journal.run_data, "start_time", "2023/02/03-2023/02/04")

    assert len(search_results) == 2

    assert 2 in search_results
    assert 3 in search_results


# Helpers


def _example_journal_data(journal: Journal):
    assert journal.display_name == JOURNAL_NAME
    assert journal.filename == JOURNAL_FILENAME
    assert journal.get_file_url() == JOURNAL_FILE_URL
    assert journal.data_directory == JOURNAL_DATA_ROOT
    assert journal.last_modified == JOURNAL_MODTIME
