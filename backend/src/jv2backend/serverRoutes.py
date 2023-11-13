# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that are server-related"""
from flask import Flask, jsonify
from flask.wrappers import Response as FlaskResponse
import jv2backend.generator

def add_routes(
    app: Flask,
    journalGenerator: jv2backend.generator.JournalGenerator
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.route("/ping")
    def ping() -> FlaskResponse:
        """Return that we are ready
        """
        return jsonify('READY', 200)

    @app.route("/shutdown")
    def shutdown() -> FlaskResponse:
        """Prepare for nice shutdown
        """
        journalGenerator.stop_scan()

        return jsonify('OK', 200)


    return app
