# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 Team JournalViewer and contributors
"""Define a interface for finding Run data files"""
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Optional


class RunDataFileLocator(ABC):
    """Subclass and implement the appropriate behaviour to find a Run"""

    @property
    @abstractmethod
    def prefix(self) -> Path:
        """Set a new base prefix for the file search

        :param prefix: A Path prefix that is the common base of all files
        """
        raise NotImplementedError()  # pragma: no cover

    @prefix.setter
    @abstractmethod
    def prefix(self, prefix: str):
        """Set a new base prefix for the file search

        :param prefix: A Path prefix that is the common base of all files
        """
        raise NotImplementedError()  # pragma: no cover

    @abstractmethod
    def locate(self, run: dict) -> Optional[Path]:
        """For a given run find the data file

        :param run: A mapping describing the run
        """
        raise NotImplementedError()  # pragma: no cover
