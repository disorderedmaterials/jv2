# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from jv2backend.journals import JournalData
import xml.etree.ElementTree as ET
import pandas
import pytest

# Load test run data
with open("jv2backend/tests/data/simpleRunData1.xml", "rb") as f:
    runData = pandas.read_xml(f, dtype=str)
#    runData = ET.parse(f)


def test_basic_constructor():
    data = JournalData(runData)

    assert data.run_count == 6


def test_run_data_ranges():
    data = JournalData(runData)

    assert data.get_last_run_number() == 9


@pytest.mark.parametrize("run_number", [1, 2, 3, 6, 8, 9])
def test_run_data_contains_run_number(run_number):
    data = JournalData(runData)

    assert run_number in data
    assert data.run(run_number) is not None


@pytest.mark.parametrize("run_number", [0, 4, 5, 7, 99, 12301])
def test_run_data_does_not_contain_run_number(run_number):
    data = JournalData(runData)

    assert run_number not in data
    assert data.run(run_number) is None
