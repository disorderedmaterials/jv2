# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
import logging
import os.path
import h5py
import xml.etree.ElementTree as ET

from jv2backend.requestData import RequestData
from jv2backend.utils import jsonify, url_join
from jv2backend.journals import JournalLibrary, JournalCollection


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

    def list_files(self, data_directory: str) -> int:
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

        return jsonify(response)

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

    def scan_files(self, requestData: RequestData,
                   journalLibrary: JournalLibrary):
        """Scan the current list of discovered files amd create journals
        accordingly.

        :param requestData: RequestData object containing the necessary info

        Search all folders in the data file directory and generate a set of
        journal files and an accompanying index file describing the contents.
        """
        # Iterate over available data files and get their journal attributes
        runData = []
        for dir in self._discovered_files:
            logging.debug(f"Probing files in directory {dir}...")
            for f in self._discovered_files[dir]:
                logging.debug(f"... {f}")
                # Get attributes from the file
                runData.append(self.create_journal_entry(dir, f))

        # Create organised journals for the run data
        journals = {}
        if requestData.parameter == "Directory":
            sortKey = "data_directory"
        elif requestData.parameter == "RBNumber":
            sortKey = "experiment_identifier"
        logging.debug(f"Sorting key is {sortKey}")

        for run in runData:
            # If the sorting key isn't already in the dict, add it now
            if run[sortKey] not in journals:
                journals[run[sortKey]] = []
            journals[run[sortKey]].append(run)

        # Create and write index and journal files
        indexRoot = ET.Element("journal")
        for j in journals:
            logging.debug(f"Journal for {j} has {len(journals[j])} runs")
            
            # Construct the child journal data
            journalRoot = ET.Element("NXroot")
            for run in journals[j]:
                runEntry = ET.SubElement(journalRoot, "NXentry")
                runEntry.set("name", run["filename"].replace(".nxs", ""))
                for d in run:
                    dataEntry = ET.SubElement(runEntry, d)
                    dataEntry.text = run[d]

            # Add an entry in the index file
            indexEntry = ET.SubElement(indexRoot, "file")
            indexEntry.set("data_directory", j)
            indexEntry.set("display_name",
                           j.removeprefix(
                               requestData.data_directory
                               ).lstrip("/"))

        # Store as a new collection in the library
        journalLibrary[requestData.url] = JournalCollection(journals)

        return jsonify("SUCCESS")
