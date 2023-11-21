# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import argparse
from . import app
import gunicorn.app.base

# Set up command-line arguments
parser = argparse.ArgumentParser(description="The JournalViewer 2 backend.")
parser.add_argument('-b', metavar='bind',
                    type=str, required=True,
                    help="Address to bind to (e.g. 127.0.0.1:5000)",
                    )


args = parser.parse_args()

# We will launch our Flask app with gunicorn on Linux/OSX but revert to
# waitress on Windows


class StandaloneApplication(gunicorn.app.base.BaseApplication):

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
    
gunicorn_options = {
    'bind': '%s:%s' % ('127.0.0.1', '5000'),
    "log-level": "DEBUG"
}
StandaloneApplication(app.create_app(True), gunicorn_options).run()
