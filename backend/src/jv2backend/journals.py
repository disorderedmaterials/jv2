# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
from dataclasses import dataclass, KW_ONLY
import typing
import datetime
from typing import Optional, Sequence
from jv2backend.utils import url_join
from jv2backend.integerRange import IntegerRange
import jv2backend.select as Selector
import xml.etree.ElementTree as ElementTree
import json
import logging

# Format of start time search string
USERINPUT_DT_FORMAT_STR = "%Y/%m/%d"
UNIX_DT_FORMAT_STR = "%Y-%m-%dT%H:%M:%S"

def _to_datetime(user_input: str, input_format: str) -> datetime.datetime:
    """Convert from a string of specified format to a datetime object"""
    return datetime.datetime.strptime(user_input, input_format)

# Query handlers
# A handler should have the form Callable[[pd.DataFrame, str, bool],
# pd.DataFrame]
def _query_string_contains(
    data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field contains the text
    string provided.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A dict with matching runs
    """
    results = {}
    search_value = value if case_sensitive else value.lower()
    for run in data:
        if field not in data[run]:
            continue
        text = data[run][field] if case_sensitive else data[run][field].lower()
        if search_value in text:
            results[run] = data[run]
    return results

def _query_string_equals(
        data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field exactly matches the
    string provided.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Text to search
    :param case_sensitive: If True the case of the data must match the query
    :return: A dict with matching runs
    """
    results = {}
    search_value = value if case_sensitive else value.lower()
    for run in data:
        if field not in data[run]:
            continue
        text = data[run][field] if case_sensitive else data[run][field].lower()
        if search_value == text:
            results[run] = data[run]
    return results

def _query_integer_in_range(
        data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field, when converted to an
    int, falls within the range specified in the value parameter. It is
    assumed that the field can be reliably converted to an int, and the search
    is inclusive.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Integer range, which can be of the following construction:
                   N-M - A range of N to M inclusive
                   <N  - Any number less than N
                   >N  - Any number greater than N
    :param case_sensitive: <Unused, required by API>
    :return: A dict with matching runs
    """
    results = {}

    if "-" in value:
        # Range search
        try:
            irange = IntegerRange.from_string(value)
        except ValueError:
            return {}

        for run in data:
            if field in data[run] and int(data[run][field]) in irange:
                results[run] = data[run]

    elif value.startswith("<"):
        # Less than
        ivalue = int(value.lstrip("<>"))
        for run in data:
            if field in data[run] and int(data[run][field]) < ivalue:
                results[run] = data[run]

    elif value.startswith(">"):
        # Greater than
        ivalue = int(value.lstrip("<>"))
        for run in data:
            if field in data[run] and int(data[run][field]) > ivalue:
                results[run] = data[run]

    else:
        # Equal to
        ivalue = int(value)
        for run in data:
            if field in data[run] and int(data[run][field]) == ivalue:
                results[run] = data[run]

    return results


def _query_datetime_in_range(
        data: {}, field: str, value: str, case_sensitive: bool
) -> {}:
    """Return a dict of run data whose specified field, when converted to an
    datetime, falls within the range specified in the value parameter. It is
    assumed that the field can be reliably converted to a datetime, and the
    search is inclusive.

    :param data: A dict of input run data
    :param field: The name of the field that should be matched
    :param value: Datetime range, which can be of the following construction:
                   N-M - A range of N to M inclusive
                   <N  - Any datetime before N
                   >N  - Any datetime after N
    :param case_sensitive: <Unused, required by API>
    :return: A dict with matching runs
    """
    results = {}

    if "-" in value:
        # Range search
        try:
            start, end = value.split("-")
            start = _to_datetime(start.strip(), USERINPUT_DT_FORMAT_STR)
            end = _to_datetime(end.strip(), USERINPUT_DT_FORMAT_STR)
        except ValueError:
            return {}

        for run in data:
            if (field in data[run] and
                start <= _to_datetime(data[run][field], UNIX_DT_FORMAT_STR)
                <= end):
                results[run] = data[run]

    elif value.startswith("<"):
        # Less than
        mark = _to_datetime(value.lstrip("<>"), USERINPUT_DT_FORMAT_STR)
        for run in data:
            if (field in data[run] and
                _to_datetime(data[run][field], UNIX_DT_FORMAT_STR) < mark):
                results[run] = data[run]

    elif value.startswith(">"):
        # Greater than
        mark = _to_datetime(value.lstrip("<>"), USERINPUT_DT_FORMAT_STR)
        for run in data:
            if (field in data[run] and
                    _to_datetime(data[run][field], UNIX_DT_FORMAT_STR) > mark):
                results[run] = data[run]

    else:
        raise RuntimeError("Can't perform a literal time comparison.")

    return results

# Map a field name to a handler for that query if it should have special
# handling
_SPECIAL_QUERY_HANDLERS = {
    "experiment_identifier": _query_string_equals,
    "run_number": _query_integer_in_range,
    "start_time": _query_datetime_in_range,
}


# JournalData class
class JournalData:
    """JournalData captures all run information from a given journal file"""

    def __init__(self, data: {}) -> None:
        self._data = data
        self._run_number_ranges: typing.List[IntegerRange] = []
        self.__create_run_ranges()

    @classmethod
    def from_element_tree(cls, treeRoot: ElementTree.Element):
        """Create an instance from the supplied ElementTree data.
        We assume that "interesting" entries are those keyed 'NXentry', but we
        need to account for namespaces added into element tags by the
        ElementTree parser (an issue with ISIS journal files but not our own
        generated ones since we don't use namespacing).
        """
        data = {}

        for prefix in [None, "{http://definition.nexusformat.org/schema/3.0}"]:
            for run in treeRoot.findall("NXentry" if prefix is None
                                        else prefix + "NXentry"):
                cls.__make_run_data_entry(run, data, prefix)

        return cls(data)

    @classmethod
    def __make_run_data_entry(cls, run: ElementTree.Element, target_data: {},
                              namespace_prefix: str = None):
        """Create a dict for the specified run"""
        children = list(run)
        run_number_element = run.find("run_number" if namespace_prefix is None
                                      else namespace_prefix + "run_number")
        if run_number_element is None:
            raise RuntimeError("A run_number entry is missing in the data.")
        run_number = int(run_number_element.text)

        # Convert our XML tree to a dict for storage
        target_data[run_number] = {}
        for child in children:
            tag = (child.tag if namespace_prefix is None
                   else child.tag.removeprefix(namespace_prefix))
            target_data[run_number][tag] = str(child.text).strip()

    def __create_run_ranges(self):
        """Create run ranges from the current data"""
        self._ranges = []
        for run_number in self._data:
            rnr = next((r for r in self._run_number_ranges
                        if r.extend(run_number)), None)
            if not rnr:
                self._run_number_ranges.append(
                    IntegerRange(first=run_number, last=run_number)
                )

        # Sort the ranges so that the highest run number appears in the last one
        self._run_number_ranges.sort(key=lambda range: range.last)

    def __contains__(self, run_number: int) -> bool:
        rnr = next((r for r in self._run_number_ranges
                    if run_number in r), None)
        return rnr is not None

    @property
    def data(self) -> {}:
        """Return the contained run data dictionary"""
        return self._data

    @property
    def run_count(self) -> int:
        """Return the number of runs listed within this Journal"""
        return len(self._data)

    @property
    def get_last_run_number(self) -> Optional[int]:
        """Return the run number of the last run in the journal"""
        if len(self._run_number_ranges) == 0:
            return None
        else:
            return self._run_number_ranges[-1].last

    def run(self, run_number: int) -> Optional[dict]:
        """Return a dictionary describing the given run number

        :param run_number: Run number to select
        :return: A dict describing the Run or None if the run does not exist
        """
        if run_number in self._data:
            return self._data[run_number]
        else:
            return None

    # Output formats
    def to_json(self) -> str:
        items = []
        for key in self._data:
            items.append(self._data[key])
        return json.dumps(items)


# Operations on multiple journal data
def concatenate(journals: Sequence[JournalData]) -> JournalData:
    """Concatenate the Journals to a single Journal

    :param journals: Sequence of Journal objects
    :return: A new JournalData, the result of concatenating the input data
    """
    return JournalData(pd.concat([journal._data for journal in journals]))


@dataclass
class BasicJournalFile:
    """Defines basic properties of a single journal file"""
    display_name: str
    server_root: str
    directory: str
    filename: str
    data_directory: str
    last_modified: datetime.datetime = None

    @classmethod
    def from_derived(cls, derived):
        basic = cls(derived.display_name, derived.server_root,
                    derived.directory, derived.filename,
                    derived.data_directory, derived.last_modified)
        return basic


@dataclass
class JournalFile(BasicJournalFile):
    """Defines a single journal file, including run data"""
    run_data: JournalData = None

    def get_data(self, run_number: int) -> typing.Dict:
        """Return the data for the specified run number.

        :param run_number: Run number of interest
        :return: A Dict describing the run, or None if not found
        """
        return self.run_data.run(run_number)

    def __contains__(self, run_number: int):
        """Return whether the run_number exists in the journal file"""
        return run_number in self.run_data


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
