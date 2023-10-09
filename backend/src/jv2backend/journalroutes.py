# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access journal information"""
from flask import Flask, jsonify
from flask.wrappers import Response as FlaskResponse

from jv2backend.io.journalserver import JournalServer
from jv2backend.utils import json_response, split


def add_routes(
    app: Flask,
    journal_server: JournalServer,
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.route("/ping")
    def ping() -> FlaskResponse:
        """Return that we are ready
        """
        return jsonify('READY')

    @app.route("/listJournals/<instrument>")
    def listJournals(instrument: str) -> FlaskResponse:
        """Return the list of journal files in the given instrument directory

        :param instrument: The name of an instrument
        :return: A JSON reponse
        """
        try:
            return jsonify(journal_server.journal_filenames(instrument_name=instrument))
        except Exception as exc:
            return jsonify(f"Error: Unable to fetch cycles for {instrument}: {str(exc)}")

    @app.route("/getJournal/<instrument>/<filename>")
    def getJournal(instrument: str, filename: str) -> FlaskResponse:
        """Return a single journal of Runs for the instrument and cycle

        :param instrument: The name of an instrument
        :param filename: The filename of a cycle journal file in the format journal_YY_N.xml
        :return: A JSON reponse
        """
        try:
            return json_response(
                journal_server.journal(instrument_name=instrument, filename=filename)
            )
        except Exception as exc:
            return jsonify(
                f"Error: Unable to fetch journal for {instrument}, cycle {filename}: {str(exc)}"
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
            return json_response(
                journal_server.search(instrument, field, search, case_sensitive)
            )
        except Exception as exc:
            return jsonify(f"Error: Unable to complete search '{search}': {str(exc)}")

    @app.route("/pingJournals/<instrument>")
    def pingJournals(instrument):
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
            return json_response(all_cycle_runs.search("run_number", f">{last_run}"))
        except Exception as exc:
            return jsonify(
                f"Error: Unable to fetch new runs for {instrument}, cycle {filename}: {str(exc)}"
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
                f"Error: Unable to fetch journal for {instrument}, cycle {cycle}: {str(exc)}"
            )
        run_info = [journal.run(run) for run in split(runs, ";")]
        if not all(run_info):
            return jsonify(f"Error: Unable to find all run information: {runs}")

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

    # ------------------------ End Routes -------------------------

    return app
