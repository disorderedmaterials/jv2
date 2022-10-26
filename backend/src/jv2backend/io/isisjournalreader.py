# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
"""Defines a reader class to read an ISIS Journal file"""
from pathlib import PurePosixPath, PureWindowsPath
from lxml import etree
import re

from jv2backend.io.journalreader import JournalReader
from jv2backend.cycle import Cycle
from jv2backend.instrument import Instrument
from jv2backend.journal import Journal
from jv2backend.run import Run

# Regex to match a date-based journal filename
_JOURNAL_FILENAME_RE = re.compile(r"journal_(\d\d)_(\d+)")
_JOURNAL_YEAR_PREFIX = "20"

# Define known attribute keys
class _RootAttrib:
    FILENAME = "file_name"


class ISISXMLJournalReader(JournalReader):
    """A concrete type to read an XML-formatted Journal file"""

    def __init__(self, instrument: Instrument) -> None:
        """A Journal file is associated with an Instrument.
        This class reads the attributes into a Journal of runs

        :param instrument: The instrument associated with runs in this journal
        """
        super().__init__()
        self._instrument = instrument

    def read(self, content: bytes) -> Journal:
        """Read an XML-formatted iterable describing a Journal

        :param content: A bytes object containing XML describing the run content.
        :return: A Journal containing the parsed Run data
        """
        try:
            root = etree.fromstring(content)
        except SyntaxError as exc:
            raise SyntaxError(f"Unable to parse content as XML:\n\t{str(exc)}") from exc

        cycle = _parse_cycle(root.attrib[_RootAttrib.FILENAME])
        return self._create_journal(root, cycle)

    def _create_journal(self, root: etree.Element, cycle: Cycle) -> Journal:
        """
        :param root: Root node of the journal XML
        :param cycle: The cycle associated with this journal
        :return: A new Journal instance
        """
        journal = Journal(self._instrument, cycle)
        # run_builder = RunBuilder(self._instrument, cycle)
        for nxentry in root.getchildren():
            # run_builder.set().set().set().build() ???
            journal.add_run(_to_dict(nxentry))

        return journal


# Private functions


def _parse_cycle(filepath: str) -> Cycle:
    """Parse the cycle information from the file_name
    Uses only the filename portion of a path and assumes
    a format YY-N.xml where YY is a 2-digit year and N is the
    cycle number within that year

    :param filepath: file name or full path to journal file
    """
    # Select the Path implementation based on the content of the filename
    # and not the operating system we're runnning on
    pathcls = PureWindowsPath if "\\" in filepath else PurePosixPath
    filestem = pathcls(filepath).stem

    match = _JOURNAL_FILENAME_RE.match(filestem)
    if match is None:
        raise ValueError(
            "Cannot parse cycle information from 'file_name' attribute"
            f"Expecting format '{_JOURNAL_FILENAME_RE.pattern}' but found: {filepath}"
        )

    return Cycle(
        year=int(f"{_JOURNAL_YEAR_PREFIX}{match.group(1)}"),
        number=int(match.group(2)),
    )


def _to_dict(nxentry: etree.Element) -> dict:
    """Convert an NXEntry tag to a dictionary structure.
    In the process the namespace qualifiers on the tag names are removed.

    :param nxentry: An XML description of a run
    :return: A dictionary of key-value pairs from the entry
    """
    run_as_dict = dict()
    for elem in nxentry.getchildren():
        if not elem.text:
            text = "None"
        else:
            text = elem.text
        run_as_dict[_strip_namespace(elem.tag)] = text

    return run_as_dict


def _strip_namespace(qualified_name: str) -> str:
    """Remove any XML-namespace qualifier from the given
    name. The qualifiers are defined as schema path enclosed in braces '{}'
    e.g. {http://definition.nexusformat.org/schema/3.0}tag_name

    :param qualified_name: A name including qualification by the schema
    :return: The string stripped of the qualifier otherwise return the original string
    """
    end_brace_index = qualified_name.find("}")
    if qualified_name.startswith("{") and end_brace_index > 0:
        return qualified_name[end_brace_index + 1 :]
    else:
        return qualified_name
