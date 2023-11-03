# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journal import Journal, JournalData
from jv2backend.utils import url_join
import datetime

# Journal Data
JOURNAL_NAME = "MyName"
JOURNAL_DIRECTORY = "/my/root/location/directory"
JOURNAL_FILENAME = "file.name.xml"
JOURNAL_FILE_URL = url_join(JOURNAL_DIRECTORY, JOURNAL_FILENAME)
JOURNAL_DATA_ROOT = "/my/data/directory"
JOURNAL_MODTIME = datetime.datetime.now()


def test_constructor():
    derived = Journal(JOURNAL_NAME, JOURNAL_DIRECTORY, JOURNAL_FILENAME,
                      JOURNAL_DATA_ROOT, JOURNAL_MODTIME,
                      JournalData({}))

    _test_journal_data(derived)
    assert derived.run_data.run_count == 0


# Helpers


def _test_journal_data(journal: Journal):
    assert journal.display_name == JOURNAL_NAME
    assert journal.filename == JOURNAL_FILENAME
    assert journal.get_file_url() == JOURNAL_FILE_URL
    assert journal.data_directory == JOURNAL_DATA_ROOT
    assert journal.last_modified == JOURNAL_MODTIME
