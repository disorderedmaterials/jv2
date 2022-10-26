# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.run import Run
from jv2backend.experiment import Experiment
from jv2backend.instrument import Instrument


def test_run_requires_number_instrument_and_experiment():
    fake_instrument = Instrument("fake")
    fake_experiment = Experiment(5)
    run_number = 1

    run = Run(run_number, fake_instrument, fake_experiment)

    assert run.instrument == fake_instrument
    assert run.number == run_number
    assert run.experiment == fake_experiment
