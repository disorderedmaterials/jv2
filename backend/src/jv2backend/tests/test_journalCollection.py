# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journals import JournalCollection
from jv2backend.journalFile import JournalFile, JournalData
import xml.etree.ElementTree as ElementTree
import datetime
import pytest

# Construct two test journals
with open("jv2backend/tests/data/simpleRunData1.xml", "rb") as f1:
    journal1 = JournalFile("Journal A", "/a/local/disk", "simpleRunData1.xml",
                           "/fake/data/root", datetime.datetime.now(),
                           JournalData.from_element_tree(ElementTree.parse(f1)))
with open("jv2backend/tests/data/simpleRunData2.xml", "rb") as f2:
    journal2 = JournalFile("Journal B", "/a/local/disk", "simpleRunData2.xml",
                           "/fake/data/root", datetime.datetime.now(),
                           JournalData.from_element_tree(ElementTree.parse(f2)))


def test_constructor():
    collection = JournalCollection([journal1, journal2])

    assert len(collection.journalFiles) == 2


@pytest.mark.parametrize("run_number,journal", [(1, "Journal A"), (2, "Journal A"), (4, "Journal B"), (5, "Journal B"), (10, None)])
def test_find_journal_containing_run_number(run_number: int, journal: str):
    collection = JournalCollection([journal1, journal2])

    if journal is None:
        assert collection.journal_for_run(run_number) is None
    else:
        assert collection.journal_for_run(run_number) is not None
        assert collection.journal_for_run(run_number).display_name == journal


def test_retrieve_valid_journal():
    collection = JournalCollection([journal1, journal2])

    journal = collection.get_journal("simpleRunData1.xml")
    assert journal is not None
    assert journal.display_name == "Journal A"


def test_retrieve_invalid_journal():
    collection = JournalCollection([journal1, journal2])

    journal = collection.get_journal("simpleRunData99.xml")
    assert journal is None


@pytest.mark.parametrize("case_sensitive", ["true", "false", "notEither"])
def test_search_across_journals_for_title(case_sensitive):
    collection = JournalCollection([journal1, journal2])
    matches = collection.search({"title": "science", "caseSensitive": case_sensitive})

    if case_sensitive == "true":
        assert len(matches) == 0
    else:
        assert len(matches) == 9


def test_search_across_journals_for_title_and_run_number_range():
    collection = JournalCollection([journal1, journal2])
    matches = collection.search({"title": "science", "run_number": "6-10"})

    assert len(matches) == 3
    for run in [6, 7, 8]:
        assert run in matches


def test_search_across_journals_for_title_and_start_date():
    collection = JournalCollection([journal1, journal2])
    matches = collection.search({"title": "again", "start_time": "<2023/02/05"})

    assert len(matches) == 1
    assert 3 in matches
