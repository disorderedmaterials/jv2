# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs

from jv2backend.cycle import Cycle
from jv2backend.instrument import Instrument
from jv2backend.io.isis.xmljournalreader import ISISXMLJournalReader

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


@pytest.mark.parametrize(
    "content",
    ["", "{}", PARTIAL_RUN],
)
def test_reader_raises_ValueError_for_bad_data(content):
    reader = ISISXMLJournalReader(Instrument("fake"))

    with pytest.raises(SyntaxError):
        reader.read(content)


def test_reader_raises_ValueError_with_incorrect_file_name_attribute():
    reader = ISISXMLJournalReader(instrument=Instrument(SAMPLE_JOURNAL_INST))

    with pytest.raises(ValueError, match="Cannot parse cycle"):
        reader.read(BAD_CYCLE_FORMAT)


def test_reader_returns_journal_with_expected_attributes(sample_journal_xml):
    reader = ISISXMLJournalReader(instrument=Instrument(SAMPLE_JOURNAL_INST))

    journal = reader.read(sample_journal_xml)

    assert journal.cycle == Cycle(2021, 1)
    assert journal.run_count() == 3


# Test list
#   test read from file for different instrument
