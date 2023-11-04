# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access journal information"""
import logging
from flask import Flask, jsonify, request, make_response
from flask.wrappers import Response as FlaskResponse

from jv2backend.requestData import RequestData, InvalidRequest
from jv2backend.journals import JournalLibrary, JournalData
import jv2backend.io.journalLocator
import jv2backend.io.journalGenerator
from jv2backend.utils import json_response


def add_routes(
    app: Flask,
    journalLocator: jv2backend.io.journalLocator.JournalLocator,
    journalLibrary: JournalLibrary
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/journals/list")
    def listJournals() -> FlaskResponse:
        """Return the list of journal files in a specified location

        In addition to basic source information the POST data should contain
        full journal file location (the target of the 'list' operation) as well
        as an associated run data location for use if the target file will be
        parsed for the first time.

        :return: A JSON response containing available journals in a
                 list(BasicJournalFile), or an error
        """
        try:
            postData = RequestData(request.json, journalLibrary,
                                   require_journal_file=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Listing journals for {postData.source_id}: "
                      f"{postData.journal_file_url()}")

        # If we already have a library collection for the specified
        # source, just return it
        if postData.journal_collection is not None:
            logging.debug(f"Returning existing journal collection for "
                          f"'{postData.library_key()}'")
            return postData.journal_collection.to_basic()

        # Parse the journal index
        return journalLocator.get_index(postData, journalLibrary)

    @app.post("/journals/get")
    def getJournalData() -> FlaskResponse:
        """Return the specified journal contents

        In addition to basic source information the POST data should contain
        full journal file location (the target of the 'get' operation).

        :return: A JSON response containing the journal data, or an error
        """
        try:
            postData = RequestData(request.json, journalLibrary,
                                   require_journal_file=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Get journal {postData.journal_file_url()} "
                      f"from '{postData.library_key()}'")

        journalLibrary.list()

        return journalLocator.get_journal_data(postData)

    @app.post("/journals/getUpdates")
    def getUpdates():
        """Checks the specified journal file for updates, returning any new
        run data

        In addition to basic source information the POST data should contain
        full journal file location (the target of the 'get' operation).

        :return: A JSON-formatted list of new run data, or None
        """
        try:
            postData = RequestData(request.json, journalLibrary,
                                   require_journal_file=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Get journal {postData.journal_file_url()} from source "
                      f"{postData.source_id}")

        return journalLocator.get_updates(postData)

    @app.post("/journals/search")
    def search() -> FlaskResponse:
        """Search over all available journals in a target source for any runs
        matching the specified search parameters

        In addition to basic source information the POST data should contain
        one or more parameters on which to search the data. The search will
        generate a new journal containing matching run data - this is stored
        in the library for future retrieval.

        :return: A JSON-formatted list of run data, or None
        """
        try:
            postData = RequestData(request.json, journalLibrary,
                                   require_in_library=True,
                                   require_value_map=True)
        except InvalidRequest as exc:
            return jsonify({"Error": str(exc)})

        # First, make sure all journals in the collection are ready for searching
        journalLocator.get_all_journal_data(postData.journal_collection)

        try:
            runs = postData.journal_collection.search(postData.value_map)
        except Exception as exc:
            return make_response(jsonify(
                {"Error": f"Unable to complete search: {str(exc)}"}), 200
            )

        # Return the data
        return make_response(JournalData(runs).to_json(), 200)

    # ---- TO BE CONVERTED TO REMOVE CYCLE / INSTRUMENT SPECIFICS


    @app.route("/journals/goToCycle/<instrument>/<run>")
    def getGoToCycle(instrument, run):
        """Find the cycle containing the run.

        :param instrument: The instrument name
        :param run: The run number
        :return: The journal filename containing the run or "Not Found" if no
                 run is found
        """
        try:
            result = journalLocator.filename_for_run(instrument, run)
        except Exception as exc:
            return make_response(
                jsonify(f"Error finding {run} for {instrument}: {exc}"), 200
            )

        if result is not None:
            return result
        else:
            return "Not Found"

    # ------------------------ End Routes -------------------------

    return app
