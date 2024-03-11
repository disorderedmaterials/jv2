# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2024 Team JournalViewer and contributors

"""Define the Flask instance and backend API routes"""
import logging
import logging.config
from flask import Flask
import jv2backend.routes.journal
import jv2backend.routes.generate
import jv2backend.routes.acquisition
import jv2backend.routes.nexus
import jv2backend.routes.server
import jv2backend.main.library
import jv2backend.main.generator
import jv2backend.main.userCache
import jv2backend.classes.collection
import xml.etree.ElementTree as ElementTree
import argparse


def create_app(activate_cache: bool = True) -> Flask:
    """Create the Flask application and define the routes served by the
    backend.

    See config.py for configuration settings

    :param activate_cache: Whether to employ the user cache.
    """
    app = Flask(__name__)

    # Create our main objects
    journal_generator = jv2backend.main.generator.JournalGenerator()
    journal_library = jv2backend.main.library.JournalLibrary({})
    journal_acquirer = jv2backend.classes.collection.JournalAcquirer()

    # Register Flask routes
    jv2backend.routes.server.add_routes(app, journal_generator)
    jv2backend.routes.journal.add_routes(app, journal_library)
    jv2backend.routes.acquisition.add_routes(app, journal_acquirer, journal_library)
    jv2backend.routes.generate.add_routes(app, journal_generator, journal_library)
    jv2backend.routes.nexus.add_routes(app, journal_library)

    # Register XML namespaces
    ElementTree.register_namespace( '', "http://definition.nexusformat.org/schema/3.0")
    ElementTree.register_namespace('xsi', "http://www.w3.org/2001/XMLSchema-instance")

    # Initialise and activate the user data cache
    if activate_cache:
        jv2backend.main.userCache.initialise()

    return app


def go_gunicorn(jv2app: Flask, args):
    import gunicorn.app.base

    class GUnicornApplication(gunicorn.app.base.BaseApplication):

        def __init__(self, app, options=None):
            self.options = options or {}
            self.application = app
            super().__init__()

        def load_config(self):
            config = {key: value for key, value in self.options.items()
                      if key in self.cfg.settings and value is not None}
            for key, value in config.items():
                self.cfg.set(key.lower(), value)

        def load(self):
            return self.application

    # Set up the server options
    options = {
        'bind': args.bind,
    }
    if args.timeout:
        options["timeout"] = args.timeout
    if args.debug:
        options["log-level"] = "INFO"
    GUnicornApplication(jv2app, options).run()


global server
def go_waitress(jv2app: Flask, args):
    from waitress.server import create_server
    import signal
    global server

    # serve(jv2app, listen=args.bind, channel_timeout=args.timeout)

    # signal handler, to do something before shutdown service
    def handle_sig(sig, frame):
        logging.warning(f"Got signal {sig}, now close worker...")
        # worker.close()
        server.close()

    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGQUIT, signal.SIGHUP):
        signal.signal(sig, handle_sig)

    server = create_server(jv2app, listen=args.bind, channel_timeout=args.timeout)
    server.run()


def go():
    # Set up command-line arguments
    parser = argparse.ArgumentParser(description="The JournalViewer 2 backend.")
    parser.add_argument('-b', '--bind',
                        type=str, required=True,
                        help="Address to bind to (e.g. 127.0.0.1:5000)"
                        )
    parser.add_argument('-d', '--debug',
                        action='store_true',
                        help="Enable debug logging from the WSGI server."
                        )
    parser.add_argument('-t', '--timeout',
                        type=int, default=120,
                        help="Set request timeout (in second) for the server."
                        )
    parser.add_argument('-w', '--waitress',
                        action='store_true',
                        help="Whether to use waitress over gunicorn."
                        )

    args = parser.parse_args()

    # Create the Flask app
    app = create_app(True)

    logging.config.dictConfig(
        {
            "version": 1,
            "root": {
                "level": "DEBUG" if args.debug else "INFO",
            },
        }
    )

    # We will launch our Flask app with gunicorn on Linux/OSX unless waitress
    # is requested specifically
    if args.waitress:
        go_waitress(app, args)
    else:
        go_gunicorn(app, args)
