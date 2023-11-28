# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from typing import Any, Sequence
from functools import reduce
from datetime import datetime
from flask import jsonify
from flask.wrappers import Response as FlaskResponse


def json_response(result: Any) -> FlaskResponse:
    """Create a JSON-formatted response for the Flask server

    :result: An object with a .to_json method to be packed into a Response
    :return: A Flask-Response object
    """
    if hasattr(result, "to_json"):
        return FlaskResponse(result.to_json(), mimetype="application/json")
    else:
        return jsonify(result)


def _join_slash(a: str, b: str):
    """Join two strings together with a forward slash"""
    if a is None or len(a) == 0:
        return b
    elif b is None or len(b) == 0:
        return a
    else:
        return a.rstrip('/') + '/' + b.lstrip('/')


def url_join(*args):
    """Join together a number of strings to form a coherent URL path"""
    return reduce(_join_slash, args) if args else ''


def lm_to_datetime(timestamp: str) -> datetime:
    """Convert the supplied timestamp string, as provided in a Last-Modified
    header, to a datetime.

    :param timestamp: A timestamp as a string
    :return: A datetime.datetime object
    """
    LAST_MODIFIED_DT_FORMAT = "%a, %d %b %Y %H:%M:%S %Z"
    return datetime.strptime(timestamp, LAST_MODIFIED_DT_FORMAT)
