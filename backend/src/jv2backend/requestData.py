# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
import jv2backend.journals
from jv2backend.utils import url_join


class RequestData:
    """Simple class for checking / handling POST data supplied to a Flask
    route"""

    def __init__(self, requestData: typing.Any,
                 library: jv2backend.journals.JournalLibrary,
                 require_in_library=False,
                 require_data_directory=False,
                 require_filename=False) -> None:
        """Set up the class. The POST data contains the following:
              rootUrl: The main, root URL path (http or file)
            directory: [OPTIONAL] Directory within the rootUrl to consider
        dataDirectory: [OPTIONAL] Associated run data directory
             filename: [OPTIONAL] Target filename

        We can make various stipulations on the available data:
            require_in_library: Whether a library matching the library_key
                                must already exist
        require_data_directory: Whether a 'dataDirectory' must be provided
              require_filename: Whether a 'filename' must be provided
        """
        self._root_url: str = None
        self._directory: str = None
        self._full_url: str = None
        self._data_directory: str = None
        self._filename: str = None
        self._journal_collection: jv2backend.journals.JournalCollection = None
        self._is_http: bool = False
        self._error: str = None

        # Root url is always required
        if "rootUrl" not in requestData:
            self._error = "No root URL provided in request."
            return
        self._root_url = requestData["rootUrl"]

        # Determine whether this is a network or file url
        self._is_http = (self._root_url.lower().startswith("http://") or
                         self._root_url.lower().startswith("https://"))

        # Assess the full path / library key - may have optional "directory"
        if "directory" in requestData:
            self._directory = requestData["directory"]
        self._full_url = (self._root_url if self._directory is None else
                          url_join(self._root_url, self._directory))

        # Try to find the library corresponding to the library key
        self._journal_collection = (library[self._full_url] if
                                    self._full_url in library else None)
        if require_in_library and self._journal_collection is None:
            self._error = f"No collection {self._full_url} in library."
            return

        # Was a data directory provided?
        self._data_directory = (requestData["dataDirectory"]
                                if "dataDirectory" in requestData else None)

        # Was a data directory required?
        if require_data_directory and self._data_directory is None:
            self._error = f"Data directory required for URL \
                          {self._full_url}."
            return

        # Was an optional filename provided?
        self._filename = (requestData["filename"] if "filename" in
                          requestData else None)

        # Was a filename required?
        if require_filename and self._filename is None:
            self._error = f"Filename required for URL {self._full_url}."
            return

    @property
    def url(self) -> str:
        """Return the full URL (rootUrl plus any optional directory)"""
        return self._full_url

    @property
    def root_url(self) -> str:
        """Return the rootUrl"""
        return self._root_url

    @property
    def directory(self) -> str:
        """Return the directory within the rootURl (if provided)"""
        return self._directory

    @property
    def data_directory(self) -> bool:
        """Return the data directory (if given)"""
        return self._data_directory

    @property
    def is_http(self) -> bool:
        """Return whether the URL is an HTTP location"""
        return self._is_http

    @property
    def file_url(self) -> str:
        """Return the full URL to the filename provided"""
        if self._filename is None:
            raise ValueError("No filename is set.")
        return url_join(self._full_url, self._filename)

    @property
    def library_key(self) -> str:
        """Return the library key (same as full URL)"""
        return self._full_url

    @property
    def journal_collection(self) -> jv2backend.journals.JournalCollection:
        """Return the associated JournalCollection object (if any)"""
        return self._journal_collection

    @property
    def is_valid(self) -> bool:
        """Return whether the route data is all valid"""
        return self._error is None

    @property
    def error(self) -> str:
        """Return the encountered error (if any)"""
        return self._error
