# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journals import JournalData
import xml.etree.ElementTree as ElementTree
import json
import pytest

# Load test run data
with open("jv2backend/tests/data/simpleRunData1.xml", "rb") as f:
    runDataTree = ElementTree.parse(f)


def test_basic_constructor():
    data = JournalData.from_element_tree(runDataTree)

    assert data.run_count == 6


def test_run_data_ranges():
    data = JournalData.from_element_tree(runDataTree)

    assert data.get_last_run_number == 9


@pytest.mark.parametrize("run_number", [1, 2, 3, 6, 8, 9])
def test_run_data_contains_run_number(run_number):
    data = JournalData.from_element_tree(runDataTree)

    assert run_number in data
    assert data.run(run_number) is not None


@pytest.mark.parametrize("run_number", [0, 4, 5, 7, 99, 12301])
def test_run_data_does_not_contain_run_number(run_number):
    data = JournalData.from_element_tree(runDataTree)

    assert run_number not in data
    assert data.run(run_number) is None


@pytest.mark.parametrize("case_sensitive", [True, False])
def test_search_by_user_name_field_uses_a_contains_check_not_exact_match(case_sensitive):
    data = JournalData.from_element_tree(runDataTree)

    search_results = data.search("user_name", "devlin", case_sensitive)

    if case_sensitive:
        assert search_results.run_count == 0
    else:
        assert search_results.run_count == 6


@pytest.mark.parametrize("case_sensitive", [True, False])
def test_search_by_experiment_identifier_uses_exact_string_matching(case_sensitive):
    data = JournalData.from_element_tree(runDataTree)

    search_results = data.search("experiment_identifier", "123456", case_sensitive)
    assert search_results.run_count == 0

    search_results = data.search("experiment_identifier", "1234567", case_sensitive)
    assert search_results.run_count == 3


@pytest.mark.parametrize("case_sensitive", [True, False])
def test_search_by_title_uses_a_contains_check_and_not_exact_match(case_sensitive):
    data = JournalData.from_element_tree(runDataTree)

    search_results = data.search("title", "science", case_sensitive)
    if case_sensitive:
        assert search_results.run_count == 0
    else:
        assert search_results.run_count == 5


def test_search_by_run_number_uses_a_range_check_assuming_user_gives_start_end():
    data = JournalData.from_element_tree(runDataTree)

    search_results = data.search("run_number", "10-1000")
    assert search_results.run_count == 0

    search_results = data.search("run_number", "1-5")
    assert search_results.run_count == 3

    runs = json.loads(search_results.to_json())

    assert runs[0]["run_number"] == "1"
    assert runs[1]["run_number"] == "2"
    assert runs[2]["run_number"] == "3"


def test_search_by_run_number_uses_a_range_check_given_range_operator():
    data = JournalData.from_element_tree(runDataTree)

    search_results = data.search("run_number", ">6")
    assert search_results.run_count == 2

    runs = json.loads(search_results.to_json())
    assert runs[0]["run_number"] == "8"
    assert runs[1]["run_number"] == "9"


def test_search_by_start_time_treating_input_as_datetime_and_equivalent_to_start_date():
    data = JournalData.from_element_tree(runDataTree)

    search_results = data.search("start_time", "2023/02/03-2023/02/04")

    assert search_results.run_count == 2
    runs = json.loads(search_results.to_json())
    assert runs[0]["run_number"] == "2"
    assert runs[1]["run_number"] == "3"
