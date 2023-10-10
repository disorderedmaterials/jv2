# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import logging
from pathlib import Path
from typing import Optional


class RunDataFileLocator:
    """Define an interface for finding run data files
    
    Implements file searching based on the ISIS
    archive layout. Files can be found by constructing
    the following path

    <prefix>/<instrument_name.lower()>/Instrument/data/cycle_YY_N/<name>.nxs
    """

    EXTENSION = ".nxs"

    def __init__(self, prefix: str) -> None:
        """
        :param prefix: Filesystem prefix for all run data.
        """
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
