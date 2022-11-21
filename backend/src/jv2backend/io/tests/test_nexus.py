# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from pathlib import Path
import jv2backend.io.nexus as nxs

import pytest


def test_log_paths_returns_expected_paths_to_log_entries_if_file_is_accessible(
    sample_nexus_filepath,
):
    log_paths = nxs.logpaths(sample_nexus_filepath)

    assert len(log_paths) == 2
    # we don't care about the order
    for group in log_paths:
        assert group[0] in ("runlog", "selog")
        for path in group[1:]:
            assert path.startswith(f"/raw_data_1/{group[0]}")


def test_log_paths_raises_IOError_if_file_not_accessible():
    with pytest.raises(IOError):
        nxs.logpaths(Path("/bad/file/path.nxs"))
