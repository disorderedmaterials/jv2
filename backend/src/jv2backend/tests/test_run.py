# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import dataclasses
from jv2backend.run import Run

import pytest


def test_Run_defines_accessors_for_each_field():
    run_attrs = dict(
        name="entryname",
        run_number="1",
        instrument_name="fake",
        experiment_identifier="5",
    )

    run = Run(run_attrs)

    for field in dataclasses.fields(Run):
        assert getattr(run, field.name) == run_attrs[field.name]


def test_missing_fields_raise_ValueError_during_creation():
    with pytest.raises(ValueError, match=".*experiment_identifier,instrument_name.*"):
        Run(dict(run_number="1"))
