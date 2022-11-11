# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Capture Run information"""
from __future__ import annotations
from collections import Mapping
from dataclasses import dataclass, fields
from typing import Dict


@dataclass
class Run(Mapping):
    """Capture an experiment Run information"""

    # Define the attributes from the Run captured from the Journal
    name: str
    run_number: str
    experiment_identifier: str
    instrument_name: str

    def __init__(self, run_attrs: Dict[str, str]) -> None:
        """Create an object from some or all of the given keywords. They must
        be one of those specified in the dataclass fields. Additional values
        are ignored. Keys/Types are not checked. It is recommended that factory
        methods such as from_dict are used.
        """
        missing = [
            field.name
            for field in fields(self.__class__)
            if field.name not in run_attrs
        ]
        if missing:
            raise ValueError(
                f"Cannot create Run. Missing the following fields: " + ",".join(missing)
            )

        # accept values
        self._fields: Dict[str, str] = run_attrs

    def __iter__(self):
        return iter(self._fields)

    def __len__(self):
        return len(self._fields)

    def __getitem__(self, key: str) -> str:
        return self._fields[key]

    def __getattr__(self, key: str) -> str:
        """Each dictionary attribute can be access by a named property"""
        fields = self._fields
        if key in fields:
            return fields[key]
        else:
            return getattr(self, key)

    # def as_dict(self) -> Dict[str, str]:
    #     """Return a dictionary representation of this Run object"""
    #     return {}
