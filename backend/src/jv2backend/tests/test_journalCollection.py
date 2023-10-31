# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journals import JournalCollection, JournalFile, JournalData
import datetime
import pandas
import pytest

# Construct two test journals
with open("data/simpleRunData1.xml", "rb") as f1:
    journal1 = JournalFile("Journal A", "/a/local/disk",
                           "data", "simpleRunData1.xml",
                           "/fake/data/root", datetime.datetime.now(),
                           JournalData(pandas.read_xml(f1, dtype=str)))
with open("data/simpleRunData2.xml", "rb") as f2:
    journal2 = JournalFile("Journal B", "/a/local/disk",
                           "data", "simpleRunData2.xml",
                           "/fake/data/root", datetime.datetime.now(),
                           JournalData(pandas.read_xml(f2, dtype=str)))


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
