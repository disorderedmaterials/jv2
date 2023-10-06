# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""Tests for the Cycle class"""
from jv2backend.cycle import Cycle


def test_Cycle_can_be_constructed_from_year_and_number():
    cycle = Cycle(2022, 2)

    assert cycle.year == 2022
    assert cycle.number == 2
