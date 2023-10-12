# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access journal information"""
import logging
from flask import Flask, jsonify, request
from flask.wrappers import Response as FlaskResponse

from jv2backend.io.journals.networkLocator import NetworkJournalLocator
from jv2backend.utils import json_response, split


def add_routes(
    app: Flask,
    networkJournalLocator: NetworkJournalLocator,
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/journals/list")
    def listJournals() -> FlaskResponse:
        """Return the list of journal files in a specified location

        The POST data should contain:
               rootUrl: The root network or disk location for the journals
             directory: The directory in rootUrl to probe for journals
            index_file: Name of the index file in the directory, if known

        :return: A JSON response containing an array of available journals in the form of a JournalFileList, or an error
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        logging.debug("Listing journals for: " + rootUrl + ", " + directory)
        try:
            if (rootUrl.startswith("http")):
                if "indexFile" in data:
                    return jsonify(networkJournalLocator.get_index(server_root=rootUrl, journal_directory=directory, index_file=data['indexFile']))
                else:
                    return jsonify(f"Error: Index file name must be provided for a network source ({rootUrl}/{directory}).")
        except Exception as exc:
            return jsonify(f"Error: Unable to list journals for {directory} from {rootUrl}: {str(exc)}")

    @app.post("/journals/get")
    def getJournal() -> FlaskResponse:
        """Return a journal of Runs for the instrument and cycle

        The POST data should contain:
                rootUrl: The root network or disk location for the journals
              directory: The directory in rootUrl to probe for journals
            journalFile: Name of the target journal file in the directory

        :return: A JSON reponse containing the content of the journal, or an error
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        journalFile = data["journalFile"]
        logging.debug(
            f"Get journal {journalFile} from {directory} at {rootUrl}"
            )
        try:
            return json_response(
                networkJournalLocator.get_journal(server_root=rootUrl,
                                                  journal_directory=directory,
                                                  journal_file=journalFile)
            )
        except Exception as exc:
            return jsonify(
                f"Error: Unable to get journal {journalFile} from {directory} at {rootUrl}: {str(exc)}"
            )



    # ---- TO BE CONVERTED TO REMOVE CYCLE / INSTRUMENT SPECIFICS

    @app.route("/journals/findRuns/<instrument>/<field>/<search>/<options>")
    def findRuns(
        instrument: str, field: str, search: str, options: str
    ) -> FlaskResponse:
        """Search over all available journals for any runs matching the specified search parameters

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
                networkJournalLocator.search(instrument, field, search, case_sensitive)
            )
        except Exception as exc:
            return jsonify(f"Error: Unable to complete search '{search}': {str(exc)}")

    @app.route("/journals/ping/<instrument>")
    def pingJournals(instrument):
        """Check if a new journal has been added for the instrument

        :param instrument: Instrument name
        :return: Json string containing the name of the new journal
        """
        result = networkJournalLocator.check_for_journal_filenames_update(instrument)
        return result if result is not None else ""

    @app.route("/journals/update/<instrument>/<filename>/<last_run>")
    def updateJournal(instrument, filename, last_run):
        """Return runs after the last given run for the instrument and cycle

        :param instrument: The instrument name
        :param filename: The cycle filename
        :param last_run: The last run that is currently known
        :return: A JSON-formatted list of Run data for runs newer than last_run
        """
        try:
            all_cycle_runs = networkJournalLocator.journal(instrument, filename=filename)
            return json_response(all_cycle_runs.search("run_number", f">{last_run}"))
        except Exception as exc:
            return jsonify(
                f"Error: Unable to fetch new runs for {instrument}, cycle {filename}: {str(exc)}"
            )

    @app.route("/journals/getTotalMuAmps/<instrument>/<cycle>/<runs>")
    def getTotalMuAmps(instrument, cycle, runs):
        """Return the total current values for each run

        :param instrument: The name of the instrument
        :param cycle: The cycle containing the runs
        :param runs: The list of runs whose data is returned
        :return: The total current in microamps, for each run as a ';' separate string
        """
        try:
            journal = networkJournalLocator.journal(instrument_name=instrument, filename=cycle)
        except Exception as exc:
            return jsonify(
                f"Error: Unable to fetch journal for {instrument}, cycle {cycle}: {str(exc)}"
            )
        run_info = [journal.run(run) for run in split(runs, ";")]
        if not all(run_info):
            return jsonify(f"Error: Unable to find all run information: {runs}")

        return ";".join([info["proton_charge"] for info in run_info])  # type: ignore

    @app.route("/journals/goToCycle/<instrument>/<run>")
    def getGoToCycle(instrument, run):
        """Find the cycle containing the run.

        :param instrument: The instrument name
        :param run: The run number
        :return: The journal filename containing the run or "Not Found" if no run is found
        """
        try:
            result = networkJournalLocator.filename_for_run(instrument, run)
        except Exception as exc:
            return jsonify(f"Error finding {run} for {instrument}: {exc}")

        if result is not None:
            return result
        else:
            return "Not Found"

    # ------------------------ End Routes -------------------------

    return app
