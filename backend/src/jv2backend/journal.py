# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines a Journal class to encapsulate a collection of Runs on an instrument"""
from __future__ import annotations
from typing import Sequence
import pandas as pd

from jv2backend.instrument import Instrument


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


# Map a field name to a handler for that query.
_QUERY_HANDLERS = {"user_name": contains}

# Journal class
class Journal:
    """A Journal captures records of runs on a given instrument"""

    def __init__(self, instrument: Instrument, data: pd.DataFrame) -> None:
        """
        :param instrument: Defines the instrument associated with this
        """
        self._instrument = instrument
        self._data = data
        # todo: check instrument matches data

    @property
    def instrument(self) -> Instrument:
        return self._instrument

    @property
    def run_count(self) -> int:
        """Return the number of runs listed within this Journal"""
        return len(self._data)

    def search(
        self, run_field: str, user_input: str, case_sensitive: bool = False
    ) -> Journal:
        """Search across the runs for those matching the user_input over the run_field"""
        # Different fields need handling differently but we will fallback to a basic
        # "is in string check"
        query_handle = _QUERY_HANDLERS.get(run_field, contains)

        return Journal(
            self.instrument,
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
        journals[0].instrument, pd.concat([journal._data for journal in journals])
    )
