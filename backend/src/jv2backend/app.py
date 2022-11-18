# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Define the Flask instance and backend API routes"""
from typing import Optional
from flask import Flask

from jv2backend.config import CONFIG
from jv2backend.routes import add_routes

# Import the ISIS server. Use a factory in the future should
# alternate implementations be required
from jv2backend.io.isis.isisjournalserver import ISISJournalServer


def create_app(journal_server_url: Optional[str] = None) -> Flask:
    """Create the Flask application and define
    the routes served by the backend.

    :param journal_server_url: The address of the server providing the journal information.
                               Defaults to the value store in CONFIG
    """
    app = Flask(__name__)
    if journal_server_url is None:
        journal_server_url = CONFIG["journal_server_url"]
    journal_server = ISISJournalServer(journal_server_url)
    return add_routes(app, journal_server)


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
