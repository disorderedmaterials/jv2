# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2024 Team JournalViewer and contributors

from __future__ import annotations
import typing
import datetime
from io import BytesIO
from jv2backend.utils import url_join, lm_to_datetime
import jv2backend.main.selector
from jv2backend.classes.journal import Journal, SourceType
import jv2backend.main.userCache
import xml.etree.ElementTree as ElementTree
import logging
import json
import requests
import functools
import lxml.etree as etree
from threading import Thread, Event, Lock


_ACQUISITION_THREAD = None
_STOP_ACQUISITION_EVENT = Event()
_ACQUISITION_THREAD_JOURNAL_MUTEX = Lock()
_ACQUISITION_THREAD_COMPLETE_MUTEX = Lock()
_ACQUISITION_THREAD_NUM_COMPLETED_MUTEX = Lock()
_ACQUISITION_THREAD_LAST_FILENAME_MUTEX = Lock()


class JournalCollection:
    """Defines a collection of journal files relating to a specific instrument,
    directory etc.
    """

    def __init__(self, source_type: SourceType = SourceType.Unknown,
                 library_key: str = None,
                 index_root_url: str = None,
                 index_filename: str = None,
                 run_data_url: str = None,
                 last_modified: datetime.datetime = None,
                 journals: [] = None):
        self._source_type = source_type
        self._library_key = library_key
        self._index_root_url = index_root_url
        self._index_filename = index_filename
        self._run_data_url = run_data_url
        self._last_modified = last_modified
        self._journals = [] if journals is None else journals

    def __getitem__(self, filename: str):
        return next(
            (j for j in self._journals if j.filename == filename),
            None)

    def __contains__(self, key):
        j = self.__getitem__(key)
        return j is not None

    @property
    def library_key(self) -> str:
        """Return the library key for the collection"""
        return self._library_key

    def get_index_file_url(self) -> str:
        """Return the full URL to the journal"""
        return url_join(self._index_root_url, self._index_filename)

    @property
    def run_data_url(self) -> str:
        """Return the run data location"""
        return self._run_data_url

    def add_journal(self, display_name: str, journal_filename: str,
                    data_directory: str,
                    run_data: {} = None) -> Journal:
        """Add a new journal to the list"""
        journal = Journal(
            display_name,
            self._source_type,
            self._library_key,
            self._index_root_url,
            journal_filename,
            data_directory,
            datetime.datetime.now(),
            {} if run_data is None else run_data
        )

        self._journals.append(journal)
        return journal

    @property
    def journals(self):
        """Return the current journals list"""
        return self._journals

    def get_journal_count(self):
        """Return the number of journals in the collection"""
        return 0 if self._journals is None else len(self._journals)

    # ---------------- Data Handling

    def _update_journals(self, data: {}):
        """Update journal information from supplied dict"""
        for j in data:
            logging.debug(f"Processing index entry {j['filename']}...")

            # If this journal already exists we just move on
            if j["filename"] in self:
                logging.debug("Skipping as already exists.")
                continue

            # Create a new journal entry
            self._journals.append(Journal(
                j["display_name"],
                self._source_type,
                self._library_key,
                self._index_root_url,
                j["filename"],
                j["data_directory"]
            ))

    def is_up_to_date(self) -> bool:
        """Get the modification time of the index from its root source and
        compare this to the stored value, returning whether the current data
        is up-to-date.
        """
        if self._last_modified is None:
            return False
        elif self._source_type == SourceType.Network:
            # Try to retrieve from source
            response = requests.head(self.get_index_file_url(), timeout=3)
            response.raise_for_status()
            return lm_to_datetime(
                response.headers["Last-Modified"]
            ) == self._last_modified
        elif self._source_type == SourceType.Generated:
            return True

        return False

    def get_index(self) -> None:
        """Retrieve index file information

        The index xml file contains a simple list of journal files in the
        same directory:

        <journal>
           <file name="journal.xml"/>
           <file name="journal_YY_N.xml"/>
           ...
           <file name="hash1.xml" display_name="My Data"
                 data_directory="/data"/>           # JV2-generated entry
        </journal>

        The first entry (journal.xml) is not relevant and should not be
        returned as a valid result. Other files represent journals
        corresponding to directory locations or, in the case of the ISIS
        standard journals, specific years (YY) and cycle integers (N).
        These journal files are expected to reside in the same directory as
        the index file.

        -- Display Name --

        The display name of the journal is given in the 'display_name'
        attribute. If not specified, a display name is generated from the
        journal filename, assuming that it is ISIS standard.

        -- Run Data Location --

        The location on disk of the associated run data for the journal is
        given in the 'data_directory' attribute. If not present, it is
        assumed that the location follows the ISIS Archive format, e.g.:

        /data_directory/NDXINSTRUMENT/Instrument/data/cycle_YY_M

        The 'instrument' in this case is expected to be the `directory`
        parameter passed in the request_data object.
        """
        # Check the cache for the data first
        if jv2backend.main.userCache.has_data(self._library_key,
                                               self._index_filename):
            data, mtime = jv2backend.main.userCache.get_data(
                self._library_key,
                self._index_filename
            )
            self._update_journals(json.loads(data))
            self._last_modified = mtime
        elif self._source_type == SourceType.Network:
            response = requests.get(url_join(self._index_root_url, self._index_filename), timeout=3)
            response.raise_for_status()
            index_tree = ElementTree.parse(BytesIO(response.content))
            index_root = index_tree.getroot()
            self._last_modified = lm_to_datetime(
                response.headers["Last-Modified"]
            )

            # Construct a dict of journals from the XML
            data = []
            for j in index_root.iter("file"):
                # Skip the "journal.xml" or malformed entries
                if "name" not in j.attrib or j.attrib["name"] == "journal.xml":
                    continue

                # Determine display name
                if "display_name" in j.attrib:
                    display_name = j.attrib["display_name"]
                else:
                    display_name = j.attrib["name"].replace("journal", "Cycle")
                    display_name = display_name.replace(".xml", "").replace("_", " ")

                # Determine data directory
                if "data_directory" in j.attrib:
                    data_directory = j.attrib["data_directory"]
                else:
                    data_directory = url_join(
                        self._run_data_url,
                        "Instrument",
                        "data",
                        j.attrib["name"].replace(
                            "journal", "cycle"
                        ).replace(".xml", ""))

                # Push a new journal definition
                data.append({
                    "display_name": display_name,
                    "data_directory": data_directory,
                    "filename": j.attrib["name"]
                })

            # Update the current journals with the new data
            self._update_journals(data)
        elif self._source_type == SourceType.Generated:
            # Respond that the index file is not found by raising a FileNotFoundError
            raise FileNotFoundError
        else:
            raise RuntimeError("Don't know how to get data for source.")

    def get_journal_data(self, journal_filename: str) -> str:
        """Retrieve run data contained in a journal file

        :param journal_filename: Name of the journal to retrieve
        :return: JSON array of run data information
        """
        # Search the collection for the specified journal file
        j = self[journal_filename]
        if j is None:
            return json.dumps(
                {"Error": f"Journal {journal_filename} not in collection."}
            )

        # If we already have run data for the journal, check its modtime and
        # return the existing data if it is up-to-date
        if j.run_data is not None:
            if j.is_up_to_date():
                logging.debug(f"Returning current data for journal "
                              f"{j.filename} as it is up-to-date.")
                return j.get_run_data_as_json_array()

        # Not up-to-date, or not present, so get the full file content
        try:
            j.get_run_data()
        except (requests.HTTPError, requests.ConnectionError,
                FileNotFoundError) as exc:
            return json.dumps({"Error": str(exc)})
        except etree.XMLSyntaxError as exc:
            return json.dumps({"Error": str(exc)})

        return j.get_run_data_as_json_array()

    def get_updates(self, journal_filename: str) -> str:
        """Check if the journal index files has been modified since the last
        retrieval and return new runs added after the last known.

        :param journal_filename: Target journal to probe for updates
        :return: JSON array of new run data information or None
        """
        # Search the collection for the specified journal file
        j = self[journal_filename]
        if j is None:
            return json.dumps(
                {"Error": f"Journal {journal_filename} not in collection."}
            )

        # If we already have this journal file in the collection, check its
        # modification time, returning the current data if up-to-date
        logging.debug(f"get_updates: Checking mtime for {j.display_name}....")
        if j.is_up_to_date():
            logging.debug("get_updates: ...up-to-date so returning None")
            return json.dumps(None)

        # Changed, so read full data and store the whole thing, storing the
        # current last run number before we set the new data
        old_last_run_number = j.get_last_run_number()
        logging.debug(
            f"get_updates: Last run number known is {old_last_run_number}"
            )
        try:
            j.get_run_data(ignore_cache=True)
        except (requests.HTTPError, requests.ConnectionError,
                FileNotFoundError) as exc:
            return json.dumps({"Error": str(exc)})

        # If our old last run number is None then we had no data so return all
        if old_last_run_number is None:
            logging.debug(f"get_updates: ...returning all available data.")
            return j.get_run_data_as_json_array()

        # If the old run numbers are the same, nothing to update
        if old_last_run_number == j.get_last_run_number():
            logging.debug(f"get_updates: ...no new data, returning None.")
            return json.dumps(None)

        # Return any new runs after the previous last known run number
        return Journal.convert_run_data_to_json_array(
            j.get_run_data_after(old_last_run_number)
        )

    def get_uncached_journal_count(self) -> int:
        """Get the number of journal files currently uncached and requiring
        retrieval.

        :return: The number of uncached journals for this collection
        """
        # If the source is not a Network type then we already have everything
        if self._source_type is not SourceType.Network:
            return 0

        # Network type so check the cache for each
        return functools.reduce(
            lambda x, y:
            x + (1 if not jv2backend.main.userCache.has_data(
                    self._library_key,
                    y.filename
                    )
                 else 0),
            self._journals, 0
        )

    # ---------------- File Location

    def filename_for_run(self, instrument: str, run: str) -> typing.Optional[str]:
        """Find the journal file that contains the given run

        :param instrument: The instrument name
        :param run: Run number
        :return: Filename str or None if the run cannot be found
        """
        # We do not use the search method as it is more likely a
        # user will request a recent run and we want to break when this is
        # found
        filename = None
        for filename in reversed(self.journal_filenames(instrument)):
            journal = self.journal(instrument, filename=filename).run(run)
            if journal is not None:
                filename = filename
                break

        return filename

    def journal_for_run(self, run_number: int) -> Journal:
        """Find the journal in the collection that contains the specified run
        number.

        :param run_number: Run number to locate
        :return: Journal containing the run number, or None if not found
        """
        # For network sources we can (most likely) assume that the journals
        # are organised chronologically with sequential run numbers. However,
        # for generated journals there is no such guarantee. However, in the
        # latter case load times a very quick since we have all information in
        # the user cache, so just try to load everything.
        if self._source_type is not SourceType.Network:
            for jf in self._journals:
                jf.get_run_data()
                if run_number in jf:
                    return jf
            return None

        # Try a quick scan over loaded journals which have run number info
        journal = next(
            (jf for jf in self._journals if run_number in jf),
            None)
        if journal is not None:
            return journal

        # Not in an existing loaded journal, so determine list index limits
        # for where the requested run is
        left = 0
        last_index = len(self._journals) - 1
        right = last_index

        while left != right:
            # Get our best limits
            for n in range(0, last_index + 1):
                # Leftmost limit
                if self._journals[n].has_run_data():
                    logging.debug(f"Journal at {n} has runs from {self._journals[n].get_first_run_number()} to {self._journals[n].get_last_run_number()}")
                    if self._journals[n].get_first_run_number() <= run_number:
                        left = n

                # Rightmost limit (working backwards)
                if self._journals[-(n+1)].has_run_data():
                    logging.debug(f"and journal at {last_index-n} has runs from {self._journals[-(n+1)].get_first_run_number()} to {self._journals[-(n+1)].get_last_run_number()}")
                    if self._journals[-(n+1)].get_last_run_number() >= run_number:
                        right = last_index-n

            # Journal located?
            if left == right:
                return self._journals[left]

            # Edge cases - if left == right == 0 then the run number requested
            # is before the earliest journal available. Similarly, if both
            # equal the last journal index, it is after our most recent data
            if (left == right == 0) or (left == right == last_index):
                return None

            # Retrieve run data for the journal at the centre of the specified range
            self._journals[left + int((right-left)/2)].get_run_data()

    def locate_data_file(self, run_number: int) -> str:
        """Return the full path to the data (NeXuS) file for the specified
        run number

        :param run_number: Run number to locate the data file for
        :return: Full path to the data file or None if it couldn't be found
        """
        # Get the journal file for the specified run number
        jf = self.journal_for_run(run_number)
        if jf is None:
            return None
        logging.debug(f"Run number {run_number} exists in journal "
                      f"{jf.filename}")

        # Get the data for the specified run number
        data = jf.get_run(run_number)

        # The journal entry may contain the full data_directory and filename
        # information if we generated it. Otherwise we have to assume the
        # stored 'data_directory' and use the 'name' attribute.
        if "data_directory" in data and "filename" in data:
            return url_join(data["data_directory"], data["filename"])
        else:
            return url_join(jf.data_directory, data["name"] + ".nxs")

    def locate_data_files(
            self, run_numbers: typing.List[int]
    ) -> typing.Dict[int, str]:
        """Return a dict of run number/paths to NeXuS data files
        If a run number is not locatable, return None for that entry

        :param run_numbers: A list of integer run numbers to locate
        :return: A Dict[int,str] mapping of integer run number to either
                 to eithr NeXuS file path or None if not locatable
        """
        # For each of the supplied run numbers, find its parent journal
        result = {}
        for i in run_numbers:
            result[i] = self.locate_data_file(i)
        return result

    # ---------------- Search

    def search(self, search_terms: {}) -> {}:
        """
        Search across all journals in the collection, selecting those which
        match _all_ of the specified search_terms. The search terms are
        applied sequentially in the order they appear in the dict.

        :param search_terms: Dict of search field/values
        :return: A dict of runs matching the search query.
        """
        results = {}

        # See if we have a case-sensitive flag
        case_sensitive = ("caseSensitive" in search_terms and
                          search_terms["caseSensitive"] == "true")
        if "caseSensitive" in search_terms:
            del search_terms["caseSensitive"]

        for jf in self._journals:
            logging.debug(f"Journal {jf.filename} .....")
            if not jf.has_run_data:
                continue
            matches = None
            # Cycle over search terms
            # If the current 'matches' is None then search the whole run data
            # If it is not, then search it instead (chaining searches)
            # If it is ever a size of zero we have excluded all runs
            logging.debug("Starting loop over run data...")
            for field in search_terms:
                matches = jv2backend.main.selector.select(jf.run_data if matches is None
                                          else matches,
                                          field,
                                          search_terms[field],
                                          case_sensitive)
                logging.debug(f"...after checking '{field}' there are "
                              f"{len(matches)} matches...")
                if len(matches) == 0:
                    break

            if matches is None:
                continue

            logging.debug(f"Journal {jf.filename} matched {len(matches)} runs.")
            results.update(matches)

        return results

    # ---------------- Conversion

    def to_basic_json(self) -> str:
        """Return basic journal information as formatted JSON"""
        basic = []
        for journal in self._journals:
            basic.append(journal.get_journal_as_dict())
        return json.dumps(basic)


