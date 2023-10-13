# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Access configuration values for the program"""
import os

# This must match that defined in frontend
ENVIRON_NAME_PREFIX = "JV2"

DEFAULTS = {
    "logger_level": "INFO",
    "run_locator_class": "jv2backend.io.isis.fileLocator.LegacyArchiveFileLocator",
    "run_locator_prefix": "/archive",
}


def get(name: str) -> str:
    """Access a configuration value via its name.
    Values can be overridden by defining environment
    variables named JV2_{name.upper()}. If not
    enviroment variable exists a default value is
    returned.

    :return: The configuration value
    :raises: KeyError if not value can be found
    """
    environ_var_name = f"{ENVIRON_NAME_PREFIX}_{name.upper()}"
    return os.environ.get(environ_var_name, DEFAULTS[name])
