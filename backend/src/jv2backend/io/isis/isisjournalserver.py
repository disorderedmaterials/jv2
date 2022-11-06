# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from typing import Optional
import requests

from jv2backend.instrument import Instrument
from jv2backend.journal import Journal
from jv2backend.journalfilelist import JournalFileList
from jv2backend.io.journalserver import JournalServer
from jv2backend.io.isis.xmljournalreader import ISISXMLJournalReader


class ISISJournalServer(JournalServer):
    """Interface to the ISIS journal server to retrieve
    information."""

    JOURNAL_FILELIST_FILENAME = "journal_main.xml"

    def __init__(self, root_url: str):
        """
        :param root_url: An optional root URL for the server to replace the default
        """
        self._root_url = root_url

    def journal_filenames(self, instrument_name: str) -> Optional[JournalFileList]:
        """
        :param instrument_name: The instrument name
        :return: The list of journal filenames as strings
        """
        response = requests.get(self._mainfile_url(instrument_name))
        if response.status_code != 200:
            return None

        reader = ISISXMLJournalReader(Instrument(instrument_name))
        return reader.read_indexfile(response.content)

    def journal(self, instrument_name: str, filename: str) -> Optional[Journal]:
        """
        :param instrument_name: The instrument name
        :param filename: Filename of the cycle
        :return: The list of journal filenames as strings
        """
        response = requests.get(self._journalfile_url(instrument_name, filename))
        if response.status_code != 200:
            return None

        reader = ISISXMLJournalReader(Instrument(instrument_name))
        return reader.read_journalfile(response.content)

    # private
    def _mainfile_url(self, instrument_name: str) -> str:
        """Return the URL for the _main index journal file

        :param instrument_name: The name of the instrument
        :return: A string containing the full URL
        """
        return f"{self._journal_root(instrument_name)}/{self.JOURNAL_FILELIST_FILENAME}"

    def _journalfile_url(self, instrument_name: str, filename: str) -> str:
        """Return the URL for jounal file given

        :param instrument_name: The name of the instrument
        :param filename: The filename of the journalfile
        :return: A string containing the full URL
        """
        return f"{self._journal_root(instrument_name)}/{filename}"

    def _journal_root(self, instrument_name: str) -> str:
        """Return the base url on the server for a given instrument

        :param instrument_name: _description_
        :return: The URL to the directory of the journals
        """
        return self._root_url + f"/ndx{instrument_name.lower()}"
