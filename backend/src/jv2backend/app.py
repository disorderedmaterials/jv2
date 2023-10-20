# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Define the Flask instance and backend API routes"""
import logging
from flask import Flask

from jv2backend import config
import jv2backend.journalRoutes
import jv2backend.generateRoutes
import jv2backend.nexusRoutes
import jv2backend.serverRoutes
import jv2backend.journals
import jv2backend.io.journalLocator
import jv2backend.io.journalGenerator


def create_app(inside_gunicorn: bool = True) -> Flask:
    """Create the Flask application and define
    the routes served by the backend. See config.py for configuration settings

    :param inside_gunicorn: Whether the app has been run by gunicorn and
                            not directly
    """
    app = Flask(__name__)
    configure_logging(app, inside_gunicorn)

    journalLocator = jv2backend.io.journalLocator.JournalLocator()
    journalGenerator = jv2backend.io.journalGenerator.JournalGenerator()
    journalLibrary = jv2backend.journals.JournalLibrary({})

    jv2backend.serverRoutes.add_routes(app)
    jv2backend.journalRoutes.add_routes(app, journalLocator, journalLibrary)
    jv2backend.generateRoutes.add_routes(app, journalGenerator, journalLibrary)
    jv2backend.nexusRoutes.add_routes(app, journalLibrary)

    return app


def configure_logging(app: Flask, inside_gunicorn: bool) -> Flask:
    """_summary_

    :param app: Flask app to configure
    :param inside_gunicorn: Whether the app has been run by gunicorn and
                            not directly
    """
    if inside_gunicorn:
        # Match logging handlers and configuration with gunicorn
        gunicorn_logger = logging.getLogger('gunicorn.error')
        app.logger.handlers = gunicorn_logger.handlers
        app.logger.setLevel(gunicorn_logger.level)
        root = logging.getLogger()
        root.setLevel(gunicorn_logger.level)
    else:
        logging.config.dictConfig(
            {
                "version": 1,
                "root": {
                    "level": config.get("logger_level"),
                },
            }
        )

    return app


# In future add any command-line arguments here
def main():  # pragma: no cover
    """Start the backend"""
    app = create_app()

    # It is assumed that running this directly will only
    # be used during development so we activate debugging.
    # In production use a WSGI server such as gunicorn
    # and pass the create_app function to it
    app.run(debug=False)


# On running this module as main, start the server
if __name__ == "__main__":
    main()  # pragma: no cover
