# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.io.isis.xmljournallistreader import ISISXMLJournalListReader


def test_reader_returns_JournalIndex_with_expected_number_of_entries(
    sample_xml_journallist,
):
    reader = ISISXMLJournalListReader()

    journals = reader.read(sample_xml_journallist)

    assert len(journals) == 1
    assert journals[0] == "journal_21_1.xml"
