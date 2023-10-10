# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines a reader class to read an ISIS Journal file"""
from io import BytesIO
import pandas as pd

from jv2backend.io.journalReader import JournalReader
from jv2backend.instrument import Instrument
from jv2backend.journal import Journal
from jv2backend.journalFileList import JournalFileList


class ISISXMLJournalReader(JournalReader):
    """A concrete type to read an XML-formatted Journal file"""

    def __init__(self, instrument: Instrument) -> None:
        """A Journal file is associated with an Instrument.
        This class reads the attributes into a Journal of runs

        :param instrument: The instrument associated with runs in this journal
        """
        super().__init__()
        self._instrument = instrument

    def read_indexfile(self, content: bytes) -> JournalFileList:
        """Read an XML-formatted index of journal names

        :param content: An bytes object containing the XML-formatted data
        :return: A new JournalFileIndex object
        """
        data = pd.read_xml(BytesIO(content), dtype=str)
        journals = JournalFileList()
        for name in data["name"]:
            journals.append(name)

        return journals

    def read_journalfile(self, content: bytes) -> Journal:
        """Read an XML-formatted iterable describing a Journal

        :param content: A bytes object containing XML describing the run content.
        :return: A Journal containing the parsed Run data
        """
        return Journal(self._instrument, data=pd.read_xml(BytesIO(content), dtype=str))
