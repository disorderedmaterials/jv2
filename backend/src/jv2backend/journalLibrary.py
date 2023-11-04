# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
from dataclasses import dataclass
import typing
from jv2backend.journalCollection import JournalCollection
import logging

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
                if j.has_run_data():
                    logging.debug(f"     {j.get_file_url()} "
                                  f"({j.get_run_count()} run data)")
                else:
                    logging.debug(f"     {j.get_file_url()} (not yet loaded)")
