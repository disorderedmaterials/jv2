# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Define the Flask instance and backend API routes"""
import logging
import logging.config
from flask import Flask
import jv2backend.routes.journal
import jv2backend.routes.generate
import jv2backend.routes.nexus
import jv2backend.routes.server
import jv2backend.main.library
import jv2backend.main.generator
import jv2backend.main.userCache
import xml.etree.ElementTree as ElementTree


def create_app(activate_cache: bool = True) -> Flask:
    """Create the Flask application and define the routes served by the
    backend.

    See config.py for configuration settings

    :param activate_cache: Whether to employ the user cache.
    """
    app = Flask(__name__)
    configure_logging(app)

    # Create our main objects
    journal_generator = jv2backend.main.generator.JournalGenerator()
    journal_library = jv2backend.main.library.JournalLibrary({})

    # Register Flask routes
    jv2backend.routes.server.add_routes(app, journal_generator)
    jv2backend.routes.journal.add_routes(app, journal_library)
    jv2backend.routes.generate.add_routes(app, journal_generator, journal_library)
    jv2backend.routes.nexus.add_routes(app, journal_library)

    # Register XML namespaces
    ElementTree.register_namespace( '', "http://definition.nexusformat.org/schema/3.0")
    ElementTree.register_namespace('xsi', "http://www.w3.org/2001/XMLSchema-instance")

    # Initialise and activate the user data cache
    if activate_cache:
        jv2backend.main.userCache.initialise()

    return app


def configure_logging(app: Flask) -> Flask:
    """_summary_

    :param app: Flask app to configure
    """
    # Match logging handlers and configuration with gunicorn
    wsgi_logger = logging.getLogger('gunicorn.error')
    if not wsgi_logger:
        wsgi_logger = logging.getLogger('waitress')
    app.logger.handlers = wsgi_logger.handlers
    app.logger.setLevel(wsgi_logger.level)
    root = logging.getLogger()
    root.setLevel(wsgi_logger.level)

    return app