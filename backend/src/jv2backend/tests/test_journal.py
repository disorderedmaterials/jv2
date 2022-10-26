# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.cycle import Cycle
from jv2backend.instrument import Instrument
from jv2backend.journal import Journal


def test_default_journal_creation_stores_cycle_and_instrument_and_contains_zero_runs():
    instrument = Instrument("fake")
    cycle = Cycle(2021, 1)
    journal = Journal(instrument, cycle)

    assert journal.instrument == instrument
    assert journal.cycle == cycle
    assert journal.run_count() == 0
