# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from dataclasses import dataclass


@dataclass
class JournalFile:
    """Defines a single journal file"""
    rootUrl: str
    directory: str
    filename: str

