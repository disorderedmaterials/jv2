# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import json

from jv2backend.cycle import Cycle
from jv2backend.instrument import Instrument
from jv2backend.io.isis.xmlJournalReader import ISISXMLJournalReader

import pytest

SAMPLE_JOURNAL_INST = "ALF"

BAD_CYCLE_FORMAT = b"""<?xml version="1.0" encoding="UTF-8"?>
  <NXroot NeXus_version="4.3.0" XML_version="mxml"
file_name="c:\data\journal_2021_1.xml"
xmlns="http://definition.nexusformat.org/schema/3.0"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xsi:schemaLocation="http://definition.nexusformat.org/schema/3.0"
file_time="2021-05-01T09:28:00+00:00">
</NXroot>
"""

PARTIAL_RUN = b"""<?xml version="1.0" encoding="UTF-8"?>
  <NXroot NeXus_version="4.3.0" XML_version="mxml"
file_name="c:\data\journal_21_1.xml"
xmlns="http://definition.nexusformat.org/schema/3.0"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xsi:schemaLocation="http://definition.nexusformat.org/schema/3.0"
file_time="2021-05-01T09:28:00+00:00">
    <NXentry name="ALF00085422">
      <title NAPItype="NX_CHAR[15]">cycle 211 setup</title>
"""


def test_read_indexfile_returns_JournalIndex_with_expected_number_of_entries(
    sample_journallist_xml,
):
    reader = ISISXMLJournalReader(Instrument("fake"))

    journals = reader.read_indexfile(sample_journallist_xml)

    assert len(journals) == 2
    assert "journal_21_1.xml" in journals
    assert "journal_20_2.xml" in journals


@pytest.mark.parametrize(
    "content",
    [b"", b"{}", PARTIAL_RUN],
)
def test_reader_raises_ValueError_for_bad_data(content):
    reader = ISISXMLJournalReader(Instrument("fake"))

    with pytest.raises(SyntaxError):
        reader.read_journalfile(content)


def test_reader_returns_journal_with_expected_attributes_and_strips_whitespace(
    sample_journal_xml,
):
    reader = ISISXMLJournalReader(instrument=Instrument(SAMPLE_JOURNAL_INST))

    journal = reader.read_journalfile(sample_journal_xml())

    assert journal.run_count == 3
    assert json.loads(journal.to_json())[0]["run_number"] == "85422"
