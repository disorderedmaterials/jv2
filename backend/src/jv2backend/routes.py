# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines the Flask endpoints supported by the server"""
from typing import Any

from flask import Flask, jsonify
from flask.wrappers import Response as FlaskResponse

from jv2backend.io.journalserver import JournalServer


def add_routes(app: Flask, journal_server: JournalServer) -> Flask:
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

    @app.route("/getJournal/<instrument>/<cycle>")
    def getJournal(instrument: str, cycle: str) -> FlaskResponse:
        """Return a single journal of Runs for the instrument and cycle

        :param instrument: The name of an instrument
        :param cycle: The filename of a cycle journal file in the format journal_YY_N.xml
        :return: A JSON reponse
        """
        try:
            return _json_response(
                journal_server.journal(instrument_name=instrument, filename=cycle)
            )
        except Exception as exc:
            return jsonify(
                f"Unable to fetch journal for {instrument}, cycle {cycle}: {str(exc)}"
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

    @app.route("/updateJournal/<instrument>/<cycle>/<last_run>")
    def updateJournal(instrument, cycle, last_run):
        """Return runs after the last given run for the instrument and cycle

        :param instrument: The instrument name
        :param cycle: The cycle filename
        :param last_run: The last run that is currently known
        :return: A JSON-formatted list of Run data for runs newer than last_run
        """
        try:
            all_cycle_runs = journal_server.journal(instrument, cycle)
            return _json_response(all_cycle_runs.search("run_number", f">{last_run}"))
        except Exception as exc:
            return jsonify(
                f"Unable to fetch new runs for {instrument}, cycle {cycle}: {str(exc)}"
            )

    # -------------- No op routes for backwards compatability -----------
    @app.route("/setLocalSource/<inLocalSource>")
    def setLocalSource(_):
        return jsonify("")

    @app.route("/clearLocalSource")
    def clearLocalSource():
        return jsonify("")

    @app.route("/shutdown")
    def shutdown():
        return jsonify("")

    # ------------------------ End Routes -------------------------

    return app


# Private functions
def _json_response(result: Any) -> FlaskResponse:
    """Create a JSON-formatted response for the Flask server

    :result: An object with a .to_json method to be packed into a Response
    :return: A Flask-Response object
    """
    return FlaskResponse(result.to_json(), mimetype="application/json")
