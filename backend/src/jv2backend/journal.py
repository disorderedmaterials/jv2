# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines a Journal class to encapsulate a Journal for user cycle"""
from jv2backend.cycle import Cycle
from jv2backend.instrument import Instrument
from jv2backend.run import Run


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
        return len(self._runs)

    def add_run(self, run: Run) -> None:
        """Adds a run to the collection of managed runs for this Journal.
        The run must belong to the same instrument that associated with this
        Journal or a ValueError is raised.

        :param run: A Run to be added to this Journal. The instruments must match
        :raises ValueError: If the instrument does not match that defined
                            when the Journal was created
        """
        if run.instrument != self.instrument:
            raise ValueError(
                f"This journal is defined for the '{self.instrument.name}' "
                f"instrument but the run provided is associated with '{run.instrument.name}'"
            )

        self._runs.append(run)
