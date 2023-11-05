# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import logging
from flask import Flask, jsonify, request
from flask.wrappers import Response as FlaskResponse

from jv2backend.requestData import RequestData, InvalidRequest
import jv2backend.journalLibrary
import jv2backend.io.journalGenerator


def add_routes(
    app: Flask,
    journalGenerator: jv2backend.io.journalGenerator.JournalGenerator,
    journalLibrary: jv2backend.journalLibrary.JournalLibrary
) -> Flask:
    """Add journal generation routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/generate/list")
    def list() -> FlaskResponse:
        """List available NeXuS files in a target directory

        The POST data should contain:
          journalRoot: The root network or disk location for journals [UNUSED]
        dataDirectory: The data file directory to list

        :return: The number of NeXuS files found
        """
        try:
            postData = RequestData(request.json,
                                   require_data_directory=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Scan for NeXuS files in data directory \
                      {postData.run_data_url}...")

        return journalGenerator.list_files(postData.run_data_url)

    @app.post("/generate/scan")
    def scan() -> FlaskResponse:
        """Generates journals and accompanying index file for a target dir

        The POST data should contain:
             journalRoot: Unique identifier for the journal set
               directory: The directory in journalRoot containing the journal
           dataDirectory: Location of the run data to scan
        dataOrganisation: How the data is to be organised
                filename: Name of the target index file to generate

        :return: A JSON-formatted list of new run data, or None
        """
        try:
            postData = RequestData(request.json,
                                   require_journal_file=True,
                                   require_data_directory=True,
                                   require_parameter="dataOrganisation")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Generate journal {postData.journal_filename} from \
                      {postData.run_data_url}")

        return journalGenerator.scan_files(postData, journalLibrary)

    return app
