# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from typing import Optional, List
from io import BytesIO
from jv2backend.utils import url_join, lm_to_datetime
import datetime
import pandas as pd
import requests
import logging
import os.path

from jv2backend.requestData import RequestData
from jv2backend.journals import JournalCollection, JournalFile
from jv2backend.journals import JournalData, JournalLibrary, concatenate
from jv2backend.utils import jsonify, json_response


class JournalLocator:
    """Journal file locator"""

    @classmethod
    def _get_file(cls, requestData: RequestData) -> BytesIO:
        """Get the content of the file"""
        if requestData.is_http:
            response = requests.get(requestData.file_url)
            response.raise_for_status()
            return BytesIO(response.content)
        else:
            with open(requestData.file_url, "rb") as file:
                buffer = BytesIO(file.read())
            return buffer

    @classmethod
    def _get_modification_time(cls,
                               requestData: RequestData) -> datetime.datetime:
        """Get the modification time of the file"""
        if requestData.is_http:
            response = requests.head(requestData.file_url)
            response.raise_for_status()
            return lm_to_datetime(response.headers["Last-Modified"])
        else:
            return os.path.getmtime(requestData.file_url)

    def get_index(self, requestData: RequestData,
                  journalLibrary: JournalLibrary) -> List[JournalFile]:
        """Retrieve an index file containing journal information

        :param requestData: RequestData object containing index file details
        :return: A JSON response with the journal list or an error string

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

        Furthermore, it is expected that the data directory layout whose
        root is the data_directory parameter is laid out as follows:

        /data_directory/NDXINSTRUMENT/Instrument/data/cycle_YY_M
        """
        # Retrieve the specified file, assumed to be an xml index file
        try:
            fileBytes = self._get_file(requestData)
        except requests.HTTPError as exc:
            return jsonify(f"Error: {str(exc)}")
        except FileNotFoundError:
            return jsonify("Index File Not Found")

        # Parse the journal index file
        data = pd.read_xml(fileBytes, xpath="/journal/file", dtype=str)

        # Set base data file location
        baseDataLocation = url_join(requestData.data_directory,
                                    requestData.directory,
                                    "Instrument", "data")

        # Construct list of valid journal files for return
        journals = []
        for name in data["name"]:
            if not name == "journal.xml":
                dirName = name.replace("journal", "cycle").replace(".xml", "")
                journals.append(
                    JournalFile(
                        requestData.root_url,
                        requestData.directory,
                        name, url_join(baseDataLocation, dirName)))

        # Store as a new collection in the library
        journalLibrary[requestData.url] = JournalCollection(journals)

        return jsonify(journalLibrary[requestData.url].to_basic())

    def get_journal_data(self, requestData: RequestData) -> JournalData:
        """Retrieve run data contained in a journal file

        :param requestData: RequestData object containing journal details
        :return: Array of run data information
        """
        # If we already have this journal file in the collection, check its
        # modification time
        j = requestData.journal_collection.get_info(requestData.filename)
        if j is not None:
            # Return current data if the file still has the same modtime
            current_last_modified = self._get_modification_time(requestData)
            if current_last_modified == j.last_modified:
                return json_response(j.run_data)

        # Not up-to-date, so get the full file content and update modtime
        try:
            fileBytes = self._get_file(requestData)
        except (requests.HTTPError, FileNotFoundError) as exc:
            return jsonify(f"Error: {str(exc)}")
        j.last_modified = current_last_modified

        # Read in the run data from the journal file
        j.run_data = JournalData(
            requestData.filename, data=pd.read_xml(fileBytes, dtype=str))

        # Store the most-recent (highest) run number in the journal for future
        # reference
        j.last_run_number = j.run_data.get_last_run_number

        return json_response(j.run_data)

    def get_all_journal_data(self, collection: JournalCollection) -> None:
        """Retrieve all run data for all journals listed in the collection

        :param collection: JournalCollection in which the data should be stored
        :return: None
        """
        # Loop over defined journal files. If run_data is already present we
        # assume it's up-to-date.
        for journal in [j for j in collection.journalFiles
                        if j.run_data is None]:
            logging.debug(f"Obtaining journal {journal.filename}")
            self.get_journal_data(collection, journal.server_root,
                                  journal.directory, journal.filename)

    def get_updates(self, requestData: RequestData) -> JournalData:
        """Check if the journal index files has been modified since the last
        retrieval and return new runs added after the last known.

        :param requestData: RequestData object containing journal details
        :return: Array of new run data information or None
        """
        # If we already have this journal file in the collection, check its
        # modification time
        j = requestData.journal_collection.get_info(requestData.filename)
        if j is not None:
            # Compare modification times - if the same, return existing data
            current_last_modified = self._get_modification_time(requestData)
            if current_last_modified == j.last_modified:
                return jsonify(None)

        # Changed, so read full data
        try:
            fileBytes = self._get_file(requestData)
        except (requests.HTTPError, FileNotFoundError) as exc:
            return jsonify(f"Error: {str(exc)}")
        j.last_modified = current_last_modified

        # Read in the run data from the journal file and store the whole thing
        j.run_data = JournalData(
            requestData.filename, data=pd.read_xml(fileBytes, dtype=str))

        # Get the last run number
        old_last_run_number = j.last_run_number
        current_last_run_number = j.run_data.get_last_run_number
        if old_last_run_number == current_last_run_number:
            return None
        j.last_run_number = current_last_run_number

        # Get new run data
        return json_response(j.run_data.search("run_number",
                                               f">{old_last_run_number}"))

    def filename_for_run(self, instrument: str, run: str) -> Optional[str]:
        """Find the journal file that contains the given run

        :param instrument: The instrument name
        :param run: Run number
        :return: Filename str or None if the run cannot be found
        """
        # We do not use the search method as it is more likely a
        # user will request a recent run and we want to break when this is
        # found
        filename = None
        for filename in reversed(self.journal_filenames(instrument)):
            journal = self.journal(instrument, filename=filename).run(run)
            if journal is not None:
                filename = filename
                break

        return filename

    def search(
        self,
        instrument_name: str,
        run_field: str,
        user_input: str,
        case_sensitive: bool = False,
    ) -> JournalData:
        """
        Search across all journals for the given user input against the given
        field in the journals

        :param instrument_name: The instrument name
        :param run_field: Field to search over from all runs
        :param user_input: Search query
        :param case_sensitive: If True, use case sensitive searching
        :return: A JournalData object of the runs matching the search query.
                 An empty object indicates nothing could be found
        """
        results = []
        for filename in self.journal_filenames(instrument_name):
            journal = self.journal(instrument_name, filename=filename)
            results.append(
                journal.search(
                    run_field,
                    user_input,
                    case_sensitive))

        return concatenate(results)
