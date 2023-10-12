# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from datetime import datetime
from typing import MutableMapping, Optional
from io import BytesIO
from functools import reduce
import pandas as pd
import requests

from jv2backend.instrument import Instrument
from jv2backend.journal import Journal, concatenate
from jv2backend.journalFileList import JournalFileList
from jv2backend.io.journals.xmlJournalReader import ISISXMLJournalReader

class NetworkJournalLocator:
    """Journal file locator"""

    JOURNAL_FILELIST_FILENAME = "journal_main.xml"
    JOURNAL_FILENAME_TEMPLATE = "journal_{}.xml"
    LAST_MODIFIED_DT_FORMAT = "%a, %d %b %Y %H:%M:%S %Z"

    @classmethod
    def filename(cls, cyclename: str):
        """Return the Journal filename from a cycle name
        :param cyclename: Name of the cycle in the form [cycle_]YY_N
        :return: Construct the journal filename as journal_YY_N.xml
        """
        cycleid = cyclename.lstrip("cycle_") if cyclename.startswith("cycle") else cyclename
        return cls.JOURNAL_FILENAME_TEMPLATE.format(cycleid)

    def __init__(self, root_url: str):
        """
        :param root_url: An optional root URL for the server to replace the default
        """
        self._root_url = root_url
        # Modification times of journal files and index file
        self._journal_files_last_modified: MutableMapping[str, datetime] = dict()

    def get_index(self, server_root: str, journal_directory: str, index_file: str) -> JournalFileList:
        """Retrive an index file containing journal information

        It is expected that index file is "ISIS standard" XML and structured
        in the following way:
        
        <journal>
           <file name="journal.xml"/>
           <file name="journal_YY_N.xml"/>
           ...
        </journal>

        The first entry (journal.xml) is not relevant and should not be
        returned as a valid result. Other files represent journals
        corresponding to specific years (YY) and cycle integers (N)
        and which can be expected to reside in the same directory as
        the index file.

        :param root_url: Root server URL to make request to
        :param journal_directory: Directory containing journal index file
        :param index_file: Name of index file containing all available journals
        :return: The list of journal filenames as strings or an Exception
        """
        url = self._url_join(server_root, journal_directory, index_file)
        response = requests.get(url)
        response.raise_for_status()

        # Store the last modified time of the index file for future reference
        self._store_last_modified_time(
            url, response.headers["Last-Modified"]
        )

        # Parse the journal index file
        data = pd.read_xml(BytesIO(response.content), xpath="/journal/file", dtype=str)

        # Construct list of valid journal files for return
        journals = JournalFileList()
        for name in data["name"]:
            if not name == "journal.xml":
                journals.append(name)

        return journals

    def get_journal(self, server_root: str, journal_directory: str, journal_file: str) -> Journal:
        """Retrieve a journal file containing run information

        :param root_url: Root server URL to make request to
        :param journal_directory: Directory containing journal file
        :param journal_file: Name of journal file to retrieve
        :return: Array of run data information
        """
        url = self._url_join(server_root, journal_directory, journal_file)
        response = requests.get(url)
        response.raise_for_status()

        # Store the last modified time of the journal file for future reference
        self._store_last_modified_time(
            url, response.headers["Last-Modified"]
        )

        reader = ISISXMLJournalReader(Instrument("NOTMYNAME"))
        return reader.read_journalfile(response.content)

    def filename_for_run(self, instrument: str, run: str) -> Optional[str]:
        """Find the journal file that contains the given run

        :param instrument: The instrument name
        :param run: Run number
        :return: Filename str or None if the run cannot be found
        """
        # We do not use the search method as it is more likely a
        # user will request a recent run and we want to break when this is found
        filename = None
        for filename in reversed(self.journal_filenames(instrument)):
            journal = self.journal(instrument, filename=filename).run(run)
            if journal is not None:
                filename = filename
                break

        return filename

    def check_for_journal_filenames_update(self, instrument_name: str) -> Optional[str]:
        """Check if the journal index files has been modified since last checked
        and return the latest entry if it has, otherwise return None

        :param instrument_name: The name of the instrument
        :return: The latest journal filename if there have been updates to the main list
        """
        response = requests.head(self._mainfile_url(instrument_name))
        last_modified_on_server = self._to_datetime(response.headers["Last-Modified"])
        last_modified_here = self._journal_files_last_modifieds.get(instrument_name, None)
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
            journal = self.journal(instrument_name, filename=filename)
            results.append(journal.search(run_field, user_input, case_sensitive))

        return concatenate(results)

    # private
    def _join_slash(self, a: str, b: str):
        return a.rstrip('/') + '/' + b.lstrip('/')

    def _url_join(self, *args):
        return reduce(self._join_slash, args) if args else ''

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
        return self._root_url + f"/{instrument_name.lower()}"

    def _store_last_modified_time(
        self, instrument_name: str, last_modified_ts: str
    ) -> datetime:
        """
        :param instrument_name: Name of the instrument
        :param last_modified_ts: Timestamp of the last modification time as a str
        """
        dt = self._to_datetime(last_modified_ts)
        self._journal_files_last_modified[instrument_name] = dt
        return dt

    @classmethod
    def _to_datetime(cls, timestamp: str) -> datetime:
        """_summary_

        :param timestamp: A timestamp as a string
        :return: A datetime.datetime object
        """
        return datetime.strptime(timestamp, cls.LAST_MODIFIED_DT_FORMAT)
