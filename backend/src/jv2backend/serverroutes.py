# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that are server-related"""
from flask import Flask, jsonify
from flask.wrappers import Response as FlaskResponse

def add_routes(
    app: Flask,
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.route("/ping")
    def ping() -> FlaskResponse:
        """Return that we are ready
        """
        return jsonify('READY')

    return app
