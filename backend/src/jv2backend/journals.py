# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
from dataclasses import dataclass
import typing
from jv2backend.utils import url_join
import jv2backend.select as Selector
from jv2backend.journalFile import JournalFile
import logging


@dataclass
class JournalCollection:
    """Defines a collection of journal files"""

    # The available journal files within a collection
    journalFiles: typing.List[JournalFile]

    def __init__(self, journalFiles: typing.List[JournalFile]) -> None:
        self.journalFiles = journalFiles

    def journal_for_run(self, run_number: int) -> JournalFile:
        """Find the journal in the collection that contains the specified run
        number.

        :param run_number: Run number to locate
        :return: JournalFile containing the run number, or None if not found
        """
        return next(
            (jf for jf in self.journalFiles if run_number in jf),
            None)

    def locate_data_file(self, run_number: int) -> str:
        """Return the full path to the data (NeXuS) file for the specified
        run number
        :param run_number: Run number to locate the data file for
        :return: Full path to the data file or None if it couldn't be found
        """
        # Get the journal file for the specified run number
        jf = self.journal_for_run(run_number)
        if jf is None:
            return None
        logging.debug(f"Run number {run_number} exists in journal "
                      f"{jf.filename}")

        # Get the data for the specified run number
        data = jf.get_data(run_number)

        # The journal entry may contain the full data_directory and filename
        # information if we generated it. Otherwise we have to assume the
        # stored 'data_directory' and use the 'name' attribute.
        if "data_directory" in data and "filename" in data:
            return url_join(data["data_directory"], data["filename"])
        else:
            return url_join(jf.data_directory, data["name"] + ".nxs")

    def locate_data_files(
            self, run_numbers: typing.List[int]
    ) -> typing.Dict[int, str]:
        """Return a dict of run number/paths to NeXuS data files
        If a run number is not locatable, return None for that entry

        :param run_numbers: A list of integer run numbers to locate
        :return: A Dict[int,str] mapping of integer run number to either
                 to eithr NeXuS file path or None if not locatable
        """
        # For each of the supplied run numbers, find its parent journal
        result = {}
        for i in run_numbers:
            result[i] = self.locate_data_file(i)
        return result

    def get_journal(self, filename: str) -> JournalFile:
        return next(
            (jf for jf in self.journalFiles if jf.filename == filename),
            None)

    def to_basic(self) -> typing.List[BasicJournalFile]:
        basic = []
        for x in self.journalFiles:
            basic.append(x.from_derived(x))
        return basic

    def search(self, search_terms: {}) -> {}:
        """
        Search across all journals in the collection, selecting those which
        match _all_ of the specified search_terms. The search terms are
        applied sequentially in the order they appear in the dict.

        :param search_terms: Dict of search field/values
        :return: A dict of runs matching the search query.
        """
        results = {}

        # See if we have a case-sensitive flag
        case_sensitive = ("caseSensitive" in search_terms and
                          search_terms["caseSensitive"] == "true")
        if "caseSensitive" in search_terms:
            del search_terms["caseSensitive"]

        for jf in self.journalFiles:
            logging.debug(f"Journal {jf.filename} .....")
            if jf.run_data is None:
                continue
            matches = None
            # Cycle over search terms
            # If the current 'matches' is None then search the whole run data
            # If it is not, then search it instead (chaining searches)
            # If it is ever a size of zero we have excluded all runs
            logging.debug("Starting loop over run data...")
            for field in search_terms:
                matches = Selector.select(jf.run_data.data if matches is None
                                          else matches,
                                          field,
                                          search_terms[field],
                                          case_sensitive)
                logging.debug(f"...after checking '{field}' there are "
                              f"{len(matches)} matches...")
                if len(matches) == 0:
                    break

            if matches is None:
                continue

            logging.debug(f"Journal {jf.filename} matched {len(matches)} runs.")
            results.update(matches)

        return results


@dataclass
class JournalLibrary:
    """Defines one or more data source rootURL/directory and their associated
    journal collections.
    """
    collections: typing.Dict[str, JournalCollection]

    def __setitem__(self, key, value):
        self.collections[key] = value

    def __getitem__(self, key):
        if key in self.collections:
            return self.collections[key]
        else:
            return None

    def __contains__(self, key):
        return key in self.collections

    def list(self):
        """List contents of library"""
        for c in self.collections:
            logging.debug(f"Collection '{c}' contains "
                          f"{len(self.collections[c].journalFiles)} "
                          f"journal files:")
            for j in self.collections[c].journalFiles:
                if j.run_data is None:
                    logging.debug(f"     {j} (not yet loaded)")
                else:
                    logging.debug(f"     {j} ({j.run_data.run_count} run data)")
