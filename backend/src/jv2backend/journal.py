# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines a Journal class to encapsulate a Journal for user cycle"""
from jv2backend.cycle import Cycle
from jv2backend.instrument import Instrument


class Journal:
    """A Journal captures data from a user cycle on a given instrument, stored
    as a collection of Run objects. The order is not defined."""

    def __init__(self, instrument: Instrument, cycle: Cycle) -> None:
        """
        :param cycle: Defines the associated cycle for this run
        """
        self._instrument = instrument
        self._cycle = cycle
        self._runs = []

    @property
    def instrument(self) -> Instrument:
        return self._instrument

    @property
    def cycle(self) -> Cycle:
        return self._cycle

    def run_count(self) -> int:
        """Return the number of runs listed within this Journal"""
        return 0
