# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Encapsulate an Instrument description"""

from dataclasses import dataclass


@dataclass
class Instrument:
    """Defines an Instrument"""

    name: str