# Threading Class to Acquire all Journal RunData in a
class AcquisitionThread(Thread):
    def __init__(self, collection: JournalCollection):
        Thread.__init__(self)
        self._collection = collection
        self._num_completed = 0
        self._complete = False

    def run(self):
        global _ACQUISITION_THREAD_JOURNAL_MUTEX, _STOP_ACQUISITION_EVENT
        global _ACQUISITION_THREAD_NUM_COMPLETED_MUTEX
        global _ACQUISITION_THREAD_COMPLETE_MUTEX
        global _ACQUISITION_THREAD_LAST_FILENAME_MUTEX

        error = None

        for j in self._collection.journals:
            if _STOP_ACQUISITION_EVENT.is_set():
                break

            with _ACQUISITION_THREAD_NUM_COMPLETED_MUTEX:
                self._num_completed = self._num_completed + 1

            if j.has_run_data():
                logging.debug(f"Skipping {j.filename} as data are present...")
                continue

            logging.debug(f"Acquiring run data for {j.filename}...")
            with _ACQUISITION_THREAD_LAST_FILENAME_MUTEX:
                self._last_filename = j.filename

            with _ACQUISITION_THREAD_JOURNAL_MUTEX:
                try:
                    j.get_run_data()
                except (requests.HTTPError, requests.ConnectionError,
                        FileNotFoundError) as exc:
                    error = str(exc)
                    break
                except etree.XMLSyntaxError as exc:
                    error = str(exc)
                    break

        with _ACQUISITION_THREAD_COMPLETE_MUTEX:
            self._complete = True

        return json.dumps("OK" if error is None else {"Error": error})

    def get_update(self) -> ():
        """Return an update on the acquisition"""
        global _ACQUISITION_THREAD_JOURNAL_MUTEX, _ACQUISITION_THREAD_COMPLETE_MUTEX

        with _ACQUISITION_THREAD_NUM_COMPLETED_MUTEX:
            n = self._num_completed

        with _ACQUISITION_THREAD_COMPLETE_MUTEX:
            complete = self._complete

        with _ACQUISITION_THREAD_LAST_FILENAME_MUTEX:
            last_filename = self._last_filename

        return json.dumps(
            {
                "num_completed": n,
                "last_filename": last_filename,
                "complete": complete
            }
        )


