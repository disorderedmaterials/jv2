# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Capture Run information"""
import dataclasses

from jv2backend.experiment import Experiment
from jv2backend.instrument import Instrument


@dataclasses.dataclass(init=False)
class Run:
    """Capture an experiment Run information"""

    # Define the attributes from the Run captured from the Journal
    run_number: int
    experiment: Experiment
    instrument: Instrument
    # __slots__ = ("run_number", "experiment", "instrument")

    def __init__(self, **kwargs) -> None:
        """Create an object from some or all of the given keywords. They must
        be one of those specified in the __slots__ field. Additional values
        are annoyed.
        """
        missing, type_errors = [], []
        for field in dataclasses.fields(self.__class__):
            name = field.name
            value = kwargs.get(name, None)
            if value is None:
                missing.append(name)
                continue
            if not isinstance(value, field.type):
                type_errors.append(name)
            # accept the value
            setattr(self, name, value)

        if missing:
            raise ValueError(
                f"Cannot create Run. Missing the following fields: " + ",".join(missing)
            )

        if type_errors:
            raise TypeError(
                f"Cannot create Run. The following fields have unexpected types: "
                + ",".join(type_errors)
            )
