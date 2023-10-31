# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journals import BasicJournalFile, JournalFile, JournalData
import datetime
import pandas
import typing

# Journal Data
JOURNAL_NAME = "MyName"
JOURNAL_ROOT_URL = "/my/root/location"
JOURNAL_DIRECTORY = "myDirectory"
JOURNAL_FILENAME = "MyFilename"
JOURNAL_DATA_ROOT = "/my/data/directory"
JOURNAL_MODTIME = datetime.datetime.now()


def test_basic_constructor():
    journal = BasicJournalFile(JOURNAL_NAME, JOURNAL_ROOT_URL,
                               JOURNAL_DIRECTORY, JOURNAL_FILENAME,
                               JOURNAL_DATA_ROOT, JOURNAL_MODTIME)

    _test_journal_data(journal)


def test_derived_constructor():
    derived = JournalFile(JOURNAL_NAME, JOURNAL_ROOT_URL,
                          JOURNAL_DIRECTORY, JOURNAL_FILENAME,
                          JOURNAL_DATA_ROOT, JOURNAL_MODTIME,
                          JournalData(pandas.DataFrame()))

    _test_journal_data(derived)
    assert derived.run_data.run_count == 0

def test_basic_from_derived():
    derived = JournalFile(JOURNAL_NAME, JOURNAL_ROOT_URL,
                          JOURNAL_DIRECTORY, JOURNAL_FILENAME,
                          JOURNAL_DATA_ROOT, JOURNAL_MODTIME,
                          JournalData(pandas.DataFrame()))
    basic = JournalFile.from_derived(derived)

    _test_journal_data(basic)

# Helpers

def _test_journal_data(journal_data: typing.Any):
    assert journal_data.display_name == JOURNAL_NAME
    assert journal_data.server_root == JOURNAL_ROOT_URL
    assert journal_data.directory == JOURNAL_DIRECTORY
    assert journal_data.filename == JOURNAL_FILENAME
    assert journal_data.data_directory == JOURNAL_DATA_ROOT
    assert journal_data.last_modified == JOURNAL_MODTIME
