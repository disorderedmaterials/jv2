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

    # ========== routes ==========
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
            return FlaskResponse(journal.runs(), mimetype="application/json")
        except Exception as exc:
            return jsonify(
                f"Unable to fetch journal for {instrument}, cycle {cycle}: {str(exc)}"
            )

    @app.route("/getAllJournals/<instrument>/<search>")
    def getAllJournals(instrument: str, search: str) -> FlaskResponse:
        """Return a list of runs for each instrument

        :param instrument: The string name of the instrment
        :param search: A search string of the form "<run_field>/<user_input>/caseSensitivity=<true|false>
        :return: A list of all runs matching the criteria
        """
        journals = journal_server.journal_filenames(instrument)
        if journals is None:
            return jsonify(f"Unable to search journals for {instrument}")

        run_field, user_input, case_sensitive = search.split("/")
        journals.search(run_field, user_input, case_sensitive)
        # for journal in journals:
        #     runs = journal.search(run_field, user_input, case_sensitive)

    # ========== end routes ==========

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
