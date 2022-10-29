# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import dataclasses
from jv2backend.run import Run
from jv2backend.experiment import Experiment
from jv2backend.instrument import Instrument

import pytest


def test_Run_created_with_required_fields_stores_them():
    run_attrs = dict(
        name="entryname",
        run_number="1",
        instrument=Instrument("fake"),
        experiment=Experiment(5),
    )

    run = Run(**run_attrs)

    for field in dataclasses.fields(Run):
        assert getattr(run, field.name) == run_attrs[field.name]


def test_missing_fields_raise_ValueError_during_creation():
    with pytest.raises(ValueError, match=".*experiment,instrument.*"):
        Run(run_number="1")
