# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

"""A collection of function to return data for a NeXus file"""
from pathlib import Path, PurePath
import re
from typing import Any, MutableSequence, Sequence, Tuple

import h5py as h5
import numpy as np


class NXStrings:
    """Define known strings for NeXus files"""

    NX_class = b"NX_class"
    NXentry = b"NXentry"
    IXrunlog = b"IXrunlog"
    IXselog = b"IXselog"
    StartTime = "start_time"
    EndTime = "end_time"
    ValueLog = "value_log"
    DetectorPrefix = "detector_"
    MonitorPrefix = "monitor_"
    Data = "data"
    Counts = "counts"
    ToF = "time_of_flight"


# Match a monitor group name
_MonitorRE = re.compile(f"^{NXStrings.MonitorPrefix}\\d+$")


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


def get_detector_count(filepath: Path) -> int:
    """Return the number of spectra in detector_1

    :param filepath: A path to a NeXus file
    :return: The number of spectra
    """
    with h5.File(filepath) as h5file:
        return len(group_at(h5file, 0)[NXStrings.DetectorPrefix + "1"][NXStrings.Counts][0])  # type: ignore


def get_detector_spectrum(filepath: Path,
                          spectrum: int) -> Sequence[Tuple[float, float]]:
    """Return a single spectra of data from a file as a list of (tof,signal) pairs

    If the TOF values are bin edges then they are converted to bin centres.
    :param filepath: _description_
    :param spectrum: _description_
    :return: A list of (tof,signal) pairs as float64
    """
    with h5.File(filepath) as h5file:
        det1 = group_at(h5file, 0)[NXStrings.DetectorPrefix + "1"]
        return _tof_signal_points(
            det1[NXStrings.ToF], det1[NXStrings.Counts][0][spectrum]
        )


def get_monitor_count(filepath: Path) -> int:
    """Return the number of monitors in the first group

    :param filepath: A path to a NeXus file
    :return: The number of spectra
    """
    with h5.File(filepath) as h5file:
        first_group = group_at(h5file, 0)
        return len(
            [key for key in first_group.keys() if _MonitorRE.match(key) is not None]
        )


def get_monitor_spectrum(filepath: Path,
                         monitor: int) -> Sequence[Tuple[float, float]]:
    """Return a single monitor spectrum from a file as a list of (tof,signal)
    pairs

    If the TOF values are bin edges then they are converted to bin centres.
    :param filepath: Path to a HDF5 file
    :param monitor: The number of the monitor whose data should be returned
    :return: A list of (tof,signal) pairs as float64
    """
    with h5.File(filepath) as h5file:
        monitor_group = group_at(h5file, 0)[NXStrings.MonitorPrefix + str(monitor)]
        return _tof_signal_points(
            monitor_group[NXStrings.ToF], monitor_group[NXStrings.Data][0][0]
        )


def nonzero_spectra_ratio(filepath: Path) -> str:
    """Return the ratio of number of (non_zero spectra/spectra_count)

    :param filepath: A path to a NeXus file
    :return: The nonzero_spectra ratio
    """
    with h5.File(filepath) as h5file:
        counts = group_at(h5file, 0)[NXStrings.DetectorPrefix + "1"][NXStrings.Counts]  # type: ignore
        non_zero_count = np.count_nonzero(np.sum(counts[0], axis=1))  # type: ignore
        return f"{non_zero_count}/{len(counts[0])}"  # type: ignore


def open_at(filepath: Path, index: int) -> Tuple[h5.File, h5.Group]:
    """Return the group at the index given"""
    h5file = h5.File(filepath)
    return h5file, group_at(h5file, index)


def group_at(h5file: h5.File, index: int) -> h5.Group:
    """Return the group at the index given"""
    return list(h5file.values())[index]


# private helpers


def _tof_signal_points(
    tof_bins: h5.Dataset, counts: h5.Dataset
) -> Sequence[Tuple[float, float]]:
    """Take 2 datasets of binned TOF values and point count values
    and convert to a single list of (tof,signal) pairs where tof is the bin centre

    :param tof_bins: Bin edge values for ToF
    :param counts: Count values
    :return: A single list of (tof,signal) pairs where tof is the bin centre
    """
    tof_centres = 0.5 * (tof_bins[1:] + tof_bins[:-1])
    return [
        (tof.astype("float64"), count.astype("float64"))
        for tof, count in zip(tof_centres, counts)
    ]
