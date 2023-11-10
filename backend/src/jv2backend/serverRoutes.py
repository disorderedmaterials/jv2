# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Defines the Flask endpoints that are server-related"""
from flask.wrappers import Response as FlaskResponse
from flask import Flask, jsonify, request
from threading import Thread, Event
import _thread
import logging
import time
from collections.abc import Callable

_WERKZEUG_SHUTDOWN_FUNCTION = Callable


# Threading
class KeepAliveThread(Thread):
    def __init__(self, wait_time: int):
        Thread.__init__(self, daemon=True)
        self._wait_time = wait_time
        self._counter = wait_time
        self._destroy = False

    def run(self):
        logging.debug("Starting keepalive timer...")
        global _WERKZEUG_SHUTDOWN_FUNCTION
        while self._counter > 0:
            # First check - if the reset event has fired we must set our
            # counter back to the wait_time
            if _KEEPALIVE_RESET_EVENT.is_set():
                self._counter = self._wait_time
                _KEEPALIVE_RESET_EVENT.clear()

            # Sleep for 1
            time.sleep(1)

            # Decrement counter
            self._counter = self._counter - 1
            logging.debug(self._counter)

        logging.debug("KEEPALIVE EXPIRED.")
        # _thread.interrupt_main()
        _WERKZEUG_SHUTDOWN_FUNCTION()


_KEEPALIVE_RESET_EVENT = Event()
_KEEPALIVE_THREAD = KeepAliveThread(15)


def add_routes(
    app: Flask,
) -> Flask:
    """Add routes to the given Flask application."""

    # ---------------- Queries ------------------
    @app.post("/ping")
    def ping() -> FlaskResponse:
        """Handle a ping from the frontend
        """
        global _WERKZEUG_SHUTDOWN_FUNCTION
        if "message" not in request.json:
            return jsonify('???', 400)

        if request.json["message"] == "HELLO":
            _WERKZEUG_SHUTDOWN_FUNCTION = request.environ.get('werkzeug.server.shutdown')
            return jsonify('HELLO', 200)
        elif request.json["message"] == "KEEPALIVE":
            if not  _KEEPALIVE_THREAD.is_alive():
                _KEEPALIVE_THREAD.start()
            return jsonify('OK', 200)

        return jsonify('???', 400)


    def keep_alive() -> None:
        logging.debug("HEsdflkhsdlkhfksjd")
        _KEEPALIVE_RESET_EVENT.set()

    return app
