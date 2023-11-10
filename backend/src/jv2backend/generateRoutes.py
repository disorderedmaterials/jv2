# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import logging
from flask import Flask, jsonify, request, make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.utils import url_join
from jv2backend.journalCollection import JournalCollection
from jv2backend.journal import SourceType
from jv2backend.requestData import RequestData, InvalidRequest
import jv2backend.journalLibrary
import jv2backend.generator
import datetime


def add_routes(
    app: Flask,
    journalGenerator: jv2backend.generator.JournalGenerator,
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
            post_data = RequestData(request.json,
                                    require_data_directory=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Scan for NeXuS files in data directory "
                      f"{post_data.run_data_root_url}...")

        return make_response(
            journalGenerator.list_files(post_data.run_data_root_url),
            200
        )

    @app.post("/generate/go")
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
            post_data = RequestData(request.json,
                                    require_journal_file=True,
                                    require_data_directory=True,
                                    require_parameter="sortKey")
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Generate journals for '{post_data.library_key()}' "
                      f"from NeXuS files in {post_data.run_data_root_url}")

        journalLibrary[post_data.library_key()] = JournalCollection(
            SourceType.Generated,
            post_data.library_key(),
            post_data.journal_root_url,
            "index.xml",
            post_data.run_data_root_url,
            datetime.datetime.now()
        )

        return journalGenerator.generate(journalLibrary[post_data.library_key()], post_data.parameter)

    return app
