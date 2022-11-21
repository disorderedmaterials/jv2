# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines the Flask endpoints supported by the server"""
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
        filepaths = _locate_run_files(instrument, cycles, runs)
        logpaths = []
        for filepath, run in zip(filepaths, runs):
            if filepath is None:
                return jsonify(f"Unable to find run file for run '{run}'")
            logpaths.extend(nxs.logpaths(filepath))

        return _json_response(logpaths)

    # -------------- No op routes for backwards compatability -----------
    @app.route("/setLocalSource/<inLocalSource>")
    def setLocalSource(_):
        return _json_response("")

    @app.route("/clearLocalSource")
    def clearLocalSource():
        return _json_response("")

    @app.route("/shutdown")
    def shutdown():
        return _json_response("")

    # ------------------------ End Routes -------------------------

    def _locate_run_files(
        instrument: str, cycles_str: str, runs_str: str
    ) -> Sequence[Optional[Path]]:
        """Return a list of Path objects to files for the given query

        :param file_locator: An object that understands how to locate a Run
        :param instrument: Instrument name
        :param cycles: A semi-colon separated list of cycle names in the format YY_N
        :param runs: A semi-colon separated list of run numbers. Length should match cycles.
        :return: A list of Path|None to the found data files. None indicates the files could not be found
        """
        filepaths = []
        cycles, runs = _split(cycles_str, ";"), _split(runs_str, ";")
        for cycle, run in zip(cycles, runs):
            journal = journal_server.journal(instrument, cyclename=cycle)
            run = journal.run(run_number=run)
            if run is None:
                filepaths.append(None)
                continue
            filepaths.append(run_locator.locate(run))

        return filepaths

    # ------------------------ End Helpers -------------------------

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
