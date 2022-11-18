# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
from abc import ABCMeta, abstractmethod
from typing import Optional

from jv2backend.journal import Journal
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

    @abstractmethod
    def journal(self, instrument_name: str, filename: str) -> Journal:
        """
        :param instrument_name: The instrument name
        :param cycle_name: Name of cycle whose journal should be returned
        :return: The list of journal filenames as strings
        """
        raise NotImplementedError

    @abstractmethod
    def check_for_journal_filenames_update(self, instrument_name: str) -> Optional[str]:
        """Check if the journal index files has been modified since last checked
        and return the latest entry if it has, otherwise return None

        :param instrument_name: The name of the instrument
        :return: The latest journal filename if there have been updates to the main list
        """
        raise NotImplementedError()

    @abstractmethod
    def search(
        self,
        instrument_name: str,
        run_field: str,
        user_input: str,
        case_sensitive: bool = False,
    ) -> Journal:
        """
        :param instrument_name: The instrument name
        :param run_field: Field to search over from all runs
        :param user_input: Search query
        :param case_sensitive: If True, use case sensitive searching
        :return: A Journal of the runs matching the search query
        """
        raise NotImplementedError
