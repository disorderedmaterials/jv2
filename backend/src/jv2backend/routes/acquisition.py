# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import logging
from flask import Flask, jsonify, request, make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.classes.requestData import RequestData, InvalidRequest
from jv2backend.classes.collection import JournalAcquirer
from jv2backend.main.library import JournalLibrary


def add_routes(
    app: Flask,
    journalAcquirer: JournalAcquirer,
    journalLibrary: JournalLibrary,
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/acquire")
    def acquire() -> FlaskResponse:
        """Acquire all data within the specified source. The process will be threaded
        and run in the background.

        :return: A JSON response containing OK or an error message
        """
        try:
            post_data = RequestData(request.json,
                                    require_journal_file=True)
        except InvalidRequest as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        logging.debug(f"Get all journals for '{post_data.library_key()}'")

        collection = journalLibrary[post_data.library_key()]
        if collection is None:
            return make_response(
                jsonify({"Error": f"No library '{post_data.library_key()}' "
                                  f"currently exists."}), 200
            )

        return make_response(
            journalAcquirer.acquire_all_data(collection),
            200
        )

    @app.get("/acquire/update")
    def acquire_update() -> FlaskResponse:
        """Provide an update on the current background acquisition"""
        return make_response(journalAcquirer.get_acquisition_update(), 200)

    @app.get("/acquire/stop")
    def acquire_stop() -> FlaskResponse:
        """Stop the current background acquisition"""
        return make_response(journalAcquirer.stop_acquisition(), 200)

    # ------------------------ End Routes -------------------------

    return app
