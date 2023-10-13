# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
import datetime as dt
from typing import Optional, Sequence
import pandas as pd

# Format of start time search string
USERINPUT_DT_FORMAT_STR = "%Y/%m/%d"


def _to_datetime(user_input: str) -> dt.datetime:
    """Convert from a user input string of format YYY/MM/DD to a datetime object"""
    return dt.datetime.strptime(user_input, USERINPUT_DT_FORMAT_STR)


# Query handlers
# A handler should have the form Callable[[pd.DataFrame, str, bool], pd.DataFrame]
def contains(
    data: pd.DataFrame, column: str, text: str, case_sensitive: bool
) -> pd.DataFrame:
    """Return the rows of the input DataFrame where the text string is
    in the column values

    :param data: The input data
    :param column: The name of the column that should be matched
    :param text: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A new DataFrame with the selected rows
    """
    return data[data[column].str.contains(text, case=case_sensitive)]


def equals(
    data: pd.DataFrame, column: str, text: str, case_sensitive: bool
) -> pd.DataFrame:
    """Return the rows of the input DataFrame where the text string matches
    the column value.

    :param data: The input data
    :param column: The name of the column that should be matched
    :param text: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A new DataFrame with the selected rows
    """
    if case_sensitive:
        return data[data[column] == text]
    else:
        return data[data[column].str.lower() == text.lower()]


def inrange_int(data: pd.DataFrame, column: str, text: str, _: bool) -> pd.DataFrame:
    """Return the rows of the input DataFrame where range provided in the
    text is included. It is assumed the column
    can be converted to an integer and the search is insclusive

    :param data: The input data
    :param column: The name of the column that should be matched. It should be convertible to an integer
    :param text: Input query:
                   - Use a "start-end" to indicate an inclusive range
                   - or "(OP)value" where (OP) is one of "<,>"
                 An empty result is returned if the format is incorrect
    :param _: Ignored in this case. Required by API
    :return: A new DataFrame with the selected rows
    """
    try:
        column_values = data[column].astype(int)
        if "-" in text:
            start, end = text.split("-")
            start, end = int(start.strip()), int(end.strip())
            return data[column_values.between(start, end, inclusive="both")]
        else:
            op, value = text[0], text[1:]
            return data.query(f"@column_values {op} {value}")

    except ValueError:  # bad format
        return pd.DataFrame()


def inrange_datetime(
    data: pd.DataFrame, column: str, text: str, _: bool
) -> pd.DataFrame:
    """Return the rows of the input DataFrame where datetime provided in the
    text (using a "-" separator) is included. It is assumed the column
    can be converted to an datetime and the search is insclusive

    :param data: The input data
    :param column: The name of the column that should be matched. It should be convertible to an datetime object
    :param text: Text to search in the format "YYYY/MM/DD-YYYY/MM/DD". An empty result is returned if the format is incorrect
    :param _: Ignored in this case. Required by API
    :return: A new DataFrame with the selected rows
    """
    try:
        start, end = text.split("-")
        start, end = _to_datetime(start.strip()), _to_datetime(end.strip())
    except ValueError:  # bad format
        return pd.DataFrame()

    column_values = pd.to_datetime(data[column])
    return data[column_values.between(start, end, inclusive="both")]


# Map a field name to a handler for that query if it should have special handling
_SPECIAL_QUERY_HANDLERS = {
    "experiment_identifier": equals,
    "run_number": inrange_int,
    "start_time": inrange_datetime,
}


# Journal class
class Journal:
    """A Journal captures records of runs from a given source"""

    def __init__(self, source: str, data: pd.DataFrame) -> None:
        """
        :param instrument: Defines the instrument associated with this
        """
        self._source = source
        self._data = data
        # todo: check instrument matches data

    @property
    def instrument(self) -> str:
        return self._source

    @property
    def run_count(self) -> int:
        """Return the number of runs listed within this Journal"""
        return len(self._data)

    @property
    def last_run_number(self) -> int:
        """Return the run number of the last run in the journal"""
        run_numbers = self._data["run_number"]
        return int(run_numbers.max())

    def run(self, run_number: str) -> Optional[dict]:
        """Return a dictionary describing the given run number

        :param run_number: Run number to select
        :return: A dict describing the Run or None if the run does not exist
        """
        matches = self._data[self._data["run_number"] == run_number]
        if len(matches) == 0:
            return None
        else:
            return matches.to_dict(orient="records")[0]

    def search(
        self, run_field: str, user_input: str, case_sensitive: bool = False
    ) -> Journal:
        """Search across the runs for those matching the user_input over the run_field"""
        # Different fields need handling differently but we will fallback to a basic
        # "is in string check"
        query_handle = _SPECIAL_QUERY_HANDLERS.get(run_field, contains)

        return Journal(
            self.source,
            query_handle(self._data, run_field, user_input, case_sensitive),
        )

    # Output formats
    def to_json(self) -> str:
        """Return the collection of runs as a list[dict()] formatted as requested"""
        return self._data.to_json(orient="records")


# Operations on multiple journals
def concatenate(journals: Sequence[Journal]) -> Journal:
    """Concatenate the Journals to a single Journal

    :param journals: Sequence of Journal objects
    :return: A new Journal, the result of concatenating the input journals into one
    """
    return Journal(
        journals[0].source, pd.concat([journal._data for journal in journals])
    )
