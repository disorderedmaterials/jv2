# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
import typing
import datetime
from typing import Optional
from jv2backend.utils import url_join
from jv2backend.integerRange import IntegerRange
import xml.etree.ElementTree as ElementTree
import json


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


class Journal:
    """Defines a full journal, including run data"""

    def __init__(self, display_name: str = None,
                 journal_directory: str = None,
                 filename: str = None,
                 data_directory: str = None,
                 last_modified: datetime.datetime = None,
                 run_data: JournalData = None):
        self._display_name = display_name
        self._journal_directory = journal_directory
        self._filename = filename
        self._data_directory = data_directory
        self._last_modified = last_modified
        self._run_data = run_data

    @property
    def display_name(self) -> str:
        """Return the display name for the journal"""
        return self._display_name

    @property
    def journal_directory(self) -> str:
        """Return the directory location for the journal"""
        return self._journal_directory

    @property
    def filename(self) -> str:
        """Return the filename for the journal, without its path"""
        return self._filename

    def get_file_url(self) -> str:
        """Return the full URL to the journal"""
        return url_join(self._journal_directory, self._filename)

    @property
    def data_directory(self) -> str:
        """Return the general data directory for the journal file"""
        return self._data_directory

    @property
    def last_modified(self) -> datetime.datetime:
        """Return the last modification time for the journal"""
        return self._last_modified

    def to_basic_json(self) -> {}:
        return {
            "display_name": self.display_name,
            "journal_directory": self.journal_directory,
            "filename": self.filename,
            "data_directory": self.data_directory,
            "last_modified": str(self.last_modified)
        }

    def get_data(self, run_number: int) -> typing.Dict:
        """Return the data for the specified run number.

        :param run_number: Run number of interest
        :return: A Dict describing the run, or None if not found
        """
        return self._run_data.run(run_number)

    def __contains__(self, run_number: int) -> bool:
        """Return whether the run_number exists in the journal file"""
        return run_number in self._run_data

    def has_run_data(self):
        """Return whether any run data is defined"""
        return self._run_data is not None and self._run_data.run_count > 0

    @property
    def run_data(self):
        """Return the entire run data for the journal"""
        return self._run_data
