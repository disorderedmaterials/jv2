# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
import jv2backend.journals
from jv2backend.utils import url_join
from enum import Enum

class InvalidRequest(Exception):

    def __init__(self, message):
        Exception.__init__(self)
        self._message = message

    def __str__(self):
        return self._message


class SourceType(Enum):
    """Source types"""
    Unknown = 0
    Network = 1
    Cached = 2
    File = 3


class RequestData:
    """Simple class for checking / handling POST data supplied to a Flask
    route"""

    def __init__(self, requestData: typing.Any,
                 library: jv2backend.journals.JournalLibrary,
                 require_journal_file=False,
                 require_in_library=False,
                 require_data_directory=False,
                 require_run_numbers=False,
                 require_parameter=None) -> None:
        """Store recognised items in the POST data. We can make various
         stipulations on the contents:

          require_journal_file: Whether a full journal index/file must be
                                given
            require_in_library: Whether a collection matching the library_key
                                must already exist
        require_data_directory: Whether a 'dataDirectory' must be provided
              require_filename: Whether a 'filename' must be provided
           require_run_numbers: Whether one or more run numbers are expected
             require_parameter: Name of an additional parameter to expect
        """
        self._source_id: str = None
        self._source_type: SourceType = SourceType.Unknown
        self._journal_root_url: str = None
        self._journal_filename: str = None
        self._directory: str = None
        # OLD
        self._data_directory: str = None
        self._parameter: str = None
        self._journal_collection: jv2backend.journals.JournalCollection = None
        self._run_numbers: typing.List[int] = []

        # Source ID / type always required - ID in conjunction with the
        # optional 'directory' makes up our unique library key for the
        # collection
        if "sourceID" not in requestData:
            raise InvalidRequest("No source ID provided in request.")
        self._source_id = requestData["sourceID"]
        if "sourceType" not in requestData:
            raise InvalidRequest("No source type provided in request.")
        self._source_type = SourceType[requestData["sourceType"]]

        # Full journal location required?
        if require_journal_file:
            if "journalRootUrl" not in requestData:
                raise InvalidRequest("No journal root URL provided in "
                                     "request.")
            self._journal_root_url = requestData["journalRootUrl"]
            if "journalFilename" not in requestData:
                raise InvalidRequest("No journal filename provided in "
                                     "request.")
            self._journal_filename = requestData["journalFilename"]

        # An optional directory may have been given - this will become part of
        # the library key and the journal url (if relevant)
        if "directory" in requestData:
            self._directory = requestData["directory"]

        # Try to find the library corresponding to the library key
        self._journal_collection = (library[self.library_key()] if
                                    self.library_key() in library else None)
        if require_in_library and self._journal_collection is None:
            raise InvalidRequest(f"No collection '{self.library_key}' "
                                 f"in library.")

        # Was a data directory provided / required?
        self._data_directory = (requestData["runDataRootUrl"]
                                if "runDataRootUrl" in requestData else None)
        if require_data_directory and self._data_directory is None:
            raise InvalidRequest("Data directory required but not given.")

        # Were run number(s) provided / required?
        if "runNumbers" in requestData:
            self._run_numbers = requestData["runNumbers"]
        if require_run_numbers and len(self._run_numbers) == 0:
            raise InvalidRequest("Run number(s) required but not given.")

        # Was an additional parameter provided / required?
        if require_parameter is not None:
            if require_parameter in requestData:
                self._parameter = requestData[require_parameter]
            else:
                raise InvalidRequest(f"Additional parameter "
                                     f"'{require_parameter}' "
                                     f"required but was not provided.")

    def library_key(self) -> str:
        """Return the library key (same as full URL)"""
        if self._directory is None:
            return self._source_id
        else:
            return url_join(self._source_id, self._directory)

    @property
    def source_id(self) -> str:
        """Return the source ID"""
        return self._source_id

    @property
    def source_type(self) -> SourceType:
        """Return the source type"""
        return self._source_type

    @property
    def journal_root_url(self) -> str:
        """Return the journal root URL"""
        return self._journal_root_url

    @property
    def journal_filename(self) -> str:
        """Return the journal filename"""
        return self._journal_filename

    def journal_file_url(self) -> str:
        """Return the full URL (journalRoot plus any optional directory)"""
        if self._journal_root_url is None or self._journal_filename is None:
            raise ValueError("No journal location is set.")

        if self._directory:
            return url_join(self._journal_root_url, self._directory,
                            self._journal_filename)
        else:
            return url_join(self._journal_root_url, self._journal_filename)

    @property
    def directory(self) -> str:
        """Return the directory (if given)"""
        return self._directory

    @property
    def data_directory(self) -> str:
        """Return the data directory (if given)"""
        return self._data_directory

    @property
    def run_numbers(self) -> typing.List[int]:
        """Return the run numbers (if given)"""
        return self._run_numbers

    @property
    def parameter(self) -> str:
        """Return the additional parameter (if given)"""
        return self._parameter

    @property
    def journal_collection(self) -> jv2backend.journals.JournalCollection:
        """Return the associated JournalCollection object (if any)"""
        return self._journal_collection
