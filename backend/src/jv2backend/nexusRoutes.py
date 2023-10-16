# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access NeXuS information"""
import logging
from pathlib import Path
from typing import Optional, Sequence

from flask import Flask, jsonify, request

from jv2backend.io.journals.networkLocator import NetworkJournalLocator
from jv2backend.io.runDataFileLocator import RunDataFileLocator
from jv2backend.journalClasses import JournalLibrary
from jv2backend.utils import json_response, split, url_join
import jv2backend.io.nexus as nxs


def add_routes(
    app: Flask,
    networkJournalLocator: NetworkJournalLocator,
    run_locator: RunDataFileLocator,
    journalLibrary: JournalLibrary
) -> Flask:
    """Add routes to the given Flask application."""

    @app.post("/runData/nexus/getLogValues")
    def getLogValues():
        """Return a list of the available log fields within the run

        The POST data should contain:
            rootUrl: The root network or disk location for the journals
          directory: The directory in rootUrl containing the journals
         runNumbers: Array of run numbers to probe for SE log values

        :return: A JSON response with the list of full paths to log fields
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        runNumbers = data["runNumbers"]
        library_key = url_join(rootUrl, directory)

        # Get the specified collection
        collection = journalLibrary[library_key]
        if collection is None:
            return jsonify(f"Error: Collection {library_key} does not exist.")

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(runNumbers)
        logging.debug(dataFiles)
        logpaths = []
        for run in dataFiles:
            if dataFiles[run] is None:
                return jsonify(f"Error: Unable to find run file for run \
                               '{run}'")
            logpaths.extend(nxs.logpaths_from_path(dataFiles[run]))

        return json_response(logpaths)

    @app.post("/runData/nexus/getLogValueData")
    def getLogValueData():
        """Return log value data specified for one or more run numbers.

        The POST data should contain:
            rootUrl: The root network or disk location for the journals
          directory: The directory in rootUrl containing the journals
         runNumbers: Array of run numbers to probe for SE log values

        :return: A list of the log data
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        log_value = data["logValue"]
        run_numbers = data["runNumbers"]
        library_key = url_join(rootUrl, directory)

        # Get the specified collection
        collection = journalLibrary[library_key]
        if collection is None:
            return jsonify(f"Error: Collection {library_key} does not exist.")

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(run_numbers)

        # Retrieve the log value data
        log_value_data = {}
        for run in dataFiles:
            if dataFiles[run] is None:
                return jsonify(f"Error: Unable to find run file for run \
                               '{run}'")

            runData = {}
            nxsfile, first_group = nxs.open_at(dataFiles[run], 0)

            runData["runNumber"] = str(run)
            runData["timeRange"] = [nxs.timerange(first_group)]
            runData["data"] = nxs.logvalues(first_group[log_value])

            log_value_data[run] = runData

        # Construct the final return object
        result = {}
        result["logValue"] = log_value
        result["runNumbers"] = run_numbers
        result["data"] = log_value_data

        return json_response(result)

    @app.post("/runData/nexus/getSpectrumRange")
    def getSpectrumRange():
        """Return the number of detector spectra for the run.

        The POST data should contain:
            rootUrl: The root network or disk location for the journals
          directory: The directory in rootUrl containing the journals
          runNumber: Run number to probe for detector count

        :return: The number of available detectors
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        run_number = data["runNumber"]
        library_key = url_join(rootUrl, directory)

        # Get the specified collection
        collection = journalLibrary[library_key]
        if collection is None:
            return jsonify(f"Error: Collection {library_key} does not exist.")

        # Locate data file for the specified run number in the collection
        dataFile = collection.locate_data_file(run_number)
        if dataFile is None:
            return jsonify(f"Error: Unable to find data file for run \
                           {run_number}")

        return str(nxs.spectra_count(dataFile))

    @app.post("/runData/nexus/getMonitorRange")
    def getMonitorRange():
        """Return the number of monitor spectra for the run.

        The POST data should contain:
            rootUrl: The root network or disk location for the journals
          directory: The directory in rootUrl containing the journals
          runNumber: Run number to probe for monitor count

        :return: The number of available monitors
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        run_number = data["runNumber"]
        library_key = url_join(rootUrl, directory)

        # Get the specified collection
        collection = journalLibrary[library_key]
        if collection is None:
            return jsonify(f"Error: Collection {library_key} does not exist.")

        # Locate data file for the specified run number in the collection
        dataFile = collection.locate_data_file(run_number)
        if dataFile is None:
            return jsonify(f"Error: Unable to find data file for run \
                           {run_number}")

        return str(nxs.monitor_count(dataFile))

    @app.post("/runData/nexus/getSpectrum")
    def getSpectrum():
        """Return detector spectrum for one or more run numbers.

        The POST data should contain:
            rootUrl: The root network or disk location for the journals
          directory: The directory in rootUrl containing the journals
         runNumbers: Array of run numbers to work on
         spectrumId: Target detector spectrum to return

        :return: A list of the detector spectra
        """

        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        spectrum_id = data["spectrumId"]
        run_numbers = data["runNumbers"]
        library_key = url_join(rootUrl, directory)

        # Get the specified collection
        collection = journalLibrary[library_key]
        if collection is None:
            return jsonify(f"Error: Collection {library_key} does not exist.")

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(run_numbers)

        # first entry matches sata expectation of the frontend
        spectra = [[run_numbers, spectrum_id, "detector"]]
        for run in dataFiles:
            if run is not None:
                spectra.append(nxs.spectrum(dataFiles[run], int(spectrum_id)))

        return json_response(spectra)

    @app.post("/runData/nexus/getMonitorSpectrum")
    def getMonitorSpectrum():
        """Return monitor spectrum for one or more run numbers.

        The POST data should contain:
            rootUrl: The root network or disk location for the journals
          directory: The directory in rootUrl containing the journals
         runNumbers: Array of run numbers to work on
         spectrumId: Target monitor spectrum to return

        :return: A list of the monitor spectra
        """

        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        spectrum_id = data["spectrumId"]
        run_numbers = data["runNumbers"]
        library_key = url_join(rootUrl, directory)

        # Get the specified collection
        collection = journalLibrary[library_key]
        if collection is None:
            return jsonify(f"Error: Collection {library_key} does not exist.")

        # Locate data files for the specified run numbers in the collection
        dataFiles = collection.locate_data_files(run_numbers)

        # first entry matches data expectation of the frontend
        spectra = [[run_numbers, spectrum_id, "monitor"]]
        for run in dataFiles:
            if run is not None:
                spectra.append(nxs.monitor_spectrum(dataFiles[run],
                                                    int(spectrum_id)))

        return json_response(spectra)

    @app.post("/runData/nexus/getDetectorAnalysis")
    def getDetectorAnalysis(instrument, cycle, run):
        """Determine the number of spectra with non-zero signal values

        :param instrument: The instrument name
        :param cycle: The cycle containing the run
        :param run: The run to analyse
        :return: A string of the form "count(non_zero)/count(all_spectra)"
        """
        filepaths = _locate_run_files(
            networkJournalLocator, run_locator, instrument, cycle, run
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {run}")

        return nxs.nonzero_spectra_ratio(filepaths[0])  # type: ignore

    # ------------------------ End Routes -------------------------

    return app


# Private functions


def _locate_run_files(
    networkJournalLocator: NetworkJournalLocator,
    run_locator: RunDataFileLocator,
    instrument: str,
    cycles_str: str,
    runs_str: str,
) -> Sequence[Optional[Path]]:
    """Return a list of Path objects to files for the given query

    :param file_locator: An object that understands how to locate a Run
    :param instrument: Instrument name
    :param cycles: A semi-colon separated list of cycle names in the format YY_N
    :param runs: A semi-colon separated list of run numbers. Length should match cycles.
    :return: A list of Path|None to the found data files. None indicates the files could not be found

    The instrument_name entries in the returned dicts are set to the input instrument name in order
    to pass the correct name (e.g. 'ndxabc') to the run locator.
    """
    logging.debug(f"Locating runs: '{cycles_str}'/'{runs_str}'")
    cycles, runs = split(cycles_str, ";"), split(runs_str, ";")
    if len(cycles) == 1:
        # Use the same cycle for each run
        cycles = [cycles[0]] * len(runs)

    filepaths = []
    for cycle, run in zip(cycles, runs):
        logging.debug(f"Fetching journal for {instrument}, cycle={cycle}")
        journal = networkJournalLocator.journal(instrument, cyclename=cycle)
        run = journal.run(run_number=run)
        if run is None:
            filepaths.append(None)
            continue
        logging.debug(f"Found metadata for run {run}")
        run["instrument_name"] = instrument
        filepaths.append(run_locator.locate(run))

    return filepaths
