# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access NeXuS information"""
import logging
from pathlib import Path
from typing import Optional, Sequence

from flask import Flask, jsonify, request

from jv2backend.io.journals.networkLocator import NetworkJournalLocator
from jv2backend.io.runDataFileLocator import RunDataFileLocator
from jv2backend.journalClasses import JournalCollection, JournalLibrary
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
          directory: The directory in rootUrl to probe for journals
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
          directory: The directory in rootUrl to probe for journals
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

        # The front end expects a list where the first entry is a list of all available fields.
        # The assumption is all instruments offer the same fields so only the first is examined.
        # The subsequent entries are one additional entry per run:
        #   [(start_time, end_time), [(run, field1),(time1, val1),...], [(run, field2),(time1, val1),...]]
        all_field_data = []
        for run in dataFiles:
            if dataFiles[run] is None:
                return jsonify(f"Error: Unable to find run file for run '{run}'")

            nxsfile, first_group = nxs.open_at(dataFiles[run], 0)
            run_data = [nxs.timerange(first_group)]

            log_data = nxs.logvalues(first_group[log_value])  # type: ignore
            log_data.insert(0, (run, log_value))  # type: ignore
            run_data.append(log_data)  # type: ignore

            nxsfile.close()
            all_field_data.append(run_data)

        # Add the list of all available log paths required by the frontend
        all_field_data.insert(0, nxs.logpaths_from_path(dataFiles[86376]))  # type: ignore

        return json_response(all_field_data)

    @app.route("/runData/nexus/getSpectrumRange/<instrument>/<cycle>/<runs>")
    def getSpectrumRange(instrument, cycle, runs):
        """Return the number of spectra for the first run. The old
        api simply ignored anything past the first run

        :param instrument: The instrument name
        :param cycle: The cycle containing the runs
        :param runs: The run numbers
        :return: A
        """
        # To conform to the frontend expectation take only the first run supplied
        run = runs
        if ";" in runs:
            run = runs[: runs.index(";")]
        filepaths = _locate_run_files(
            networkJournalLocator, run_locator, instrument, cycle, run
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {run}")

        return str(nxs.spectra_count(filepaths[0]))  # type: ignore

    @app.route("/runData/nexus/getMonitorRange/<instrument>/<cycle>/<runs>")
    def getMonitorRange(instrument, cycle, runs):
        """Return the number of monitors for the first run. The old
        api simply ignored anything past the first run

        :param instrument: The instrument name
        :param cycle: The cycle containing the runs
        :param runs: The run numbers
        :return: The number of monitors encoded as a str
        """
        # To conform to the frontend expectation take only the first run supplied
        run = runs
        if ";" in runs:
            run = runs[: runs.index(";")]
        filepaths = _locate_run_files(
            networkJournalLocator, run_locator, instrument, cycle, run
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {runs}")

        return str(nxs.monitor_count(filepaths[0]))  # type: ignore

    @app.route("/runData/nexus/getSpectrum/<instrument>/<cycle>/<runs>/<spectrum>")
    def getSpectrum(instrument, cycle, runs, spectrum):
        """Return a spectrum from a given set of runs

        :param instrument: The name of the instrument
        :param cycle: The cycle containing the runs
        :param runs: The list of runs whose data is returned
        :param spectrum: The number of the spectrum to access
        :return:
        """
        filepaths = _locate_run_files(
            networkJournalLocator, run_locator, instrument, cycle, runs
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {runs}")

        # first entry matches expectation of the frontend with sending back the parameters
        data = [[runs, spectrum, "detector"]]
        data.extend([nxs.spectrum(filepath, int(spectrum)) for filepath in filepaths])  # type: ignore
        return json_response(data)

    @app.route("/runData/nexus/getMonitorSpectrum/<instrument>/<cycle>/<runs>/<monitor>")
    def getMonitorSpectrum(instrument, cycle, runs, monitor):
        """Return a spectrum from a monitor for a given set of runs

        :param instrument: The name of the instrument
        :param cycle: The cycle containing the runs
        :param runs: The list of runs whose data is returned
        :param spectrum: The number of the spectrum to access
        :return:
        """
        filepaths = _locate_run_files(
            networkJournalLocator, run_locator, instrument, cycle, runs
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {runs}")

        # first entry matches expectation of the frontend with sending back the parameters
        data = [[runs, monitor, "monitor"]]
        data.extend([nxs.spectrum(filepath, int(monitor)) for filepath in filepaths])  # type: ignore
        return json_response(data)

    @app.route("/runData/nexus/getDetectorAnalysis/<instrument>/<cycle>/<run>")
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
