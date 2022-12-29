# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 Team JournalViewer and contributors
"""Encapsulate an Instrument description"""

from dataclasses import dataclass


@dataclass
class Instrument:
    """Defines an Instrument"""

    name: str
