# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

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
        raise NotImplementedError()  # pragma: no cover

    @abstractmethod
    def journal(
        self,
        instrument_name: str,
        *,
        filename: Optional[str] = None,
        cyclename: Optional[str] = None
    ) -> Journal:
        """
        :param instrument_name: The instrument name
        :param filename: The filename of the journal that should be returned
        :param cyclename: Name of cycle whose journal should be returned.
        If both this and filename are provided then filename takes precendence.
        :return: The list of journal filenames as strings
        """
        raise NotImplementedError()  # pragma: no cover

    @abstractmethod
    def filename_for_run(self, instrument: str, run: str) -> Optional[str]:
        """Return the filename for the given run"""
        raise NotImplementedError()  # pragma: no cover

    @abstractmethod
    def check_for_journal_filenames_update(self, instrument_name: str) -> Optional[str]:
        """Check if the journal index files has been modified since last checked
        and return the latest entry if it has, otherwise return None

        :param instrument_name: The name of the instrument
        :return: The latest journal filename if there have been updates to the main list
        """
        raise NotImplementedError()  # pragma: no cover

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
        raise NotImplementedError()  # pragma: no cover
