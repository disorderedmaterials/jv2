# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Provides a Cycle class to capture details of single user cycle"""

from dataclasses import dataclass


@dataclass
class Cycle:
    """Dataclass capturing metadata defining a cycle"""

    # The year in which the cycle is attributed.
    # Note that cycles can run in the next year but still be attributed
    # to the previous, e.g. facility cycle years running Apr->Mar
    year: int
    # The ID number of the cycle within the corresponding year
    number: int
