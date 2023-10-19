# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
import logging
import os.path
import h5py
import xml.etree.ElementTree as ET

from jv2backend.requestData import RequestData
from jv2backend.utils import jsonify, url_join


class JournalGenerator:
    """Journal file generator"""

    def __init__(self) -> None:
        self._total_files_to_scan: int = None
        self._scanned_files: typing.Dict(str, typing.Any) = {}
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
        self._nxs_integers = {
            "/raw_data_1/good_frames": "good_frames",
            "/raw_data_1/raw_frames": "raw_frames",
            "/raw_data_1/run_number": "run_number"
        }
        self._nxs_doubles = {
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
        self._scanned_files = {}
        for rootDir, dirs, files in os.walk(data_directory):
            for f in files:
                if f.lower().endswith(".nxs"):
                    if (rootDir not in self._scanned_files):
                        self._scanned_files[rootDir] = []
                    self._scanned_files[rootDir].append(f)

        num_files = sum(len(runs) for runs in self._scanned_files.values())
        logging.debug(f"Found {num_files} NeXus files over \
                      {len(self._scanned_files)} unique directories \
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

        data = {
            "data_directory": data_directory,
            "filename": filename
        }
        for stringValue in self._nxs_strings:
            if stringValue in nxs:
                value = nxs[stringValue][0]
                data[self._nxs_strings[stringValue]] = value.decode('utf-8')

        return data

    def scan_files(self, requestData: RequestData):
        """Generate an index file containing journal information

        :param requestData: RequestData object containing index file details
        :return: A JSON response with the journal list or an error string

        Search all folders in the data file directory and generate a set of
        journal files and an accompanying index file describing the contents.
        """
        # Iterate over available data files and get their journal attributes
        runData = []
        for dir in self._scanned_files:
            logging.debug(f"Probing files in directory {dir}...")
            for f in self._scanned_files[dir]:
                logging.debug(f"... {f}")
                # Get attributes from the file
                runData.append(self.create_journal_entry(dir, f))

        print(runData)

        # Create organised journals for the run data
        journals = {}
        for run in runData:
            # If the data directory isn't already in the dict, add it now
            if run["data_directory"] not in journals:
                journals[run["data_directory"]] = []
            journals[run["data_directory"]].append(run)

        for j in journals:
            logging.debug(f"Journal for {j} has {len(journals[j])} runs")
            root = ET.Element("NXroot")
            for run in journals[j]:
                runEntry = ET.SubElement(root, "NXentry")
                for attr in run:
                    runEntry.set(attr, run[attr])

            ET.dump(root)

