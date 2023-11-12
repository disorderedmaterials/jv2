# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
import logging
import os.path
import h5py
import hashlib
import datetime
import json
import time
from jv2backend.utils import jsonify, url_join
from jv2backend.journalLibrary import JournalLibrary
from jv2backend.journalCollection import JournalCollection
from jv2backend.journal import Journal, SourceType
import jv2backend.userCache
from threading import Thread


# Threading
class GeneratorThread(Thread):
    def __init__(self, wait_time: int):
        Thread.__init__(self, daemon=True)
        self._wait_time = wait_time
        self._counter = wait_time
        self._destroy = False


class JournalGenerator:
    """Journal file generator"""

    def __init__(self) -> None:
        self._total_files_to_scan: int = None
        self._discovered_files: typing.Dict[str, typing.Any] = {}
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

    def list_files(self, data_directory: str) -> str:
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
                    if rootDir not in self._discovered_files:
                        self._discovered_files[rootDir] = []
                    self._discovered_files[rootDir].append(f)

        num_files = sum(len(runs) for runs in self._discovered_files.values())
        logging.debug(f"Found {num_files} NeXus files over \
                      {len(self._discovered_files)} unique directories \
                      in root directory {data_directory}.")

        return json.dumps(
            {
                "num_files": num_files,
                "data_directory": data_directory
            }
        )

    def create_journal_entry(self, data_directory: str, filename: str) -> {}:
        """Extract values from the supplied NeXuS file to form a journal
        'entry' for the run.

        :param data_directory: Directory containing the NeXuS file.
        :param filename: NeXuS filename
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

    def _get_nexus_file_data(self):
        # Iterate over available data files and get their attributes
        all_run_data = []
        for directory in self._discovered_files:
            logging.debug(f"Probing files in directory {directory}...")
            for f in self._discovered_files[directory]:
                logging.debug(f"... {f}")
                # Get attributes from the file
                # all_run_data.append(self.create_journal_entry(directory, f))
                time.sleep(10)

    def scan(self):
        """Generate an index file containing journal information

        :param collection: Target collection for new journals
        :param sort_key: Attribute on which to sort run data into journals

        Search all folders in the data file directory and generate a set of
        journal files describing the found data.
        """
        # Iterate over available data files and get their attributes
        thread = Thread(target=self._get_nexus_file_data(), daemon=True)
        thread.start()
        logging.debug("Noe we are here.")

    def generate(self, collection: JournalCollection, sort_key: str) -> str:
        all_run_data = []
        # Sort run data into sets by sort key, constructing suitable dicts for
        # direct inclusion in our generated Journal classes
        data_sets = {}
        for run in all_run_data:
            # If the sorting key isn't already in the dict, add it now
            if run[sort_key] not in data_sets:
                data_sets[run[sort_key]] = {}

            run_number = int(run["run_number"])
            data_sets[run[sort_key]][run_number] = run

        # Create index and journal files, and assemble a list of journals
        for key in data_sets:
            logging.debug(f"Data set for {key} has {len(data_sets[key])} runs")

            # Create hash and journal filename
            journal_filename = hashlib.sha256(key.encode('utf-8')).hexdigest() + ".xml"
            display_name = key.removeprefix(collection.run_data_url).lstrip("/")

            # Push a new Journal on to the list
            collection.add_journal(
                display_name,
                journal_filename,
                key,
                data_sets[key]
            )

        # Store in the user cache
        jv2backend.userCache.put_data(collection.library_key,
                                      collection.get_index_file_url(),
                                      collection.to_basic_json(),
                                      datetime.datetime.now())
        for journal in collection.journals:
            jv2backend.userCache.put_data(collection.library_key,
                                          journal.filename,
                                          json.dumps(journal.run_data),
                                          datetime.datetime.now())

        return json.dumps("SUCCESS")
