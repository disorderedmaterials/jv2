# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Define the Flask instance and backend API routes"""
import importlib
from logging.config import dictConfig as loggerConfig
from flask import Flask

from jv2backend import config
from jv2backend import journalroutes
from jv2backend import nexusroutes

# Import the ISIS server. Use a factory in the future should
# alternate implementations be required
from jv2backend.io.isis.isisjournalserver import ISISJournalServer
from jv2backend.io.isis.filelocator import (
    LegacyArchiveFileLocator,
    RunDataFileLocator,
)


def create_app() -> Flask:
    """Create the Flask application and define
    the routes served by the backend.

    See config.py for configuration settings
    """
    loggerConfig(
        {
            "version": 1,
            "root": {
                "level": config.get("logger_level"),
            },
        }
    )

    journal_server = ISISJournalServer(config.get("journal_server_url"))
    run_locator = _create_runfilelocator()

    app = Flask(__name__)
    journalroutes.add_routes(app, journal_server)
    nexusroutes.add_routes(app, journal_server, run_locator)
    return app

def _create_runfilelocator() -> RunDataFileLocator:
    """Create the run locator from the configuration values

    :raises: RuntimeError if the class cannot be found.
    """
    try:
        full_cls_name = config.get("run_locator_class")
        last_period_index = full_cls_name.rfind(".")
        module_name, class_name = full_cls_name[:last_period_index], full_cls_name[last_period_index + 1:]
        module = importlib.import_module(module_name)
        cls = getattr(module, class_name)
        prefix = config.get("run_locator_prefix")
    except Exception as exc:
        raise RuntimeError(str(exc)) from exc

    return cls(prefix)

# In future add any command-line arguments here
def main():  # pragma: no cover
    """Start the backend"""
    app = create_app()

    # It is assumed that running this directly will only
    # be used during development so we activate debugging.
    # In production use a WSGI server such as gunicorn
    # and pass the create_app function to it
    app.run(debug=True)


# On running this module as main, start the server
if __name__ == "__main__":
    main()  # pragma: no cover
