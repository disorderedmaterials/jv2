# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from pathlib import Path

from jv2backend.instrument import Instrument
from jv2backend.io.isis.xmljournallistreader import ISISXMLJournalListReader

SAMPLE_JOURNAL_INDEX_FILE = Path(__file__).parent / "data" / "journal_main.xml"
SAMPLE_JOURNAL_INST = "ALF"


def test_reader_returns_JournalIndex_with_expected_number_of_entries():
    reader = ISISXMLJournalListReader()

    with open(SAMPLE_JOURNAL_INDEX_FILE) as journal_main:
        journals = reader.read(journal_main.read().encode("utf-8"))

    assert len(journals) == 1
    assert journals[0] == "journal_21_1.xml"
