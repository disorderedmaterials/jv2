# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines an abstract reader class and implementations for specific journal types"""
from abc import ABCMeta, abstractmethod
from typing import Iterable

from jv2backend.instrument import Instrument
from jv2backend.journal import Journal


class JournalReader(metaclass=ABCMeta):
    """Abstract interface"""

    @abstractmethod
    def read(self, journalfile: Iterable[str], instrument: Instrument) -> Journal:
        raise NotImplementedError
