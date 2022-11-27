# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import logging
from pathlib import Path
from typing import Optional

from jv2backend.io.rundatafilelocator import RunDataFileLocator


class DAaaSDataCacheFileLocator(RunDataFileLocator):
    """Implments file searching based on the ISIS
    IDAaaS data cache layout. Files can be found by constructing
    the following path

    <prefix>/<instrument_name>/<YYYY>/RB<experiment_identifier>-<part_number>/<name>.nxs

    The part_number has to be determined by globbing for the required file
    """

    EXTENSION = ".nxs"

    def __init__(self, prefix: str) -> None:
        """
        :param prefix: Filesystem prefix for all run data.
        """
        super().__init__()
        self._prefix = Path(prefix)

    def locate(self, run: dict) -> Optional[Path]:
        """For a given run find the data file and return the full Path if the file
        was found or None if it could not be found

        :param run: A mapping describing the run
        """
        instrument, cycle_name, experiment_id = (
            run["instrument_name"],
            run["isis_cycle"],
            run["experiment_identifier"],
        )
        cycle_prefix = self._prefix / instrument / _cycle_year(cycle_name)
        # Files for an experiment can be split across directories each with an incrementing
        # part number after the RB number. If multiple matches are found for the same
        # file then the file from the latest part is returned
        matches = list(
            cycle_prefix.glob(f"RB{experiment_id}-*/{run['name']}{self.EXTENSION}")
        )
        if len(matches) > 0:
            return matches[-1]
        else:
            return None


class LegacyArchiveFileLocator(RunDataFileLocator):
    """Implments file searching based on the ISIS
    archive layout. Files can be found by constructing
    the following path

    <prefix>/ndx<instrument_name.lower()>/Instrument/data/cycle_YY_N/<name>.nxs
    """

    EXTENSION = ".nxs"

    def __init__(self, prefix: str) -> None:
        """
        :param prefix: Filesystem prefix for all run data.
        """
        super().__init__()
        self._prefix = Path(prefix)

    def locate(self, run: dict) -> Optional[Path]:
        """For a given run find the data file and return the full Path if the file
        was found or None if it could not be found

        :param run: A mapping describing the run.
        """
        logging.debug(f"Looking for file for run {run['run_number']}")
        instrument, cycle_id = (
            run["instrument_name"],
            run["isis_cycle"],
        )
        filepath = (
            self._prefix
            / f"ndx{instrument.lower()}"
            / "Instrument"
            / "data"
            / f"cycle_{cycle_id}"
            / f"{run['name']}{self.EXTENSION}"
        )
        logging.debug(f"Constructed expected path: {filepath}")
        if filepath.exists():
            return filepath
        else:
            return None


# private helpers

_CYCLE_CENTURY = "20"


def _cycle_year(cycle_name: str) -> str:
    """Return the full year as YYYY from
    a cycle name such as YY_n. This func

    :param cycle_name: Short-form name of the cycle in format YY_n
    :return: The year attached to this cycle
    """
    return f"{_CYCLE_CENTURY}{cycle_name.split('_')[0]}"
