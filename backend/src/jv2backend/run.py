# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Capture Run information"""
from __future__ import annotations
import dataclasses

from jv2backend.experiment import Experiment
from jv2backend.instrument import Instrument


@dataclasses.dataclass(init=False)
class Run:
    """Capture an experiment Run information"""

    # Define the attributes from the Run captured from the Journal
    name: str
    run_number: str
    experiment: Experiment
    instrument: Instrument

    def __init__(self, **kwargs) -> None:
        """Create an object from some or all of the given keywords. They must
        be one of those specified in the __slots__ field. Additional values
        are annoyed. Keys/Types are not checked. It is reccommended that factory
        methods such as from_dict are used.
        """
        missing = []
        for field in dataclasses.fields(self.__class__):
            name = field.name
            value = kwargs.get(name, None)
            if value is None:
                missing.append(name)
                continue
            # accept the value
            setattr(self, name, value)

        if missing:
            raise ValueError(
                f"Cannot create Run. Missing the following fields: " + ",".join(missing)
            )


#         for name, value in kwargs.items():
#             setattr(self, name, value)


# class RunBuilder:
#     """Logic to build a Run type and convert from strings to correct field types"""

#     def __init__(self):
#         self._fields: dict = {}

#     def add_attrs(self, **kwargs) -> None:
#         """Set attributes to pass to the created Run object.
#         This can be called many times to add further values before calling build
#         The types are converted if they do not match those expected by the Run
#         object

#         :param kwargs: Dict of attributes defining a Run
#         """
#         self._fields.update(kwargs)

#     def build(self) -> Run:
#         """Take preset field values, attempt to convert to expected types
#         and build the Run.

#         :raises ValueError: If fields are missing
#         :raises TypeError: If field types cannot be converted
#         :return: A new Run object
#         """
#         # build!
#         return Run(**run_fields)
