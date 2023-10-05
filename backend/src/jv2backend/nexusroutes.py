# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 Team JournalViewer and contributors
"""Defines the Flask endpoints supported by the server"""
import logging
from pathlib import Path
from typing import Optional, Sequence

from flask import Flask, jsonify

import jv2backend.config as config
from jv2backend.io.journalserver import JournalServer
from jv2backend.io.rundatafilelocator import RunDataFileLocator
from jv2backend.utils import json_response, split
import jv2backend.io.nexus as nxs


def add_routes(
    app: Flask, journal_server: JournalServer, run_locator: RunDataFileLocator
) -> Flask:
    """Add routes to the given Flask application."""

    @app.route("/setRoot/<prefix>")
    def setRoot(prefix):
        """Set the prefix to use by the RunDataFileLocator"""
        if prefix == "Default":
            prefix = config.get("run_locator_prefix")
        run_locator.prefix = f"{prefix}"

        return jsonify("success")

    @app.route("/getNexusFields/<instrument>/<cycles>/<runs>")
    def getNexusFields(instrument, cycles, runs):
        """Return a list of the log fields within the run

        :param instrument: Instrument name
        :param cycles: A semi-colon separated list of cycle names
        :param runs: A semi-colon separated list of run numbers. Length should match cycles
        :return: A list of full paths to log fields. Each entry is a list
        where the first entry is the group name followed by the full path to each available log entry
        """
        filepaths = _locate_run_files(
            journal_server, run_locator, instrument, cycles, runs
        )
        app.logger.debug(f"/getNexusFields: Located files: {filepaths}")
        logpaths = []
        for filepath, run in zip(filepaths, runs):
            if filepath is None:
                return jsonify(f"Error: Unable to find run file for run '{run}'")
            logpaths.extend(nxs.logpaths_from_path(filepath))

        return json_response(logpaths)

    @app.route("/getNexusData/<instrument>/<cycles>/<runs>/<fields>")
    def getNexusData(instrument, cycles, runs, fields):
        """Return a list of the log fields within the run.

        :param instrument: Instrument name
        :param cycles: A semi-colon separated list of cycle names
        :param runs: A semi-colon separated list of run numbers. Length should match cycles
        :param fields: A semi-colon separated list of field names. A colon indicates a path separator in field
        :return: A list of the log data
        """
        filepaths = _locate_run_files(
            journal_server, run_locator, instrument, cycles, runs
        )

        # The front end expects a list where the first entry is a list of all available fields.
        # The assumption is all instruments offer the same fields so only the first is examined.
        # The subsequent entries are one additional entry per run:
        #   [(start_time, end_time), [(run, field1),(time1, val1),...], [(run, field2),(time1, val1),...]]
        runs, fields = split(runs, ";"), split(fields, ";")
        all_field_data = []
        for filepath, run in zip(filepaths, runs):
            if filepath is None:
                return jsonify(f"Error: Unable to find run file for run '{run}'")

            nxsfile, first_group = nxs.open_at(filepath, 0)
            run_data = [nxs.timerange(first_group)]
            for name in fields:
                # request substitutes '/' in place of ';' so reverse this
                field_path = name.replace(":", "/")
                log_data = nxs.logvalues(first_group[field_path])  # type: ignore
                log_data.insert(0, (run, name))  # type: ignore
                run_data.append(log_data)  # type: ignore
            nxsfile.close()
            all_field_data.append(run_data)

        # Add the list of all available log paths required by the frontend
        all_field_data.insert(0, nxs.logpaths_from_path(filepaths[0]))  # type: ignore

        return json_response(all_field_data)

    @app.route("/getSpectrumRange/<instrument>/<cycle>/<runs>")
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
            journal_server, run_locator, instrument, cycle, run
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {run}")

        return str(nxs.spectra_count(filepaths[0]))  # type: ignore

    @app.route("/getMonitorRange/<instrument>/<cycle>/<runs>")
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
            journal_server, run_locator, instrument, cycle, run
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {runs}")

        return str(nxs.monitor_count(filepaths[0]))  # type: ignore

    @app.route("/getSpectrum/<instrument>/<cycle>/<runs>/<spectrum>")
    def getSpectrum(instrument, cycle, runs, spectrum):
        """Return a spectrum from a given set of runs

        :param instrument: The name of the instrument
        :param cycle: The cycle containing the runs
        :param runs: The list of runs whose data is returned
        :param spectrum: The number of the spectrum to access
        :return:
        """
        filepaths = _locate_run_files(
            journal_server, run_locator, instrument, cycle, runs
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {runs}")

        # first entry matches expectation of the frontend with sending back the parameters
        data = [[runs, spectrum, "detector"]]
        data.extend([nxs.spectrum(filepath, int(spectrum)) for filepath in filepaths])  # type: ignore
        return json_response(data)

    @app.route("/getMonSpectrum/<instrument>/<cycle>/<runs>/<monitor>")
    def getMonSpectrum(instrument, cycle, runs, monitor):
        """Return a spectrum from a monitor for a given set of runs

        :param instrument: The name of the instrument
        :param cycle: The cycle containing the runs
        :param runs: The list of runs whose data is returned
        :param spectrum: The number of the spectrum to access
        :return:
        """
        filepaths = _locate_run_files(
            journal_server, run_locator, instrument, cycle, runs
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {runs}")

        # first entry matches expectation of the frontend with sending back the parameters
        data = [[runs, monitor, "monitor"]]
        data.extend([nxs.spectrum(filepath, int(monitor)) for filepath in filepaths])  # type: ignore
        return json_response(data)

    @app.route("/getDetectorAnalysis/<instrument>/<cycle>/<run>")
    def getDetectorAnalysis(instrument, cycle, run):
        """Determine the number of spectra with non-zero signal values

        :param instrument: The instrument name
        :param cycle: The cycle containing the run
        :param run: The run to analyse
        :return: A string of the form "count(non_zero)/count(all_spectra)"
        """
        filepaths = _locate_run_files(
            journal_server, run_locator, instrument, cycle, run
        )
        if not any(filepaths):
            return jsonify(f"Error: Unable to find run {run}")

        return nxs.nonzero_spectra_ratio(filepaths[0])  # type: ignore

    # ------------------------ End Routes -------------------------

    return app


# Private functions


def _locate_run_files(
    journal_server: JournalServer,
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
        journal = journal_server.journal(instrument, cyclename=cycle)
        run = journal.run(run_number=run)
        if run is None:
            filepaths.append(None)
            continue
        logging.debug(f"Found metadata for run {run}")
        run["instrument_name"] = instrument
        filepaths.append(run_locator.locate(run))

    return filepaths
