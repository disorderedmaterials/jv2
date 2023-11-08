# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import datetime
from jv2backend.integerRange import IntegerRange

# Format of start time search string
USERINPUT_DT_FORMAT_STR = "%Y/%m/%d"
UNIX_DT_FORMAT_STR = "%Y-%m-%dT%H:%M:%S"

def _to_datetime(user_input: str, input_format: str) -> datetime.datetime:
    """Convert from a string of specified format to a datetime object"""
    return datetime.datetime.strptime(user_input, input_format)

# Query handlers
# A handler should have the form Callable[[{}}, str, bool], {}]
def _query_string_contains(
    data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field contains the text
    string provided.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A dict with matching runs
    """
    results = {}
    search_value = value if case_sensitive else value.lower()
    for run in data:
        if field not in data[run]:
            continue
        text = data[run][field] if case_sensitive else data[run][field].lower()
        if search_value in text:
            results[run] = data[run]
    return results

def _query_string_equals(
        data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field exactly matches the
    string provided.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A dict with matching runs
    """
    results = {}
    search_value = value if case_sensitive else value.lower()
    for run in data:
        if field not in data[run]:
            continue
        text = data[run][field] if case_sensitive else data[run][field].lower()
        if search_value == text:
            results[run] = data[run]
    return results

def _query_integer_in_range(
        data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field, when converted to an
    int, falls within the range specified in the value parameter. It is
    assumed that the field can be reliably converted to an int, and the search
    is inclusive.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Integer range, which can be of the following construction:
                   N-M - A range of N to M inclusive
                   <N  - Any number less than N
                   >N  - Any number greater than N
    :param case_sensitive: <Unused, required by API>
    :return: A dict with matching runs
    """
    results = {}

    if "-" in value:
        # Range search
        try:
            irange = IntegerRange.from_string(value)
        except ValueError:
            return {}

        for run in data:
            if field in data[run] and int(data[run][field]) in irange:
                results[run] = data[run]

    elif value.startswith("<"):
        # Less than
        ivalue = int(value.lstrip("<>"))
        for run in data:
            if field in data[run] and int(data[run][field]) < ivalue:
                results[run] = data[run]

    elif value.startswith(">"):
        # Greater than
        ivalue = int(value.lstrip("<>"))
        for run in data:
            if field in data[run] and int(data[run][field]) > ivalue:
                results[run] = data[run]

    else:
        # Equal to
        ivalue = int(value)
        for run in data:
            if field in data[run] and int(data[run][field]) == ivalue:
                results[run] = data[run]

    return results


def _query_datetime_in_range(
        data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field, when converted to an
    datetime, falls within the range specified in the value parameter. It is
    assumed that the field can be reliably converted to a datetime, and the
    search is inclusive.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Datetime range, which can be of the following construction:
                   N-M - A range of N to M inclusive
                   <N  - Any datetime before N
                   >N  - Any datetime after N
    :param case_sensitive: <Unused, required by API>
    :return: A dict with matching runs
    """
    results = {}

    if "-" in value:
        # Range search
        try:
            start, end = value.split("-")
            start = _to_datetime(start.strip(), USERINPUT_DT_FORMAT_STR)
            end = _to_datetime(end.strip(), USERINPUT_DT_FORMAT_STR)
        except ValueError:
            return {}

        for run in data:
            if (field in data[run] and
                start <= _to_datetime(data[run][field], UNIX_DT_FORMAT_STR)
                <= end):
                results[run] = data[run]

    elif value.startswith("<"):
        # Less than
        mark = _to_datetime(value.lstrip("<>"), USERINPUT_DT_FORMAT_STR)
        for run in data:
            if (field in data[run] and
                _to_datetime(data[run][field], UNIX_DT_FORMAT_STR) < mark):
                results[run] = data[run]

    elif value.startswith(">"):
        # Greater than
        mark = _to_datetime(value.lstrip("<>"), USERINPUT_DT_FORMAT_STR)
        for run in data:
            if (field in data[run] and
                    _to_datetime(data[run][field], UNIX_DT_FORMAT_STR) > mark):
                results[run] = data[run]

    else:
        raise RuntimeError("Can't perform a literal time comparison.")

    return results

# Map a field name to a handler for that query if it should have special
# handling
_SPECIAL_QUERY_HANDLERS = {
    "experiment_identifier": _query_string_equals,
    "run_number": _query_integer_in_range,
    "start_time": _query_datetime_in_range,
}


def select(
    data: {}, field: str, value: str, case_sensitive: bool = False
) -> {}:
    """Search dictionary data for entries whose field matches the supplied
    value. Since fields can be of varying type we define handlers for specific
    cases, defaulting to a simple "is in string" check in the general case.

    The input `data` is assumed to be a dict of dicts with specific type
    Dict[int, Dict[str,str]].

    :param data: The dictionary of data to search
    :param field: Field (key) within each data item to test
    :param value: Value against which to perform the test. This can take
                  various forms depending on the data type.
    :param case_sensitive: Where relevant to the query handler, specifies
                  whether any text-based comparison is case sensitive.
    :return: A dict of data
    """
    # Select the query handler for the field, defaulting to "contains"
    query_handle = _SPECIAL_QUERY_HANDLERS.get(field,
                                               _query_string_contains)

    # Run the search
    return query_handle(data, field, value, case_sensitive)
