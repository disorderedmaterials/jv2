# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from typing import Optional
import requests

from jv2backend.instrument import Instrument
from jv2backend.journalfilelist import JournalFileList
from jv2backend.io.journalserver import JournalServer
from jv2backend.io.isis.xmljournalreader import ISISXMLJournalReader



class ISISJournalServer(JournalServer):
    """Interface to the ISIS journal server to retrieve
    information

    """

    ROOT_URL_DEFAULT = "http://data.isis.rl.ac.uk/journals"
    JOURNAL_FILELIST_FILENAME = "journal_main.xml"

    def __init__(self, root_url: Optional[str] = None):
        """
        :param root_url: An optional root URL for the server to replace the default
        """
        self._root_url = root_url if root_url is not None else self.ROOT_URL_DEFAULT

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

    # private
    def _mainfile_url(self, instrument_name: str) -> str:
        """Return the URL for the _main index journal file

        :param instrument_name: The name of the instrument
        :return: A string containing the full URL
        """
        return self._root_url + f"/ndx{instrument_name.lower()}/{self.JOURNAL_FILELIST_FILENAME}"
