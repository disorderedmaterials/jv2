# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines a reader class to read a journal_main file that
provides an index of existing journal files"""
from typing import Sequence
from lxml import etree

from jv2backend.journalfilelist import JournalFileList


class ISISXMLJournalListReader:
    """Parse an XML file containing a list of names of journal files"""

    def read(self, content: bytes) -> JournalFileList:
        """Read an XML-formatted index of journal names

        :param content: An bytes object containing the XML-formatted data
        :return: A new JournalFileIndex object
        """
        try:
            root = etree.fromstring(content)
        except SyntaxError as exc:
            raise SyntaxError(f"Unable to parse content as XML:\n\t{str(exc)}") from exc

        journals = JournalFileList()
        for entry in root.getchildren():
            journals.append(entry.attrib["name"])

        return journals
