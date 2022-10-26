# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Capture Run information"""

from dataclasses import dataclass
from jv2backend.experiment import Experiment
from jv2backend.instrument import Instrument


@dataclass(init=False)
class Run:
    """Capture an experiment Run"""

    number: int
    instrument: Instrument
    experiment: Experiment

    def __init__(
        self, run_number: int, instrument: Instrument, experiment: Experiment
    ) -> None:
        """
        :param run_number: The incrementing number identifying this run
        :param instrument: The instrument associated with this run
        """
        self.number = run_number
        self.instrument = instrument
        self.experiment = experiment
