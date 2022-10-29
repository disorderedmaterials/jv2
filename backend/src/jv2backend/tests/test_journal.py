# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.cycle import Cycle
from jv2backend.experiment import Experiment
from jv2backend.instrument import Instrument
from jv2backend.run import Run
from jv2backend.journal import Journal

from unittest.mock import MagicMock
import pytest


def test_default_journal_creation_stores_cycle_and_instrument_and_contains_zero_runs():
    instrument_name, year, cycle_number = "fake", 2021, 1
    journal = _create_test_journal(year, instrument_name, cycle_number)

    assert journal.instrument.name == instrument_name
    assert journal.cycle.year == year
    assert journal.cycle.number == cycle_number
    assert journal.run_count() == 0


def test_adding_run_from_a_different_instrument_raises_a_ValueError():
    journal = _create_test_journal(instrument_name="fake", year=2021, cycle_number=2)
    run = MagicMock(spec=Run, instrument=Instrument("other"))

    with pytest.raises(ValueError):
        journal.add_run(run)


def test_adding_run_from_matching_instrument_is_accepted():
    instrument_name, year, cycle_number = "fake", 2021, 1
    journal = _create_test_journal(year, instrument_name, cycle_number)
    run = MagicMock(spec=Run, instrument=journal.instrument)

    journal.add_run(run)

    assert journal.run_count() == 1


# Private helpers
def _create_test_journal(year: int, instrument_name: str, cycle_number: int) -> Journal:
    instrument = Instrument(instrument_name)
    cycle = Cycle(year, cycle_number)
    return Journal(instrument, cycle)
