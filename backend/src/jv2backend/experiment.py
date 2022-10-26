# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Encapsulate an Experiment description"""


class Experiment:
    """Encapsulate details of an experiment as a set of runs"""

    def __init__(self, identifier: int):
        """Initialize an Experiment

        :param identifier: Identifying number, also called RB number,
                           for this experiment
        """
        self._identifier = identifier

    @property
    def identifier(self) -> int:
        return self._identifier
