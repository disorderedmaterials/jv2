# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from abc import ABCMeta, abstractmethod
from jv2backend.journalfilelist import JournalFileList


class JournalServer(metaclass=ABCMeta):
    """Interface for server application interfacing with server
    holding Journal data. Concrete implmentations should read data provided by the
    server and translates it to backend data types."""

    @abstractmethod
    def journal_filenames(self, instrument_name: str) -> JournalFileList:
        """
        :param instrument_name: The instrument name
        :return: The list of journal filenames as strings
        """
        raise NotImplementedError
