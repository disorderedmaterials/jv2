# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
from dataclasses import dataclass
from typing import List, Dict
import datetime as dt
from typing import Optional, Sequence
from jv2backend.utils import url_join
import pandas as pd
import logging

# Format of start time search string
USERINPUT_DT_FORMAT_STR = "%Y/%m/%d"


def _to_datetime(user_input: str) -> dt.datetime:
    """Convert from a string of format YYY/MM/DD to a datetime object"""
    return dt.datetime.strptime(user_input, USERINPUT_DT_FORMAT_STR)


# Query handlers
# A handler should have the form Callable[[pd.DataFrame, str, bool],
# pd.DataFrame]
def contains(
    data: pd.DataFrame, column: str, text: str, case_sensitive: bool
) -> pd.DataFrame:
    """Return the rows of the input DataFrame where the text string is
    in the column values

    :param data: The input data
    :param column: The name of the column that should be matched
    :param text: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A new DataFrame with the selected rows
    """
    return data[data[column].str.contains(text, case=case_sensitive)]


def equals(
    data: pd.DataFrame, column: str, text: str, case_sensitive: bool
) -> pd.DataFrame:
    """Return the rows of the input DataFrame where the text string matches
    the column value.

    :param data: The input data
    :param column: The name of the column that should be matched
    :param text: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A new DataFrame with the selected rows
    """
    if case_sensitive:
        return data[data[column] == text]
    else:
        return data[data[column].str.lower() == text.lower()]


def inrange_int(data: pd.DataFrame, column: str,
                text: str, _: bool) -> pd.DataFrame:
    """Return the rows of the input DataFrame where range provided in the
    text is included. It is assumed the column
    can be converted to an integer and the search is insclusive

    :param data: The input data
    :param column: The name of the column that should be matched. It should be
                   convertible to an integer
    :param text: Input query:
                   - Use a "start-end" to indicate an inclusive range
                   - or "(OP)value" where (OP) is one of "<,>"
                 An empty result is returned if the format is incorrect
    :param _: Ignored in this case. Required by API
    :return: A new DataFrame with the selected rows
    """
    try:
        column_values = data[column].astype(int)
        if "-" in text:
            start, end = text.split("-")
            start, end = int(start.strip()), int(end.strip())
            return data[column_values.between(start, end, inclusive="both")]
        else:
            op, value = text[0], text[1:]
            return data.query(f"@column_values {op} {value}")

    except ValueError:  # bad format
        return pd.DataFrame()


def inrange_datetime(
    data: pd.DataFrame, column: str, text: str, _: bool
) -> pd.DataFrame:
    """Return the rows of the input DataFrame where datetime provided in the
    text (using a "-" separator) is included. It is assumed the column
    can be converted to an datetime and the search is insclusive

    :param data: The input data
    :param column: The name of the column that should be matched. It should be
                   convertible to an datetime object
    :param text: Text to search in the format "YYYY/MM/DD-YYYY/MM/DD". An
                 empty result is returned if the format is incorrect
    :param _: Ignored in this case. Required by API
    :return: A new DataFrame with the selected rows
    """
    try:
        start, end = text.split("-")
        start, end = _to_datetime(start.strip()), _to_datetime(end.strip())
    except ValueError:  # bad format
        return pd.DataFrame()

    column_values = pd.to_datetime(data[column])
    return data[column_values.between(start, end, inclusive="both")]


# Map a field name to a handler for that query if it should have special
# handling
_SPECIAL_QUERY_HANDLERS = {
    "experiment_identifier": equals,
    "run_number": inrange_int,
    "start_time": inrange_datetime,
}


# JournalData class
class JournalData:
    """JournalData captures all run information from a given journal file"""

    def __init__(self, source: str, data: pd.DataFrame) -> None:
        """
        :param source: Defines the source filename
        """
        self._source = source
        self._data = data

    @property
    def instrument(self) -> str:
        return self._source

    @property
    def run_count(self) -> int:
        """Return the number of runs listed within this Journal"""
        return len(self._data)

    @property
    def get_last_run_number(self) -> int:
        """Return the run number of the last run in the journal"""
        run_numbers = self._data["run_number"]
        return int(run_numbers.max())

    def run(self, run_number: str) -> Optional[dict]:
        """Return a dictionary describing the given run number

        :param run_number: Run number to select
        :return: A dict describing the Run or None if the run does not exist
        """
        matches = self._data[self._data["run_number"] == run_number]
        if len(matches) == 0:
            return None
        else:
            return matches.to_dict(orient="records")[0]

    def search(
        self, run_field: str, user_input: str, case_sensitive: bool = False
    ) -> JournalData:
        """Search data for runs whose run_field matches the user_input"""
        # Different fields need handling differently but we will fallback to a
        # basic "is in string check"
        query_handle = _SPECIAL_QUERY_HANDLERS.get(run_field, contains)

        return JournalData(
            self.source,
            query_handle(self._data, run_field, user_input, case_sensitive),
        )

    # Output formats
    def to_json(self) -> str:
        return self._data.to_json(orient="records")


# Operations on multiple journal data
def concatenate(journals: Sequence[JournalData]) -> JournalData:
    """Concatenate the Journals to a single Journal

    :param journals: Sequence of Journal objects
    :return: A new JournalData, the result of concatenating the input data
    """
    return JournalData(
        journals[0].source, pd.concat([journal._data for journal in journals])
    )


@dataclass
class BasicJournalFile:
    """Defines basic properties of a single journal file"""
    server_root: str
    directory: str
    filename: str
    data_directory: str = "/I/AM/A/TEST/PATH"
    last_modified: dt.datetime = None
    first_run_number: int = -1
    last_run_number: int = -1

    @classmethod
    def from_derived(cls, derived):
        basic = cls(derived.server_root, derived.directory, derived.filename,
                    derived.data_directory, derived.last_modified,
                    derived.first_run_number, derived.last_run_number)
        return basic


@dataclass
class JournalFile(BasicJournalFile):
    """Defines a single journal file, including run data"""
    run_data: JournalData = None

    def get_data(self, run_number: int) -> Dict:
        """Return the data for the specified run number.
        
        :param run_number: Run number of interest
        :return: A Dict describing the run, or None if not found
        """
        return self.run_data.run(str(run_number))

    def __contains__(self, run_number: int):
        """Return whether the run_number exists in the journal file"""
        return (run_number >= self.first_run_number and
                run_number <= self.last_run_number)


@dataclass
class JournalCollection:
    """Defines a collection of journal files"""

    # The available journal files within a collection
    journalFiles: List(JournalFile)

    def __init__(self, journalFiles: List(JournalFile)) -> None:
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
        logging.debug(f"Run number {run_number} exists in journal {jf.filename}")

        # Get the data for the specified run number
        data = jf.get_data(run_number)

        # Return the full path
        return url_join(jf.data_directory, data["name"] + ".nxs")

    def locate_data_files(self, run_numbers: List[int]) -> Dict[int, str]:
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

    def get_info(self, filename: str):
        return next(
            (jf for jf in self.journalFiles if jf.filename == filename),
            None)

    def to_basic(self) -> List(BasicJournalFile):
        basic = []
        for x in self.journalFiles:
            basic.append(x.from_derived(x))
        return basic


@dataclass
class JournalLibrary:
    """Defines one or more data source rootURL/directory and their associated
    journal collections.
    """
    collections: Dict(str, JournalCollection)

    def __setitem__(self, key, value):
        self.collections[key] = value

    def __getitem__(self, key):
        if key in self.collections:
            return self.collections[key]
        else:
            return None

    def __contains__(self, key):
        return key in self.collections
