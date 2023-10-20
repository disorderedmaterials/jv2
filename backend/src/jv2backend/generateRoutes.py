# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import logging
from flask import Flask, jsonify, request
from flask.wrappers import Response as FlaskResponse

from jv2backend.requestData import RequestData, InvalidRequest
import jv2backend.journals
import jv2backend.io.journalLocator
import jv2backend.io.journalGenerator


def add_routes(
    app: Flask,
    journalGenerator: jv2backend.io.journalGenerator.JournalGenerator,
    journalLibrary: jv2backend.journals.JournalLibrary
) -> Flask:
    """Add journal generation routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/generate/list")
    def list() -> FlaskResponse:
        """List available NeXuS files in a target directory

        The POST data should contain:
              rootUrl: The root network or disk location for journals [UNUSED]
        dataDirectory: The data file directory to list

        :return: The number of NeXuS files found
        """
        try:
            postData = RequestData(request.json, journalLibrary,
                                   require_data_directory=True)
        except InvalidRequest as exc:
            return jsonify(f"Error: {str(exc)}")

        logging.debug(f"Scan for NeXuS files in data directory \
                      {postData.data_directory}...")

        return journalGenerator.list_files(postData.data_directory)

    @app.post("/generate/scan")
    def scan() -> FlaskResponse:
        """Generates journals and accompanying index file for a target dir

        The POST data should contain:
                 rootUrl: The root network or disk location for the journal
               directory: The directory in rootUrl containing the journal
           dataDirectory: Location of the run data to scan
        dataOrganisation: How the data is to be organised
                filename: Name of the target index file to generate

        :return: A JSON-formatted list of new run data, or None
        """
        try:
            postData = RequestData(request.json, journalLibrary,
                                   require_filename=True,
                                   require_data_directory=True)
        except InvalidRequest as exc:
            return jsonify(f"Error: {str(exc)}")

        logging.debug(f"Generate journal {postData.filename} from \
                      {postData.url}")

        return journalGenerator.scan_files(postData)

    return app