# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from pathlib import Path
from jv2backend.io.journals.networkLocator import NetworkJournalLocator
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
            f"{prefix}/{instrument}/{cycle_year}/RB{experiment_id}-{part_number}/{sample_run['name']}.nxs"
        )

    return _impl


@pytest.fixture()
def archive_faker(fs, sample_run):
    def _impl(prefix: str):
        instrument, cycle_name = (
            sample_run["instrument_name"],
            "cycle_21_1",
        )
        fs.create_file(
            f"{prefix}/{instrument.lower()}/Instrument/data/{cycle_name}/{sample_run['name']}.nxs"
        )

    return _impl


# ======================== DAaaSDataCache =========================


def test_prefix_property_returns_expected_value_on_construction():
    prefix = "/mnt"
    filefinder = DAaaSDataCacheFileLocator(prefix)

    assert filefinder.prefix == Path(prefix)


def test_prefix_setter_accepts_new_prefix():
    prefix = "/mnt1"
    filefinder = DAaaSDataCacheFileLocator(prefix)

    new_prefix = "/mnt2"
    filefinder.prefix = new_prefix

    assert filefinder.prefix == Path(new_prefix)


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


# ======================== LegacyArchive =========================


def test_prefix_property_returns_expected_value_on_construction():
    prefix = "/mnt"
    filefinder = LegacyArchiveFileLocator(prefix)

    assert filefinder.prefix == Path(prefix)


def test_prefix_setter_accepts_new_prefix():
    prefix = "/mnt1"
    filefinder = LegacyArchiveFileLocator(prefix)

    new_prefix = "/mnt2"
    filefinder.prefix = new_prefix

    assert filefinder.prefix == Path(new_prefix)


def test_legacyarchive_finds_existing_file(sample_run, archive_faker):
    prefix = "/data"
    archive_faker(prefix)
    filefinder = LegacyArchiveFileLocator(prefix)

    path = filefinder.locate(sample_run)

    assert path is not None
    assert path.exists()
    assert path.name == sample_run["name"] + ".nxs"
    assert "Instrument/data" in str(path)


def test_legacyarchive_returns_None_for_none_existing_file(sample_run, archive_faker):
    prefix = "/data"
    archive_faker(prefix)
    sample_run["name"] = "ALF00000000.nxs"
    filefinder = LegacyArchiveFileLocator(prefix)

    path = filefinder.locate(sample_run)

    assert path is None
