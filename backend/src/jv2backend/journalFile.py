# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
from dataclasses import dataclass
import typing
import datetime
from typing import Optional
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
