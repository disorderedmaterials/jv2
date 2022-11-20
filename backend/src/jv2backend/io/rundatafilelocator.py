# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Define a interface for finding Run data files"""
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Optional


class RunDataFileLocator(ABC):
    """Subclass and implement the appropriate behaviour to find a Run"""

    @abstractmethod
    def locate(self, run: dict) -> Optional[Path]:
        """For a given run find the data file

        :param run: A mapping describing the run
        """
        raise NotImplementedError()  # pragma: no cover
