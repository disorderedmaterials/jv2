# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines a Journal class to encapsulate a collection of Runs on an instrument"""

import pandas as pd

from jv2backend.instrument import Instrument


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

    def run_count(self) -> int:
        """Return the number of runs listed within this Journal"""
        return len(self._data)

    def runs(self, format="json") -> str:
        """Return the collection of runs as a list[dict()] formatted as requested"""
        if format != "json":
            raise ValueError(
                f"Unsupported format type '{format}'. Available formats=json"
            )

        return self._data.to_json(orient="records")

    # def run(self, index: int) -> Run:
    #     """Return a run at the given index or raise IndexError if the
    #     index is out of bounds.

    #     :param index: Index of run in list
    #     :return: Run object
    #     """
    #     return self._runs[index]


# Queries

# def total_uamps(journal: Journal, run_numbers: Sequence[str]) -> Sequence[str]:
