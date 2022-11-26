# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""A collection of function to return data for a NeXus file"""
from pathlib import Path, PurePath
from typing import Any, MutableSequence, Sequence, Tuple
import h5py as h5


class NXStrings:
    """Define known strings for NeXus files"""

    NX_class = b"NX_class"
    NXentry = b"NXentry"
    IXrunlog = b"IXrunlog"
    IXselog = b"IXselog"
    StartTime = "start_time"
    EndTime = "end_time"
    ValueLog = "value_log"


def logpaths_from_path(filepath: Path) -> Sequence[Sequence[str]]:
    """Return a list of paths to log data within the file at the given path

    See logpaths_from_file for full description
    """
    with h5.File(filepath) as h5file:
        return logpaths_from_file(h5file)


def logpaths_from_file(h5file: h5.File) -> Sequence[Sequence[str]]:
    """Return a list of paths to log data within the file. This
    implementation currently looks for IXselog & IXrunlog types.
    The paths are grouped by the name of the log. It is assumed the
    log entries are all children of the top-level NXEntry group.

    :param h5file: Path to an existing NeXus file
    :return: A list of the sample log paths found. This may be empty.
    :raises: IOError is the file cannot be accessed
    """

    def is_log_group(item) -> bool:
        if (not isinstance(item, h5.Group)) or child.name is None:
            return False

        return child.name.endswith("log") or child.attrs[NXStrings.NX_class] in (
            NXStrings.IXrunlog,
            NXStrings.IXselog,
        )

    all_log_paths = []
    for group in h5file.values():
        if group.attrs[NXStrings.NX_class] != NXStrings.NXentry:
            continue
        for child in group.values():
            if not is_log_group(child):
                continue
            log_paths = [log.name for log in child.values()]
            log_paths.insert(0, PurePath(str(child.name)).name)
            all_log_paths.append(log_paths)

    return all_log_paths


def logdata_from_path(filepath: Path, fields: Sequence[str]) -> Sequence[Sequence[Any]]:
    """Retrieve the requested log data from a given file path

    :param filepath: A filepath to a NeXus file
    :param fields: A list of paths to log data in the file
    :return: List in form [[(time,value)...]] for each field
    """
    h5file = h5.File(filepath)
    rundata = []
    for name in fields:
        rundata.append(h5file[name])

    return rundata


def timerange(h5group: h5.Group) -> Tuple[str, str]:
    """Return a tuple from the given Group as (start_time, end_time)"""

    def value_as_str(dataset):
        return dataset[0].decode("UTF-8")

    return value_as_str(h5group[NXStrings.StartTime]), value_as_str(
        h5group[NXStrings.EndTime]
    )


def logvalues(h5group: h5.Group) -> MutableSequence[Tuple[float, float]]:
    """Return the a list of (time, value) pairs of the given group

    :param h5group: An open HDF5 Group containing logged values.
                    Looks for a value or value_log dataset in the group
    """
    value_log = (
        h5group[NXStrings.ValueLog] if NXStrings.ValueLog in h5group else h5group
    )
    return [
        (float(time), float(value))
        for time, value in zip(value_log["time"], value_log["value"])  # type: ignore
    ]


def open_at(filepath: Path, index: int) -> Tuple[h5.File, h5.Group]:
    """Return the group at the index given"""
    h5file = h5.File(filepath)
    return h5file, group_at(h5file, index)


def group_at(h5file: h5.File, index: int) -> h5.Group:
    """Return the group at the index given"""
    return list(h5file.values())[index]
