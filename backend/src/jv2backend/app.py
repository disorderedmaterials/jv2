# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Define the Flask instance and backend API routes"""
from typing import Any
from flask import Flask, jsonify
from flask.wrappers import Response as FlaskResponse

from jv2backend.config import CONFIG

# Import the ISIS server. Use a factory in the future should
# alternate implementations be required
from jv2backend.io.isis.isisjournalserver import ISISJournalServer


def create_app(journal_server_url: str) -> Flask:
    """Create the Flask application and define
    the routes served by the backend
    """
    app = Flask(__name__)
    journal_server = ISISJournalServer(journal_server_url)
    # -------------------------- Routes -------------------------

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
            journal = journal_server.journal(instrument_name=instrument, filename=cycle)
            return FlaskResponse(journal.to_json(), mimetype="application/json")
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
            results = journal_server.search(
                instrument, field, search, case_sensitive
            ).to_json()
            return FlaskResponse(results, mimetype="application/json")
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
def _json_response(*, result: Any, error_msg: str):
    """Create a response for the Flask server. If the result is None
    the error_msg is created as a single string entry in a json dict
    of the form

    {'response': error_msg}

    :param result: The result from a call or None if an error occurred
    :param error_msg: A string indicating an error
    :return: A Response object
    """
    if result is None:
        result = {"response": error_msg}
    return jsonify(result)


# In future add any command-line arguments here
def main():
    """Start the backend"""
    app = create_app(CONFIG["journal_server_url"])

    # It is assumed that running this directly will only
    # be used during development so we activate debugging.
    # In production use a WSGI server such as gunicorn
    # and pass the create_app function to it
    app.run(debug=True)


# On running this module as main, start the server
if __name__ == "__main__":
    main()
