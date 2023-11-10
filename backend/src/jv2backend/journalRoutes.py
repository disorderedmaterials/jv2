# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that only access journal information"""
import logging
from flask import Flask, jsonify, request, make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.utils import url_join
from jv2backend.requestData import RequestData, InvalidRequest
from jv2backend.journalLibrary import JournalLibrary


def add_routes(
    app: Flask,
    journalLibrary: JournalLibrary
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/journals/index")
    def get_journal_index() -> FlaskResponse:
        """Return the list of journal files in a specified location

        In addition to basic source information the POST data should contain
        full journal file location (the target of the 'list' operation) as well
        as an associated run data location for use if the target file will be
        parsed for the first time.

        :return: A JSON response containing basic information on available
                 journals
        """
        try:
            post_data = RequestData(request.json,
                                    require_journal_file=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Listing journals for {post_data.source_id}: "
                      f"{post_data.journal_file_url()}")

        # Parse the journal index
        return make_response(journalLibrary.get_index(
            post_data.source_type,
            post_data.library_key(),
            post_data.journal_root_url,
            post_data.journal_filename,
            post_data.run_data_root_url
        ), 200)

    @app.post("/journals/get")
    def get_journal_data() -> FlaskResponse:
        """Return the specified journal contents

        In addition to basic source information the POST data should contain
        full journal file location (the target of the 'get' operation).

        :return: A JSON response containing the journal data, or an error
        """
        try:
            post_data = RequestData(request.json,
                                    require_journal_file=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Get journal {post_data.journal_file_url()} "
                      f"from '{post_data.library_key()}'")

        journalLibrary.list()

        collection = journalLibrary[post_data.library_key()]
        if collection is None:
            return make_response(
                jsonify({"Error": f"No library '{post_data.library_key()}' "
                               f"currently exists."}), 200
            )

        return make_response(
            collection.get_journal_data(post_data.journal_filename),
            200
        )

    @app.post("/journals/getUpdates")
    def get_journal_updates():
        """Checks the specified journal file for updates, returning any new
        run data

        In addition to basic source information the POST data should contain
        full journal file location (the target of the 'get' operation).

        :return: A JSON-formatted list of new run data, or None
        """
        try:
            post_data = RequestData(request.json,
                                    require_journal_file=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Get journal {post_data.journal_file_url()} updates "
                      f"from source {post_data.library_key()}")

        collection = journalLibrary[post_data.library_key()]
        if collection is None:
            return make_response(jsonify(
                {"Error": f"No collection '{post_data.library_key()}' "
                          f"currently exists."}),
                200
            )

        return make_response(
            collection.get_updates(post_data.journal_filename),
            200
        )

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
            post_data = RequestData(request.json,
                                    require_value_map=True)
        except InvalidRequest as exc:
            return jsonify({"Error": str(exc)})

        logging.debug(f"Search {post_data.library_key()}...")

        collection = journalLibrary[post_data.library_key()]
        if collection is None:
            return make_response(jsonify(
                {"Error": f"No collection '{post_data.library_key()}' "
                          f"currently exists."}),
                200
            )

        return make_response(
            collection.search(post_data.value_map),
            200
        )

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
