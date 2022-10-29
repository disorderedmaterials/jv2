# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.run import Run
from jv2backend.experiment import Experiment
from jv2backend.instrument import Instrument

import pytest


def test_run_requires_number_instrument_and_experiment():
    fake_instrument = Instrument("fake")
    fake_experiment = Experiment(5)
    run_number = 1

    run = Run(
        run_number=run_number, instrument=fake_instrument, experiment=fake_experiment
    )

    assert run.instrument == fake_instrument
    assert run.run_number == run_number
    assert run.experiment == fake_experiment


def test_missing_fields_raise_ValueError_on_init():
    with pytest.raises(ValueError, match=".*run_number,experiment,instrument.*"):
        Run()

    with pytest.raises(ValueError, match=".*experiment,instrument.*"):
        Run(run_number=1)


def test_field_with_incorrect_type_raises_TypeError():
    with pytest.raises(TypeError, match=".*run_number.*"):
        Run(run_number="1", instrument=1, experiment=6)
