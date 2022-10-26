# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.run import Run
from jv2backend.instrument import Instrument

def test_run_contructed_with_number_and_instrument_name():
    fake_instrument = Instrument("fake")
    run_number = 1
    run = Run(run_number, fake_instrument)

    assert(run.instrument == fake_instrument)
    assert(run.number == run_number)
