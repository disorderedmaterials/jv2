# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from pathlib import Path
import h5py as h5
import jv2backend.io.nexus as nxs

import pytest


def test_log_paths_returns_expected_paths_to_log_entries_if_file_is_accessible(
    sample_nexus_filepath,
):
    log_paths = nxs.logpaths_from_path(sample_nexus_filepath)

    assert len(log_paths) == 3
    # we don't care about the order
    for group in log_paths:
        assert group[0] in ("runlog", "selog", "framelog")
        for path in group[1:]:
            assert path.startswith(f"/raw_data_1/{group[0]}")


def test_log_paths_raises_IOError_if_file_not_accessible():
    with pytest.raises(IOError):
        nxs.logpaths_from_path(Path("/bad/file/path.nxs"))


def test_time_range_gives_start_end_time_as_tuple(sample_nexus_filepath):
    with h5.File(sample_nexus_filepath) as h5file:
        time_range = nxs.timerange(h5file["raw_data_1"])

    assert len(time_range) == 2
    for ts in time_range:
        assert "T" in ts


@pytest.mark.parametrize(
    "field_name", ["/raw_data_1/selog/Z", "/raw_data_1/runlog/dae_beam_current"]
)
def test_logvalues_gives_list_of_time_value_pairs_for_open_block(
    field_name, sample_nexus_filepath
):
    with h5.File(sample_nexus_filepath) as h5file:
        logdata = nxs.logvalues(h5file[field_name])

    if field_name.endswith("Z"):
        assert len(logdata) == 5
        assert logdata[0][0] == pytest.approx(-5779.0)
        assert logdata[0][1] == pytest.approx(200.0075)
    elif field_name.endswith("current"):
        assert len(logdata) == 191
        assert logdata[0][0] == pytest.approx(-5775.0)
        assert logdata[0][1] == pytest.approx(0.0)


def test_spectra_count_returns_the_number_spectra_in_detector_1_entry(
    sample_nexus_filepath,
):
    assert nxs.spectra_count(sample_nexus_filepath) == 2368


def test_monitor_count_returns_the_number_monitors_in_the_first_entry(
    sample_nexus_filepath,
):
    assert nxs.monitor_count(sample_nexus_filepath) == 3


def test_spectrum_returns_expected_spectrum_data(sample_nexus_filepath):
    data = nxs.spectrum(sample_nexus_filepath, spectrum=15)

    assert len(data) == 1361
    assert data[714][0] == pytest.approx(793.703125)
    assert data[714][1] == pytest.approx(1)


def test_monitor_spectrum_returns_expected_monitor_data(sample_nexus_filepath):
    data = nxs.monitor_spectrum(sample_nexus_filepath, monitor=2)

    assert len(data) == 1361
    assert data[714][0] == pytest.approx(793.703125)
    assert data[714][1] == pytest.approx(0)


def test_nonzero_spectra_returns_ratio_as_string(sample_nexus_filepath):
    ratio_str = nxs.nonzero_spectra_ratio(sample_nexus_filepath)

    assert ratio_str == "974/2368"
