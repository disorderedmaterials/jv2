# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.experiment import Experiment
import pytest


def test_experiment_requires_identifier_on_construction():
    identifier = 0
    experiment = Experiment(identifier)

    assert experiment.identifier == 0


def test_experiment_identifier_is_readonly():
    experiment = Experiment(0)

    with pytest.raises(AttributeError):
        experiment.identifier = 1
