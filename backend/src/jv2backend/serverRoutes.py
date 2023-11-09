# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that are server-related"""
from flask.wrappers import Response as FlaskResponse
from flask import Flask, jsonify, request

def add_routes(
    app: Flask,
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/ping")
    def ping() -> FlaskResponse:
        """Handle a ping from the frontend
        """
        if "STATUS" in request.json:
            return jsonify('READY', 200)
        elif "KEEPALIVE" in request.json:
            

    return app
