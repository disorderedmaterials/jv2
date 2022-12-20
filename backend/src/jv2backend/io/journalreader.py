# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 Team JournalViewer and contributors
"""Defines an abstract reader class and implementations for specific journal types"""
from abc import ABCMeta, abstractmethod

from jv2backend.journal import Journal
from jv2backend.journalfilelist import JournalFileList


class JournalReader(metaclass=ABCMeta):
    """Abstract interface"""

    @abstractmethod
    def read_indexfile(self, content: bytes) -> JournalFileList:
        raise NotImplementedError()  # pragma: no cover

    @abstractmethod
    def read_journalfile(self, content: bytes) -> Journal:
        raise NotImplementedError()  # pragma: no cover
