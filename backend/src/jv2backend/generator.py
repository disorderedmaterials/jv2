# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
import logging
import os.path
import h5py
import xml.etree.ElementTree as ElementTree
import hashlib

import datetime
import json
from flask import make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.requestData import RequestData
from jv2backend.utils import jsonify, url_join
from jv2backend.journalLibrary import JournalLibrary
from jv2backend.journalCollection import JournalCollection
from jv2backend.journal import Journal, SourceType
import jv2backend.userCache


class JournalGenerator:
    """Journal file generator"""

    def __init__(self) -> None:
        self._total_files_to_scan: int = None
        self._discovered_files: typing.Dict(str, typing.Any) = {}
        # Dicts of descriptors we wish to extract from the NeXuS file, and the
        # journal attributes they map to
        self._nxs_strings = {
            "/raw_data_1/beamline": "instrument_name",
            "/raw_data_1/title": "title",
            "/raw_data_1/user_1/name": "user_name",
            "/raw_data_1/experiment_identifier": "experiment_identifier",
            "/raw_data_1/start_time": "start_time",
            "/raw_data_1/end_time": "end_time"
        }
        self._nxs_numericals = {
            "/raw_data_1/good_frames": "good_frames",
            "/raw_data_1/raw_frames": "raw_frames",
            "/raw_data_1/run_number": "run_number",
            "/raw_data_1/duration": "duration",
            "/raw_data_1/proton_charge": "proton_charge"
        }

    def list_files(self, data_directory: str) -> FlaskResponse:
        """List available NeXuS files in a directory.

        :param data_directory: Target data directory to scan
        :return: A JSON response with the number of NeXuS files located
        """
        # Check if a scan is already in progress
        # TODO

        # First step, create a dict of all available nxs files in the target
        # directory, organised by folder name
        self._discovered_files = {}
        for rootDir, dirs, files in os.walk(data_directory):
            for f in files:
                if f.lower().endswith(".nxs"):
                    if (rootDir not in self._discovered_files):
                        self._discovered_files[rootDir] = []
                    self._discovered_files[rootDir].append(f)

        num_files = sum(len(runs) for runs in self._discovered_files.values())
        logging.debug(f"Found {num_files} NeXus files over \
                      {len(self._discovered_files)} unique directories \
                      in root directory {data_directory}.")

        # Create the response
        response = {}
        response["num_files"] = num_files
        response["data_directory"] = data_directory

        return make_response(jsonify(response), 200)

    def create_journal_entry(self, data_directory: str, filename: str) -> {}:
        """Extract values from the supplied NeXuS file to form a journal
        'entry' for the run.

        :param filepath: Full path to the NeXuS file.
        :return: A dict of all run attributes
        """
        nxs = h5py.File(url_join(data_directory, filename))

        # Basic data
        data = {
            "data_directory": data_directory,
            "filename": filename
        }

        # String attributes
        for stringValue in self._nxs_strings:
            if stringValue in nxs:
                value = nxs[stringValue][0]
                data[self._nxs_strings[stringValue]] = value.decode('utf-8')

        # Numerical attributes
        for numValue in self._nxs_numericals:
            if numValue in nxs:
                value = nxs[numValue][0]
                data[self._nxs_numericals[numValue]] = str(value)

        return data

    def scan_files(self, request_data: RequestData,
                   journalLibrary: JournalLibrary):
        """Generate an index file containing journal information

        :param request_data: RequestData object containing the necessary info

        Search all folders in the data file directory and generate a set of
        journal files describing the found data.
        """
        # Iterate over available data files and get their attributes
        all_run_data = []
        for directory in self._discovered_files:
            logging.debug(f"Probing files in directory {directory}...")
            for f in self._discovered_files[directory]:
                logging.debug(f"... {f}")
                # Get attributes from the file
                all_run_data.append(self.create_journal_entry(directory, f))

        # Select the sorting key for the run data
        if request_data.parameter == "Directory":
            sort_key = "data_directory"
        elif request_data.parameter == "RBNumber":
            sort_key = "experiment_identifier"
        logging.debug(f"Sorting key is {sort_key}")

        # Sort run data into sets by sort key
        data_sets = {}
        for run in all_run_data:
            # If the sorting key isn't already in the dict, add it now
            if run[sort_key] not in data_sets:
                data_sets[run[sort_key]] = []
            data_sets[run[sort_key]].append(run)

        # Create index and journal files, and assemble a list of journals
        index_root = ElementTree.Element("journal")
        journals = []
        for key in data_sets:
            logging.debug(f"Data set for {key} has {len(data_sets[key])} runs")

            # Create hash and journal filename
            hash = hashlib.sha256(key.encode('utf-8'))
            journal_filename = hash.hexdigest() + ".xml"
            displayName = key.removeprefix(request_data.run_data_url).lstrip("/")

            # Construct the child journal data
            journal_root = ElementTree.Element("NXroot")
            journal_root.set("filename", journal_filename)
            for run in data_sets[key]:
                run_entry = ElementTree.SubElement(journal_root, "NXentry")
                run_entry.set("name", run["filename"].replace(".nxs", ""))
                for attribute in run:
                    data_entry = ElementTree.SubElement(run_entry, attribute)
                    data_entry.text = run[attribute]

            # Add an entry in the index file
            index_entry = ElementTree.SubElement(index_root, "file")
            index_entry.set("filename", journal_filename)
            index_entry.set("data_directory", key)
            index_entry.set("display_name", displayName)

            # Push a new Journal on to the list
            journal = Journal(
                displayName,
                SourceType.Generated,
                request_data.library_key(),
                url_join(request_data.journal_root_url, request_data.directory),
                journal_filename,
                key,
                datetime.datetime.now()
            )

            journal.set_run_data_from_element_tree(journal_root)

            journals.append(journal)

        # Create a library entry
        journalLibrary[request_data.library_key()] = JournalCollection(
            SourceType.Generated,
            request_data.library_key(),
            url_join(request_data.journal_root_url,
                     request_data.directory),
            "index.xml",
            url_join(request_data.run_data_root_url,
                     request_data.directory),
            datetime.datetime.now(),
            journals
        )

        # Store in the user cache
        jv2backend.userCache.put_data(request_data.library_key(),
                                      request_data.journal_file_url(),
                                      journalLibrary[request_data.library_key()].to_basic_json(),
                                      datetime.datetime.now())
        for journal in journals:
            jv2backend.userCache.put_data(request_data.library_key(),
                                          journal.filename,
                                          json.dumps(journal.run_data),
                                          datetime.datetime.now())

        return make_response(jsonify("SUCCESS"), 200)
