# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
from dataclasses import dataclass
import typing
from io import BytesIO
from jv2backend.utils import url_join, lm_to_datetime
import jv2backend.select as Selector
from jv2backend.journal import Journal
from jv2backend.requestData import RequestData, SourceType
import xml.etree.ElementTree as ElementTree
from flask import make_response, jsonify
from flask.wrappers import Response as FlaskResponse
import datetime
import logging
import json
import requests
import os.path
import lxml.etree as etree

@dataclass
class JournalCollection:
    """Defines a collection of journal files"""

    # The available journal files within a collection
    journalFiles: typing.List[Journal]

    def __init__(self, journalFiles: typing.List[Journal]) -> None:
        self.journalFiles = journalFiles

    # ---------------- File Operations

    @classmethod
    def _get_journal_modification_time(
            cls, source_type: SourceType, journal_file_url: str
    ) -> datetime.datetime:
        """Get the modification time of the journal"""
        if source_type == SourceType.Network:
            response = requests.head(journal_file_url, timeout=3)
            response.raise_for_status()
            return lm_to_datetime(response.headers["Last-Modified"])
        elif source_type == SourceType.Disk:
            return datetime.datetime.fromtimestamp(
                os.path.getmtime(journal_file_url),
                datetime.timezone.utc
            )
        else:
            raise RuntimeError(f"Can't handle source type {str(source_type)}.")

    @classmethod
    def _get_journal_run_data(
            cls, source_type: SourceType, journal_file_url: str
    ) -> (ElementTree.Element, datetime.datetime):
        """Get the content of the file and parse it with ElementTree"""
        tree = ElementTree
        modtime = None
        if source_type == SourceType.Network:
            response = requests.get(journal_file_url, timeout=3)
            response.raise_for_status()
            tree = ElementTree.parse(BytesIO(response.content))
            modtime = lm_to_datetime(response.headers["Last-Modified"])
        elif source_type == SourceType.File:
            with open(journal_file_url, "rb") as file:
                tree = ElementTree.parse(BytesIO(file.read()))
        else:
            raise RuntimeError(f"Can't handle source type {str(source_type)}.")

        return tree.getroot(), modtime

    # ---------------- Work Functions

    def get_journal_data(self, requestData: RequestData) -> FlaskResponse:
        """Retrieve run data contained in a journal file

        :param requestData: RequestData object containing journal details
        :return: Array of run data information
        """
        # Search the collection for the specified journal file
        j = self.get_journal(requestData.journal_filename)

        # For cached sources, we either return the found data immediately or
        # raise an error
        if requestData.source_type == SourceType.Cached:
            if j is not None:
                return make_response(j.get_run_data_as_json(), 200)
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
                return make_response(j.get_run_data_as_json(), 200)

        # Not up-to-date, so get the full file content and update modtime
        try:
            tree_root, modtime = self._get_journal_run_data(
                requestData.source_type,
                requestData.journal_file_url()
            )
        except (requests.HTTPError, requests.ConnectionError,
                FileNotFoundError) as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)
        except etree.XMLSyntaxError as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        # Store the updated run data and modtime
        j.set_run_data_from_element_tree(tree_root)
        j.last_modified = modtime

        return make_response(j.get_run_data_as_json(), 200)

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

    def get_updates(self, requestData: RequestData) -> FlaskResponse:
        """Check if the journal index files has been modified since the last
        retrieval and return new runs added after the last known.

        :param requestData: RequestData object containing journal details
        :return: Array of new run data information or None
        """
        # Search the collection for the specified journal file
        j = self.get_journal(requestData.journal_filename)

        # For cached sources, we return immediately either way
        if requestData.source_type == SourceType.Cached:
            if j is not None:
                return make_response(jsonify(None), 200)
            else:
                return make_response(
                    jsonify({"Error": "Cached journal not found."}), 200
                )

        # If we already have this journal file in the collection, check its
        # modification time, returning the current data if up-to-date
        if j is not None:
            current_last_modified = self._get_journal_modification_time(
                requestData.source_type,
                requestData.journal_file_url()
            )
            if current_last_modified == j.last_modified:
                return make_response(jsonify(None), 200)

        # Changed, so read full data and store the whole thing, storing the
        # current last run number before we set the new data
        old_last_run_number = j.get_last_run_number()
        try:
            tree_root, modtime = self._get_journal_run_data(
                requestData.source_type,
                requestData.journal_file_url()
            )
        except (requests.HTTPError, requests.ConnectionError,
                FileNotFoundError) as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)
        j.set_run_data_from_element_tree(tree_root)
        j.last_modified = modtime

        # If the old run numbers are the same
        if old_last_run_number == j.get_last_run_number():
            return make_response(jsonify(None), 200)

        # Return any new runs after the previous last known run number
        return make_response(
            Journal.convert_run_data_to_json(
                j.get_run_numbers_after(old_last_run_number)), 200
        )

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

    # ---------------- Helpers

    def journal_for_run(self, run_number: int) -> Journal:
        """Find the journal in the collection that contains the specified run
        number.

        :param run_number: Run number to locate
        :return: Journal containing the run number, or None if not found
        """
        return next(
            (jf for jf in self.journalFiles if run_number in jf),
            None)

    def locate_data_file(self, run_number: int) -> str:
        """Return the full path to the data (NeXuS) file for the specified
        run number
        :param run_number: Run number to locate the data file for
        :return: Full path to the data file or None if it couldn't be found
        """
        # Get the journal file for the specified run number
        jf = self.journal_for_run(run_number)
        if jf is None:
            return None
        logging.debug(f"Run number {run_number} exists in journal "
                      f"{jf.filename}")

        # Get the data for the specified run number
        data = jf.get_run(run_number)

        # The journal entry may contain the full data_directory and filename
        # information if we generated it. Otherwise we have to assume the
        # stored 'data_directory' and use the 'name' attribute.
        if "data_directory" in data and "filename" in data:
            return url_join(data["data_directory"], data["filename"])
        else:
            return url_join(jf.data_directory, data["name"] + ".nxs")

    def locate_data_files(
            self, run_numbers: typing.List[int]
    ) -> typing.Dict[int, str]:
        """Return a dict of run number/paths to NeXuS data files
        If a run number is not locatable, return None for that entry

        :param run_numbers: A list of integer run numbers to locate
        :return: A Dict[int,str] mapping of integer run number to either
                 to eithr NeXuS file path or None if not locatable
        """
        # For each of the supplied run numbers, find its parent journal
        result = {}
        for i in run_numbers:
            result[i] = self.locate_data_file(i)
        return result

    def get_journal(self, filename: str) -> Journal:
        return next(
            (jf for jf in self.journalFiles if jf.filename == filename),
            None)

    def to_basic_json(self) -> str:
        """Return basic journal information as formatted JSON"""
        basic = []
        for journal in self.journalFiles:
            basic.append(journal.get_journal_as_dict())
        return json.dumps(basic)

    def search(self, search_terms: {}) -> {}:
        """
        Search across all journals in the collection, selecting those which
        match _all_ of the specified search_terms. The search terms are
        applied sequentially in the order they appear in the dict.

        :param search_terms: Dict of search field/values
        :return: A dict of runs matching the search query.
        """
        results = {}

        # See if we have a case-sensitive flag
        case_sensitive = ("caseSensitive" in search_terms and
                          search_terms["caseSensitive"] == "true")
        if "caseSensitive" in search_terms:
            del search_terms["caseSensitive"]

        for jf in self.journalFiles:
            logging.debug(f"Journal {jf.filename} .....")
            if not jf.has_run_data:
                continue
            matches = None
            # Cycle over search terms
            # If the current 'matches' is None then search the whole run data
            # If it is not, then search it instead (chaining searches)
            # If it is ever a size of zero we have excluded all runs
            logging.debug("Starting loop over run data...")
            for field in search_terms:
                matches = Selector.select(jf.run_data if matches is None
                                          else matches,
                                          field,
                                          search_terms[field],
                                          case_sensitive)
                logging.debug(f"...after checking '{field}' there are "
                              f"{len(matches)} matches...")
                if len(matches) == 0:
                    break

            if matches is None:
                continue

            logging.debug(f"Journal {jf.filename} matched {len(matches)} runs.")
            results.update(matches)

        return results
