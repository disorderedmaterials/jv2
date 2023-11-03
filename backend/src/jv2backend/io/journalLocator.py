# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
from io import BytesIO
from jv2backend.utils import url_join, lm_to_datetime
import xml.etree.ElementTree as ElementTree
from flask import make_response
from flask.wrappers import Response as FlaskResponse
import datetime
import requests
import logging
import os.path
import lxml

from jv2backend.requestData import RequestData, SourceType
from jv2backend.journals import JournalCollection, JournalLibrary
from jv2backend.journalFile import JournalFile, JournalData
from jv2backend.utils import jsonify, json_response


class JournalLocator:
    """Journal file locator"""

    @classmethod
    def _get_journal_file_xml(cls, requestData: RequestData) -> ElementTree:
        """Get the content of the file and create an ElementTree"""
        if requestData.source_type == SourceType.Network:
            response = requests.get(requestData.journal_file_url(), timeout=3)
            response.raise_for_status()
            return ElementTree.parse(BytesIO(response.content))
        elif requestData.source_type == SourceType.Cached:
            if requestData.journal_collection is None:
                return ElementTree.parse("{}")
        elif requestData.source_type == SourceType.File:
            with open(requestData.file_url, "rb") as file:
                buffer = BytesIO(file.read())
            return ElementTree.parse(buffer)
        else:
            raise RuntimeError("No source type set.")

    @classmethod
    def _get_journal_modification_time(
            cls, source_type: SourceType, journal_file_url: str
) -> datetime.datetime:
        """Get the modification time of the journal"""
        if source_type == SourceType.Network:
            response = requests.head(journal_file_url, timeout=3)
            response.raise_for_status()
            return lm_to_datetime(response.headers["Last-Modified"])
        elif source_type == SourceType.Cached:
            # TODO / FIXME Need to handle this in a better way
            return datetime.datetime.now()
        elif source_type == SourceType.Disk:
            return datetime.datetime.fromtimestamp(
                os.path.getmtime(journal_file_url),
                datetime.timezone.utc
             )
        else:
            raise RuntimeError("No source type set.")

    @classmethod
    def _get_journal_run_data(
            cls, source_type: SourceType, journal_file_url: str
    ) -> ElementTree.Element:
        """Get the content of the file and parse it with ElementTree"""
        tree = ElementTree
        if source_type == SourceType.Network:
            response = requests.get(journal_file_url, timeout=3)
            response.raise_for_status()
            tree = ElementTree.parse(BytesIO(response.content))
        elif source_type == SourceType.Cached:
            # TODO / FIXME Need to handle this in a better way
            tree = ElementTree
        elif source_type == SourceType.File:
            with open(journal_file_url, "rb") as file:
                tree = ElementTree.parse(BytesIO(file.read()))
        else:
            raise RuntimeError("No source type set.")

        return tree.getroot()

    def get_index(self, requestData: RequestData,
                  journalLibrary: JournalLibrary) -> FlaskResponse:
        """Retrieve an index file containing journal information

        :param requestData: RequestData object containing index file details
        :return: A JSON response with the journal list or an error string

        The index xml file contains a simple list of full journal files in the
        same directory:

        <journal>
           <file name="journal.xml"/>
           <file name="journal_YY_N.xml"/>
           ...
           <file name="hash1.xml" display_name="My Data"
                 data_directory="/data"/>           # JV2-generated entry
        </journal>

        The first entry (journal.xml) is not relevant and should not be
        returned as a valid result. Other files represent journals
        corresponding to directory locations or, in the case of the ISIS
        standard journals, specific years (YY) and cycle integers (N).
        These journal files are expected to reside in the same directory as
        the index file.

        -- Display Name --

        The display name of the journal is given in the 'display_name'
        attribute. If not specified, a display name is generated from the
        journal filename, assuming that it is ISIS standard.

        -- Run Data Location --

        The location on disk of the associated run data for the journal is
        given in the 'data_directory' attribute. If not present, it is
        assumed that the location follows the ISIS Archive format, e.g.:

        /data_directory/NDXINSTRUMENT/Instrument/data/cycle_YY_M

        The 'instrument' in this case is expected to be the `directory`
        parameter passed in the requestData object.
        """
        # Retrieve the specified index as ElementTree data
        try:
            indexTree = self._get_journal_file_xml(requestData)
        except FileNotFoundError:
            return make_response(jsonify("Index File Not Found"), 200)
        except (requests.HTTPError, requests.ConnectionError) as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)
        except lxml.etree.XMLSyntaxError as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        indexRoot = indexTree.getroot()

        # Construct list of valid journal files for return
        journals = []
        for j in indexRoot.iter("file"):
            # Skip the "journal.xml" or malformed entries
            if "name" not in j.attrib or j.attrib["name"] == "journal.xml":
                continue

            # Determine display name
            if "display_name" in j.attrib:
                displayName = j.attrib["display_name"]
            else:
                displayName = j.attrib["name"].replace("journal", "Cycle")
                displayName = displayName.replace(".xml", "").replace("_", " ")

            # Determine data directory
            if "data_directory" in j.attrib:
                dataDirectory = j.attrib["data_directory"]
            else:
                cycleDir = j.attrib["name"].replace("journal", "cycle")
                cycleDir.replace(".xml", "")
                dataDirectory = url_join(
                    requestData.run_data_root_url,
                    requestData.directory,
                    "Instrument",
                    "data",
                    cycleDir)

            # Append to our list
            journals.append(
                JournalFile(
                    displayName,
                    requestData.journal_root_url,
                    requestData.directory,
                    j.attrib["name"], dataDirectory))

        # Store as a new collection in the library
        journalLibrary[requestData.library_key()] = JournalCollection(journals)

        return make_response(
            jsonify(journalLibrary[requestData.library_key()].to_basic()), 200
        )

    def get_journal_data(self, requestData: RequestData) -> FlaskResponse:
        """Retrieve run data contained in a journal file

        :param requestData: RequestData object containing journal details
        :return: Array of run data information
        """
        # Search the collection for the specified journal file
        j = requestData.journal_collection.get_journal(
            requestData.journal_filename
        )

        # For cached sources, we either return the found data immediately or
        # raise an error
        if requestData.source_type == SourceType.Cached:
            if j is not None:
                return make_response(j.run_data.to_json(), 200)
            else:
                return make_response(
                    jsonify({"Error": "Cached journal not found."}), 200
                )

        # If we already have this journal file in the collection, check its
        # modification time
        if j is not None:
            # Return current data if the file still has the same modtime
            current_last_modified = self._get_journal_modification_time(
                requestData.source_type,
                requestData.journal_file_url()
            )
            if current_last_modified == j.last_modified:
                return make_response(j.run_data.to_json(), 200)

        # Not up-to-date, so get the full file content and update modtime
        try:
            treeRoot = self._get_journal_run_data(requestData.source_type,
                                                  requestData.journal_file_url())
        except (requests.HTTPError, requests.ConnectionError,
                FileNotFoundError) as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)
        except lxml.etree.XMLSyntaxError as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Store the updated run data and modtime
        j.run_data = JournalData.from_element_tree(treeRoot)
        j.last_modified = current_last_modified

        # Store the most-recent (highest) run number in the journal for future
        # reference
        j.last_run_number = j.run_data.get_last_run_number

        return make_response(j.run_data.to_json(), 200)

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
        j = requestData.journal_collection.get_journal(
            requestData.journal_filename
        )
        if j is not None:
            # Compare modification times - if the same, return existing data
            current_last_modified = self._get_journal_modification_time(
                requestData.source_type,
                requestData.journal_file_url()
            )
            if current_last_modified == j.last_modified:
                return make_response(jsonify(None), 200)

        # Changed, so read full data and store the whole thing
        try:
            treeRoot = self._get_journal_run_data(requestData.source_type,
                                                  requestData.journal_file_url())
        except (requests.HTTPError, requests.ConnectionError,
                FileNotFoundError) as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)
        j.last_modified = current_last_modified
        j.run_data = JournalData.from_element_tree(treeRoot)

        # Get the last run number
        old_last_run_number = j.last_run_number
        current_last_run_number = j.run_data.get_last_run_number
        if old_last_run_number == current_last_run_number:
            return None
        j.last_run_number = current_last_run_number

        # Get new run data
        return make_response(json_response(j.run_data.search("run_number",
                                               f">{old_last_run_number}")), 200)

    def filename_for_run(self, instrument: str, run: str) -> typing.Optional[str]:
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
