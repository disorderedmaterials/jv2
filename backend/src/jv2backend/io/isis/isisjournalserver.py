# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from datetime import datetime
from typing import MutableMapping, Optional
import requests

from jv2backend.instrument import Instrument
from jv2backend.journal import Journal, concatenate
from jv2backend.journalfilelist import JournalFileList
from jv2backend.io.journalserver import JournalServer
from jv2backend.io.isis.xmljournalreader import ISISXMLJournalReader


class ISISJournalServer(JournalServer):
    """Interface to the ISIS journal server to retrieve
    information."""

    JOURNAL_FILELIST_FILENAME = "journal_main.xml"
    LAST_MODIFIED_DT_FORMAT = "%a, %d %b %Y %H:%M:%S %Z"

    def __init__(self, root_url: str):
        """
        :param root_url: An optional root URL for the server to replace the default
        """
        self._root_url = root_url
        # Store the last modified times of the _main journal files against the instrument names
        self._last_modified: MutableMapping[str, datetime] = dict()

    def journal_filenames(self, instrument_name: str) -> JournalFileList:
        """
        :param instrument_name: The instrument name
        :return: The list of journal filenames as strings or an Exception object
        """
        response = requests.get(self._mainfile_url(instrument_name))
        response.raise_for_status()
        self._store_last_modified_time(
            instrument_name, response.headers["Last-Modified"]
        )

        reader = ISISXMLJournalReader(Instrument(instrument_name))
        return reader.read_indexfile(response.content)

    def journal(self, instrument_name: str, filename: str) -> Journal:
        """
        :param instrument_name: The instrument name
        :param filename: Filename of the cycle
        :return: The list of journal filenames as strings
        """
        response = requests.get(self._journalfile_url(instrument_name, filename))
        response.raise_for_status()
        reader = ISISXMLJournalReader(Instrument(instrument_name))
        return reader.read_journalfile(response.content)

    def check_for_journal_filenames_update(self, instrument_name: str) -> Optional[str]:
        """Check if the journal index files has been modified since last checked
        and return the latest entry if it has, otherwise return None

        :param instrument_name: The name of the instrument
        :return: The latest journal filename if there have been updates to the main list
        """
        response = requests.head(self._mainfile_url(instrument_name))
        last_modified_on_server = self._to_datetime(response.headers["Last-Modified"])
        last_modified_here = self._last_modified.get(instrument_name, None)
        if last_modified_here is not None and (
            last_modified_on_server > last_modified_here
        ):
            return self.journal_filenames(instrument_name)[-1]
        else:
            return None

    def search(
        self,
        instrument_name: str,
        run_field: str,
        user_input: str,
        case_sensitive: bool = False,
    ) -> Journal:
        """
        Search across all journals for the given user input against the given field
        in the journals

        :param instrument_name: The instrument name
        :param run_field: Field to search over from all runs
        :param user_input: Search query
        :param case_sensitive: If True, use case sensitive searching
        :return: A Journal of the runs matching the search query. An empty journal
                 indicates nothing could be found
        """
        results = []
        for filename in self.journal_filenames(instrument_name):
            journal = self.journal(instrument_name, filename)
            results.append(journal.search(run_field, user_input, case_sensitive))

        return concatenate(results)

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

    def _store_last_modified_time(
        self, instrument_name: str, last_modified_ts: str
    ) -> datetime:
        """
        :param instrument_name: Name of the instrument
        :param last_modified_ts: Timestamp of the last modification time as a str
        """
        dt = self._to_datetime(last_modified_ts)
        self._last_modified[instrument_name] = dt
        return dt

    @classmethod
    def _to_datetime(cls, timestamp: str) -> datetime:
        """_summary_

        :param timestamp: A timestamp as a string
        :return: A datetime.datetime object
        """
        return datetime.strptime(timestamp, cls.LAST_MODIFIED_DT_FORMAT)