# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines a Journal class to encapsulate a Journal for user cycle"""
from typing import Iterable

import pandas as pd

from jv2backend.cycle import Cycle
from jv2backend.instrument import Instrument

from jv2backend.run import Run


class Journal:
    """A Journal captures data from a user cycle on a given instrument"""

    def __init__(self, instrument: Instrument, data: pd.DataFrame) -> None:
        """
        :param cycle: Defines the associated cycle for this run
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
