# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
import typing
import datetime
from typing import Optional
from io import BytesIO
from jv2backend.utils import url_join, lm_to_datetime
from jv2backend.integerRange import IntegerRange
import jv2backend.userCache
import xml.etree.ElementTree as ElementTree
from enum import Enum
import requests
import os.path
import json


class SourceType(Enum):
    """Source types"""
    Unknown = 0
    Network = 1
    Generated = 2
    File = 3


class Journal:
    """Defines a full journal, including run data"""

    def __init__(self, display_name: str = None,
                 source_type: SourceType = SourceType.Unknown,
                 parent_library_key: str = None,
                 journal_directory: str = None,
                 filename: str = None,
                 data_directory: str = None,
                 last_modified: datetime.datetime = None,
                 run_data: {} = None):
        self._display_name = display_name
        self._source_type = source_type
        self._parent_library_key = parent_library_key
        self._journal_directory = journal_directory
        self._filename = filename
        self._data_directory = data_directory
        self._last_modified = last_modified
        self._run_number_ranges: typing.List[IntegerRange] = []

        # Set the run data (also intialises ranges)
        self._run_data = run_data

    # ---------------- Basic Journal Information

    @property
    def display_name(self) -> str:
        """Return the display name for the journal"""
        return self._display_name

    @property
    def source_type(self) -> SourceType:
        """Return the source type for the journal"""
        return self._source_type

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

    @last_modified.setter
    def last_modified(self, value: datetime.datetime):
        """Set the last modification time"""
        self._last_modified = value

    # ---------------- File Operations

    def get_modification_time(self) -> datetime.datetime:
        """Get the modification time of the journal"""
        if self._source_type == SourceType.Network:
            response = requests.head(self.get_file_url(), timeout=3)
            response.raise_for_status()
            return lm_to_datetime(response.headers["Last-Modified"])
        elif self._source_type == SourceType.File:
            return datetime.datetime.fromtimestamp(
                os.path.getmtime(self.get_file_url()),
                datetime.timezone.utc
            )
        else:
            raise RuntimeError(f"Can't handle source type "
                               f"{str(self._source_type)}.")

    def get_run_data(self) -> None:
        """Get run data content and parse it with ElementTree"""
        # Check the cache for the data first
        if jv2backend.userCache.has_data(self._parent_library_key,
                                         self.filename):
            data, mtime = jv2backend.userCache.get_data(
                self._parent_library_key,
                self.filename
            )
            run_dict = json.loads(data)

            # Need to convert our run number keys from str -> int
            self.run_data = {int(run_no):run_dict[run_no]
                             for run_no in run_dict}
            self._last_modified = mtime
            return

        # Not present in the user cache, so try to obtain it
        if self._source_type == SourceType.Network:
            response = requests.get(self.get_file_url(), timeout=3)
            response.raise_for_status()
            tree = ElementTree.parse(BytesIO(response.content))
            self.set_run_data_from_element_tree(tree.getroot())
            self._last_modified = lm_to_datetime(
                response.headers["Last-Modified"]
            )
        elif self._source_type == SourceType.File:
            with open(self.get_file_url(), "rb") as file:
                tree = ElementTree.parse(BytesIO(file.read()))
            mtime = os.path.getmtime(self.get_file_url())
            self.set_run_data_from_element_tree(tree.getroot())
            self._last_modified = datetime.datetime.fromtimestamp(
                mtime, datetime.timezone.utc
            )
        else:
            raise RuntimeError(f"Can't handle source type "
                               f"{str(self._source_type)}.")

        # Write data to the cache
        jv2backend.userCache.put_data(self._parent_library_key, self.filename,
                                      json.dumps(self._run_data), self.last_modified)

    # ---------------- Run Data

    def __make_run_data_entry(self, run: ElementTree.Element,
                              namespace_prefix: str = None):
        """Create a dict for the specified run"""
        children = list(run)
        run_number_element = run.find("run_number" if namespace_prefix is None
                                      else namespace_prefix + "run_number")
        if run_number_element is None:
            raise RuntimeError("A run_number entry is missing in the data.")
        run_number = int(run_number_element.text)

        # Convert our XML tree to a dict for storage
        self._run_data[run_number] = {}
        for child in children:
            tag = (child.tag if namespace_prefix is None
                   else child.tag.removeprefix(namespace_prefix))
            self._run_data[run_number][tag] = str(child.text).strip()

    def __create_run_ranges(self):
        """Create run ranges from the current data"""
        self._run_number_ranges = []

        # If the run data is None we are done
        if self._run_data is None:
            return

        for run_number in self._run_data:
            rnr = next((r for r in self._run_number_ranges
                        if r.extend(run_number)), None)
            if not rnr:
                self._run_number_ranges.append(
                    IntegerRange(first=run_number, last=run_number)
                )

        # Sort the ranges so that the highest run number appears in the last one
        self._run_number_ranges.sort(key=lambda r: r.last)

    def set_run_data_from_element_tree(self, treeRoot: ElementTree.Element):
        """Create run data from the supplied ElementTree data.

        We assume that "interesting" entries are those keyed 'NXentry', but we
        need to account for namespaces added into element tags by the
        ElementTree parser (an issue with ISIS journal files but not our own
        generated ones since we don't use namespacing).
        """
        self._run_data = {}

        for prefix in [None, "{http://definition.nexusformat.org/schema/3.0}"]:
            for run in treeRoot.findall("NXentry" if prefix is None
                                        else prefix + "NXentry"):
                self.__make_run_data_entry(run, prefix)

        self.__create_run_ranges()

    def __contains__(self, run_number: int) -> bool:
        """Return whether the run_number exists in the journal file"""
        rnr = next((r for r in self._run_number_ranges
                    if run_number in r), None)
        return rnr is not None

    def has_run_data(self):
        """Return whether any run data are defined"""
        return self._run_data is not None and len(self._run_data) > 0

    @property
    def run_data(self):
        """Return the entire run data for the journal"""
        return self._run_data

    @run_data.setter
    def run_data(self, run_data: {}):
        """Set run data from the supplied dictionary."""
        self._run_data = run_data
        self.__create_run_ranges()

    def get_run_count(self) -> int:
        """Return the number of runs listed within this Journal"""
        return len(self._run_data)

    def get_last_run_number(self) -> Optional[int]:
        """Return the run number of the last run in the journal"""
        if len(self._run_number_ranges) == 0:
            return None
        else:
            return self._run_number_ranges[-1].last

    def get_run(self, run_number: int) -> typing.Dict:
        """Return the data for the specified run number.

        :param run_number: Run number of interest
        :return: A Dict describing the run, or None if not found
        """
        if run_number in self._run_data:
            return self._run_data[run_number]
        else:
            return None

    def get_run_data_after(self, run_number: int) -> {}:
        """Return data for all run numbers after the supplied run number
        (i.e. with higher numbers)
        """
        return {run_no: data for run_no, data in self._run_data.items()
                if run_no > run_number}

    # ---------------- Conversion

    def get_journal_as_dict(self) -> {}:
        """Returns the basic journal data in a dict"""
        return {
            "display_name": self.display_name,
            "journal_directory": self.journal_directory,
            "filename": self.filename,
            "data_directory": self.data_directory,
            "last_modified": str(self.last_modified)
        }

    @classmethod
    def convert_run_data_to_json_array(cls, runs: {}) -> str:
        """Convert the given run data to JSON. The input dict is assumed to
        map run numbers to Dicts of fields/values
        """
        items = []
        for key in runs:
            items.append(runs[key])
        return json.dumps(items)

    def get_run_data_as_json_array(self) -> str:
        return self.convert_run_data_to_json_array(self._run_data)
