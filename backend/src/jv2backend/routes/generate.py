# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2024 Team JournalViewer and contributors

import logging
from flask import Flask, jsonify, request, make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.utils import url_join
from jv2backend.classes.collection import JournalCollection
from jv2backend.classes.journal import SourceType
from jv2backend.classes.requestData import RequestData, InvalidRequest
import jv2backend.main.library
import jv2backend.main.generator
import datetime


def add_routes(
    app: Flask,
    journalGenerator: jv2backend.main.generator.JournalGenerator,
    journalLibrary: jv2backend.main.library.JournalLibrary
) -> Flask:
    """Add journal generation routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/generate/list")
    def list() -> FlaskResponse:
        """List available NeXuS files in a target directory

        The POST data should contain:
          runDataRootUrl: The data file directory to list

        :return: A JSON response containing details of the NeXuS files found
        """
        try:
            post_data = RequestData(request.json,
                                    require_data_directory=True,
                                    require_parameters="rootRegExpSelector")
        except InvalidRequest as exc:
            return make_response(jsonify({"InvalidRequestError": str(exc)}), 200)

        logging.debug(f"List NeXuS files in data directory "
                      f"{post_data.run_data_root_url}...")

        return make_response(
            journalGenerator.list_files(post_data.run_data_root_url,
                                              post_data.parameter("rootRegExpSelector")),
            200
        )

    @app.post("/generate/scan")
    def scan() -> FlaskResponse:
        """Generates journals and accompanying index file for a target dir

        The POST data should contain:
          runDataRootUrl: The data file directory to scan

        :return: A JSON-formatted list of new run data, or None
        """
        try:
            post_data = RequestData(request.json,
                                    require_data_directory=True,
                                    require_parameters="sortKey,scanType")
        except InvalidRequest as exc:
            return make_response(jsonify({"InvalidRequestError": str(exc)}), 200)

        logging.debug(f"Scan NeXuS files discovered in "
                      f"{post_data.run_data_root_url}")

        if post_data.parameter("scanType") == "full":
            logging.debug("... Performing full scan")
            return make_response(journalGenerator.scan(), 200)
        elif post_data.parameter("scanType") == "updateAll":
            logging.debug("... Updating all files in existing collection")
            return make_response(
                journalGenerator.scan(journalLibrary[post_data.library_key()]),
                200
            )

    @app.get("/generate/scanUpdate")
    def scan_update() -> FlaskResponse:
        """Provide an update on the current background scan"""
        return make_response(journalGenerator.get_scan_update(), 200)

    @app.get("/generate/stopScan")
    def scan_stop() -> FlaskResponse:
        """Stop the current background scan"""
        return make_response(journalGenerator.stop_scan(), 200)

    @app.post("/generate/finalise")
    def finalise() -> FlaskResponse:
        """Generates journals and accompanying index file for a target dir

        The POST data should contain:
              journalRoot: Unique identifier for the journal set
           runDataRootUrl: Location of the run data to scan
         dataOrganisation: How the data is to be organised
                  sortKey: An additional parameter describing the journal
                           organisation strategy to employ

        :return: A JSON response indicating the success of the operation
        """
        try:
            post_data = RequestData(request.json,
                                    require_journal_file=True,
                                    require_data_directory=True,
                                    require_parameters="sortKey,scanType")
        except InvalidRequest as exc:
            return make_response(jsonify({"InvalidRequestError": str(exc)}), 200)

        logging.debug(f"Finalise journal generation for '{post_data.library_key()}' "
                      f"from NeXuS files in {post_data.run_data_root_url}")

        # Create a new collection, overwriting any potential old one
        journalLibrary[post_data.library_key()] = JournalCollection(
            SourceType.Generated,
            post_data.library_key(),
            post_data.journal_root_url,
            "index.xml",
            post_data.run_data_root_url,
            datetime.datetime.now()
        )

        # Generate the full journal data
        return make_response(
            journalGenerator.generate(journalLibrary[post_data.library_key()],
                                      post_data.parameter("sortKey")),
            200
        )
    return app
