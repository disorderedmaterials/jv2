# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access journal information"""
import logging
from flask import Flask, jsonify, request
from flask.wrappers import Response as FlaskResponse

from jv2backend.requestData import RequestData
from jv2backend.journals import JournalLibrary, JournalCollection
from jv2backend.io.journals.networkLocator import NetworkJournalLocator
from jv2backend.utils import json_response, url_join


def add_routes(
    app: Flask,
    networkJournalLocator: NetworkJournalLocator,
    journalLibrary: JournalLibrary
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

        :return: A JSON response containing available journals in a
                 list(BasicJournalFile), or an error
        """
        postData = RequestData(request.json, journalLibrary,
                               require_data_directory=True,
                               require_index_for_http=True)
        if not postData.is_valid:
            return jsonify(f"Error: {postData.error}")
        logging.debug(f"Listing journals at {postData.url}")

        # If we already have a library collection for the specified
        # rootUrl/directory, just return it
        if postData.journal_collection is not None:
            logging.debug(
                f"Returning existing journal collection for \
                    {postData.library_key}")
            return postData.journal_collection.to_basic()

        try:
            if postData.is_http:
                journalLibrary[postData.url] = JournalCollection(
                    networkJournalLocator.get_index(postData))
                return jsonify(journalLibrary[postData.url].to_basic())
        except Exception as exc:
            return jsonify(
                f"Error: Unable to list journals at {postData.url}:\
                  {str(exc)}")

    @app.post("/journals/get")
    def getJournalData() -> FlaskResponse:
        """Return the specified journal contents

        The POST data should contain:
                rootUrl: The root network or disk location for the journals
              directory: The directory in rootUrl to probe for journals
            journalFile: Name of the target journal file in the directory

        :return: A JSON reponse containing the journal data, or an error
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        journalFile = data["journalFile"]
        library_key = url_join(rootUrl, directory)

        logging.debug(
            f"Get journal {journalFile} from {directory} at {rootUrl}"
        )

        try:
            if (rootUrl.startswith("http")):
                return json_response(
                    networkJournalLocator.get_journal_data(
                        journalLibrary[library_key],
                        server_root=rootUrl, journal_directory=directory,
                        journal_file=journalFile))
        except Exception as exc:
            return jsonify(
                f"Error: Unable to get journal {journalFile} from {directory} \
                  at {rootUrl}: {str(exc)}"
            )

    @app.post("/journals/getUpdates")
    def getUpdates():
        """Checks the specified journal file for updates, returning any new
        run data

                The POST data should contain:
                rootUrl: The root network or disk location for the journals
              directory: The directory in rootUrl to probe for journals
            journalFile: Name of the target journal file in the directory

        :return: A JSON-formatted list of new run data, or None
        """
        data = request.json
        rootUrl = data["rootUrl"]
        directory = data["directory"]
        journalFile = data["journalFile"]
        library_key = url_join(rootUrl, directory)

        logging.debug(
            f"Get journal {journalFile} from {directory} at {rootUrl}"
        )

        try:
            if (rootUrl.startswith("http")):
                return json_response(
                    networkJournalLocator.get_updates(
                        journalLibrary[library_key],
                        server_root=rootUrl, journal_directory=directory,
                        journal_file=journalFile))
        except Exception as exc:
            return jsonify(
                f"Error: Unable to get updates to {journalFile} from \
                  {directory} at {rootUrl}: {str(exc)}"
            )

    # ---- TO BE CONVERTED TO REMOVE CYCLE / INSTRUMENT SPECIFICS

    @app.route("/journals/findRuns/<instrument>/<field>/<search>/<options>")
    def findRuns(
        instrument: str, field: str, search: str, options: str
    ) -> FlaskResponse:
        """Search over all available journals for any runs matching the
        specified search parameters

        :param instrument: The instrument name
        :param field: The field to search
        :param search: The search text
        :param options: Options to control the search. Current recognizes
                        caseSensitivity=true|false
        :return: The runs matching the search
        """
        case_sensitive = "caseSensitivity=true" in options
        if field in ("start_time", "start_date"):
            # keep compatible with frontend sending semi-colon separated date
            # fields
            search = search.replace(";", "/")
            # start_date maps to start_time in the run_fields
            field = "start_time" if field.endswith("date") else field
        try:
            return json_response(networkJournalLocator.search(
                instrument, field, search, case_sensitive))
        except Exception as exc:
            return jsonify(
                f"Error: Unable to complete search '{search}': {str(exc)}")

    @app.route("/journals/goToCycle/<instrument>/<run>")
    def getGoToCycle(instrument, run):
        """Find the cycle containing the run.

        :param instrument: The instrument name
        :param run: The run number
        :return: The journal filename containing the run or "Not Found" if no
                 run is found
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
