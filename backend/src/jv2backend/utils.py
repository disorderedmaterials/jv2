# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 Team JournalViewer and contributors
from typing import Any, Sequence
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


def split(input: str, delimiter: str, discard_empty=True) -> Sequence[str]:
    """Split a string using a delimeter and optionally discard empty parts

    :param input: The input string
    :param delimiter: Delimeter used for splitting
    :param discard_empty: If True then empty elements are removed, defaults to True
    """
    items = input.split(delimiter)
    if discard_empty:
        items = list(filter(None, items))

    return items
