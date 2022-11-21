# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""A collection of function to return data for a NeXus file"""
from pathlib import Path, PurePath
from typing import Sequence
import h5py as h5


class NXStrings:
    """Define known strings for NeXus files"""

    NX_class = b"NX_class"
    NXentry = b"NXentry"
    IXrunlog = b"IXrunlog"
    IXselog = b"IXselog"


def logpaths(filepath: Path) -> Sequence[Sequence[str]]:
    """Return a list of paths to log data within the file. This
    implementation currently looks for IXselog & IXrunlog types.
    The paths are grouped by the name of the log. It is assumed the
    log entries are all children of the top-level NXEntry group.

    :param filepath: Path to an existing NeXus file
    :return: A list of the sample log paths found. This may be empty.
    :raises: IOError is the file cannot be accessed
    """
    with h5.File(filepath) as h5file:
        all_log_paths = []
        for group in h5file.values():
            if group.attrs[NXStrings.NX_class] != NXStrings.NXentry:
                continue
            for child in group.values():
                if not isinstance(child, h5.Group):
                    continue
                if child.attrs[NXStrings.NX_class] not in (
                    NXStrings.IXrunlog,
                    NXStrings.IXselog,
                ):
                    continue
                log_paths = [log.name for log in child.values()]
                log_paths.insert(0, PurePath(str(child.name)).name)
                all_log_paths.append(log_paths)

        return all_log_paths
