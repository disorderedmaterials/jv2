# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors
import logging
from pathlib import Path
from typing import Optional

from jv2backend.io.rundatafilelocator import RunDataFileLocator


class PrefixPathFileLocator(RunDataFileLocator):
    """Base class to support setting/getting a prefix to run data paths"""

    def __init__(self, prefix: str) -> None:
        """
        :param prefix: Filesystem prefix for all run data.
        """
        super().__init__()
        self.prefix = prefix

    @property
    def prefix(self) -> Path:
        """Return the prefix for all run paths"""
        return self._prefix

    @prefix.setter
    def prefix(self, prefix: str):
        """
        :param prefix: A new Filesystem prefix for all run data
        """
        self._prefix = Path(prefix)


class DAaaSDataCacheFileLocator(PrefixPathFileLocator):
    """Implements file searching based on the ISIS
    IDAaaS data cache layout. The files can be found on the following path
    <prefix>/<instrument_name>/<YYYY>/RB<experiment_identifier>-<part_number>/<name>.nxs
    """

    EXTENSION = ".nxs"

    def __init__(self, prefix: str) -> None:
        """
        :param prefix: Filesystem prefix for all run data.
        """
        super().__init__(prefix)

    def locate(self, run: dict) -> Optional[Path]:
        """For a given run find the data file and return the full Path if the file
        was found or None if it could not be found

        :param run: A mapping describing the run
        """
        # Files for a given experiment can be split across directories, for the full path
        # format see the class docstring. Each RB directory is suffixed with an incrementing
        # part number after the RB number. Construct everything up to the year then glob
        # for the part number
        instrument, cycle_name, experiment_id = (
            run["instrument_name"],
            run["isis_cycle"],
            run["experiment_identifier"],
        )
        logging.debug(
            f"Locate: instrument={instrument}, isis_cycle={cycle_name}, rb={experiment_id}"
        )
        year_prefix = self._prefix / instrument / _cycle_year(cycle_name)
        logging.debug(f"Constructed year_prefix={year_prefix}")
        glob_expr = f"RB{experiment_id}-*/{run['name']}{self.EXTENSION}"
        logging.debug(f"Constructed glob_expr={glob_expr}")
        matches = list(year_prefix.glob(glob_expr))
        logging.debug(f"Glob matches: {matches}")

        if len(matches) > 0:
            return matches[-1]
        else:
            return None


class LegacyArchiveFileLocator(PrefixPathFileLocator):
    """Implments file searching based on the ISIS
    archive layout. Files can be found by constructing
    the following path

    <prefix>/<instrument_name.lower()>/Instrument/data/cycle_YY_N/<name>.nxs
    """

    EXTENSION = ".nxs"

    def __init__(self, prefix: str) -> None:
        """
        :param prefix: Filesystem prefix for all run data.
        """
        super().__init__(prefix)

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
        logging.debug(f"Locate: instrument={instrument}, isis_cycle={cycle_id}")
        filepath = (
            self._prefix
            / f"{instrument.lower()}"
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