class JournalAcquirer:
    """Journal file acquirer"""

    def acquire_all_data(self, collection: JournalCollection) -> str:
        """Retrieve all run data for all journals listed in the collection
        """
        # Loop over defined journal files. If run_data is already present we
        # assume it's up-to-date.
        global _ACQUISITION_THREAD, _STOP_ACQUISITION_EVENT
        _ACQUISITION_THREAD = AcquisitionThread(collection)
        _STOP_ACQUISITION_EVENT.clear()
        _ACQUISITION_THREAD.start()
        logging.debug("Started acquisition thread...")
        return json.dumps(None)

    def get_acquisition_update(self) -> str:
        """Get an update on the current scan"""
        global _ACQUISITION_THREAD
        if _ACQUISITION_THREAD is not None:
            return _ACQUISITION_THREAD.get_update()
        return json.dumps("NOT_RUNNING")

    def stop_acquisition(self) -> str:
        """Stop any scan currently in progress"""
        global _ACQUISITION_THREAD
        if _ACQUISITION_THREAD is None:
            return json.dumps("NOT RUNNING")
        if _ACQUISITION_THREAD.is_alive():
            _STOP_ACQUISITION_EVENT.set()
            _ACQUISITION_THREAD.join()

        _ACQUISITION_THREAD = None

        return json.dumps("OK")
