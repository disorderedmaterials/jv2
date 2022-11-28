# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines the Flask endpoints supported by the server"""
import logging
from pathlib import Path
from typing import Any, Optional, Sequence

from flask import Flask, jsonify
from flask.wrappers import Response as FlaskResponse

from jv2backend.io.journalserver import JournalServer
import jv2backend.io.nexus as nxs
from jv2backend.io.rundatafilelocator import RunDataFileLocator


def add_routes(
    app: Flask, journal_server: JournalServer, run_locator: RunDataFileLocator
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.route("/getCycles/<instrument>")
    def getCycles(instrument: str) -> FlaskResponse:
        """Return the list of cycle files for the given instrument

        :param instrument: The name of an instrument
        :return: A JSON reponse
        """
        try:
            return jsonify(journal_server.journal_filenames(instrument_name=instrument))
        except Exception as exc:
            return jsonify(f"Unable to fetch cycles for {instrument}: {str(exc)}")

    @app.route("/getJournal/<instrument>/<filename>")
    def getJournal(instrument: str, filename: str) -> FlaskResponse:
        """Return a single journal of Runs for the instrument and cycle

        :param instrument: The name of an instrument
        :param filename: The filename of a cycle journal file in the format journal_YY_N.xml
        :return: A JSON reponse
        """
        try:
            return _json_response(
                journal_server.journal(instrument_name=instrument, filename=filename)
            )
        except Exception as exc:
            return jsonify(
                f"Unable to fetch journal for {instrument}, cycle {filename}: {str(exc)}"
            )

    @app.route("/getAllJournals/<instrument>/<field>/<search>/<options>")
    def getAllJournals(
        instrument: str, field: str, search: str, options: str
    ) -> FlaskResponse:
        """_summary_

        :param instrument: The instrument name
        :param field: The field to search
        :param search: The search text
        :param options: Options to control the search. Current recognizes caseSensitivity=true|false
        :return: The runs matching the search
        """
        case_sensitive = "caseSensitivity=true" in options
        if field in ("start_time", "start_date"):
            # keep compatible with frontend sending semi-colon separated date fields
            search = search.replace(";", "/")
            # start_date maps to start_time in the run_fields
            field = "start_time" if field.endswith("date") else field
        try:
            return _json_response(
                journal_server.search(instrument, field, search, case_sensitive)
            )
        except Exception as exc:
            return jsonify(f"Unable to complete search '{search}': {str(exc)}")

    @app.route("/pingCycle/<instrument>")
    def pingCycle(instrument):
        """Check if a new journal has been added for the instrument

        :param instrument: Instrument name
        :return: Json string containing the name of the new journal
        """
        result = journal_server.check_for_journal_filenames_update(instrument)
        return result if result is not None else ""

    @app.route("/updateJournal/<instrument>/<filename>/<last_run>")
    def updateJournal(instrument, filename, last_run):
        """Return runs after the last given run for the instrument and cycle

        :param instrument: The instrument name
        :param filename: The cycle filename
        :param last_run: The last run that is currently known
        :return: A JSON-formatted list of Run data for runs newer than last_run
        """
        try:
            all_cycle_runs = journal_server.journal(instrument, filename=filename)
            return _json_response(all_cycle_runs.search("run_number", f">{last_run}"))
        except Exception as exc:
            return jsonify(
                f"Unable to fetch new runs for {instrument}, cycle {filename}: {str(exc)}"
            )

    @app.route("/getTotalMuAmps/<instrument>/<cycle>/<runs>")
    def getTotalMuAmps(instrument, cycle, runs):
        """Return the total current values for each run

        :param instrument: The name of the instrument
        :param cycle: The cycle containing the runs
        :param runs: The list of runs whose data is returned
        :return: The total current in microamps, for each run as a ';' separate string
        """
        try:
            journal = journal_server.journal(instrument_name=instrument, filename=cycle)
        except Exception as exc:
            return jsonify(
                f"Unable to fetch journal for {instrument}, cycle {cycle}: {str(exc)}"
            )
        run_info = [journal.run(run) for run in _split(runs, ";")]
        if not all(run_info):
            return jsonify(f"Unable to find all run information: {runs}")

        return ";".join([info["proton_charge"] for info in run_info])  # type: ignore

    @app.route("/getGoToCycle/<instrument>/<run>")
    def getGoToCycle(instrument, run):
        """Find the cycle containing the run.

        :param instrument: The instrument name
        :param run: The run number
        :return: The journal filename containing the run or "Not Found" if no run is found
        """
        try:
            result = journal_server.filename_for_run(instrument, run)
        except Exception as exc:
            return jsonify(f"Error finding {run} for {instrument}: {exc}")

        if result is not None:
            return result
        else:
            return "Not Found"

    # -------------- NeXus access routes --------------------------------

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
                return jsonify(f"Unable to find run file for run '{run}'")
            logpaths.extend(nxs.logpaths_from_path(filepath))

        return _json_response(logpaths)

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
        runs, fields = _split(runs, ";"), _split(fields, ";")
        all_field_data = []
        for filepath, run in zip(filepaths, runs):
            if filepath is None:
                return jsonify(f"Unable to find run file for run '{run}'")

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

        return _json_response(all_field_data)

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
            return jsonify(f"Unable to find run {run}")

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
            return jsonify(f"Unable to find run {runs}")

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
            return jsonify(f"Unable to find run {runs}")

        # first entry matches expectation of the frontend with sending back the parameters
        data = [[runs, spectrum, "detector"]]
        data.extend([nxs.spectrum(filepath, int(spectrum)) for filepath in filepaths])  # type: ignore
        return _json_response(data)

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
            return jsonify(f"Unable to find run {runs}")

        # first entry matches expectation of the frontend with sending back the parameters
        data = [[runs, monitor, "monitor"]]
        data.extend([nxs.spectrum(filepath, int(monitor)) for filepath in filepaths])  # type: ignore
        return _json_response(data)

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
            return jsonify(f"Unable to find run {run}")

        return nxs.nonzero_spectra_ratio(filepaths[0])  # type: ignore

    # -------------- No op routes for backwards compatability -----------
    @app.route("/setLocalSource/<source>")
    def setLocalSource(source=""):
        return _json_response("")

    @app.route("/setRoot/<prefix>")
    def setRoot(prefix):
        return _json_response("")

    @app.route("/clearLocalSource")
    def clearLocalSource():
        return _json_response("")

    @app.route("/shutdown")
    def shutdown():
        return _json_response("")

    # ------------------------ End Routes -------------------------

    return app


# Private functions
def _json_response(result: Any) -> FlaskResponse:
    """Create a JSON-formatted response for the Flask server

    :result: An object with a .to_json method to be packed into a Response
    :return: A Flask-Response object
    """
    if hasattr(result, "to_json"):
        return FlaskResponse(result.to_json(), mimetype="application/json")
    else:
        return jsonify(result)


def _split(input: str, delimiter: str, discard_empty=True) -> Sequence[str]:
    """Split a string using a delimeter and optionally discard empty parts

    :param input: The input string
    :param delimiter: Delimeter used for splitting
    :param discard_empty: If True then empty elements are removed, defaults to True
    """
    items = input.split(delimiter)
    if discard_empty:
        items = list(filter(None, items))

    return items


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
    """
    logging.debug(f"Locating runs: '{cycles_str}'/'{runs_str}'")
    cycles, runs = _split(cycles_str, ";"), _split(runs_str, ";")
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
        filepaths.append(run_locator.locate(run))

    return filepaths
