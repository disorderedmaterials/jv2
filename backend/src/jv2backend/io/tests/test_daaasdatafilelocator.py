# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from jv2backend.io.isis.daaasdatafilelocator import DAaaSDataCacheFileLocator

import os
import pytest

# Note: in these tests 'fs' refers to the fake filesystem fixture in
# pyfakefs: https://pytest-pyfakefs.readthedocs.io/en/stable/usage.html


@pytest.fixture()
def datacache_faker(fs, sample_run):
    def _impl(prefix: str, part_number: int):
        instrument, cycle_year, experiment_id = (
            sample_run["instrument_name"],
            2021,
            sample_run["experiment_identifier"],
        )
        fs.create_file(
            f"/{prefix}/{instrument}/{cycle_year}/RB{experiment_id}-{part_number}/{sample_run['name']}.nxs"
        )

    return _impl


@pytest.mark.parametrize("part_number", [1, 3, 5])
def test_path_found_when_file_exists_in_one_part_number_directory(
    part_number, sample_run, datacache_faker
):
    prefix = "/data"
    datacache_faker(prefix, part_number)
    filefinder = DAaaSDataCacheFileLocator(prefix)

    path = filefinder.locate(sample_run)

    assert path is not None
    assert path.exists()
    assert path.name == sample_run["name"] + ".nxs"


def test_path_found_uses_highest_part_number_when_file_exists_in_multiple_dirs(
    sample_run, datacache_faker
):
    prefix = "/data"
    for part_number in (1, 2, 3, 4, 5):
        datacache_faker(prefix, part_number)
    filefinder = DAaaSDataCacheFileLocator(prefix)

    path = filefinder.locate(sample_run)

    assert path is not None
    assert path.exists()
    assert path.name == sample_run["name"] + ".nxs"
    assert "-5" in str(path)


def test_locator_created_with_non_existing_prefix_does_not_raise_error():
    filefinder = DAaaSDataCacheFileLocator("/nothere")

    assert filefinder is not None
