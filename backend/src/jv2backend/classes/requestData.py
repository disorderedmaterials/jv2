# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import typing
from jv2backend.utils import url_join
from jv2backend.classes.journal import SourceType
import logging

class InvalidRequest(Exception):

    def __init__(self, message):
        Exception.__init__(self)
        self._message = message

    def __str__(self):
        return self._message


class RequestData:
    """Simple class for checking / handling POST data supplied to a Flask
    route"""

    def __init__(self, requestData: typing.Any,
                 require_journal_file=False,
                 require_data_directory=False,
                 require_run_numbers=False,
                 require_parameters=None,
                 require_value_map=False) -> None:
        """Store recognised items in the POST data. We can make various
         stipulations on the contents:

          require_journal_file: Whether a full journal index/file must be
                                given
        require_data_directory: Whether a 'runDataRootUrl' must be provided
              require_filename: Whether a 'filename' must be provided
           require_run_numbers: Whether one or more run numbers are expected
            require_parameters: Comma-separated list of additional parameters
                                to expect as part of the request
             require_value_map: Whether a map of key=value is expected
        """
        self._source_id: str = None
        self._source_type: SourceType = SourceType.Unknown
        self._journal_root_url: str = None
        self._journal_filename: str = None
        self._instrument: str = None
        self._run_data_root_url: str = None
        self._parameters: typing.Dict[str, str] = {}
        self._run_numbers: typing.List[int] = []
        self._value_map: typing.Dict[str, str] = {}

        # Source ID / type always required - ID in conjunction with the
        # optional 'directory' makes up our unique library key for the
        # collection
        if "sourceID" not in requestData:
            raise InvalidRequest("No source ID provided in request.")
        self._source_id = requestData["sourceID"]
        if "sourceType" not in requestData:
            raise InvalidRequest("No source type provided in request.")
        self._source_type = SourceType[requestData["sourceType"]]

        # Journal file required?
        if "journalRootUrl" in requestData:
            self._journal_root_url = requestData["journalRootUrl"]
        if "journalFilename" in requestData:
            self._journal_filename = requestData["journalFilename"]
        elif require_journal_file:
            raise InvalidRequest("No journal filename provided in "
                                 "request.")

        # An optional instrument may have been given - this will become part of
        # the library key
        if "instrument" in requestData:
            self._instrument = requestData["instrument"]

        # Was a data directory provided / required?
        self._run_data_root_url = (requestData["runDataRootUrl"]
                                   if "runDataRootUrl" in requestData
                                   else None)
        if require_data_directory and self._run_data_root_url is None:
            raise InvalidRequest("Data directory required but not given.")

        # Were run number(s) provided / required?
        if "runNumbers" in requestData:
            for run in requestData["runNumbers"]:
                self._run_numbers.append(int(run))
        if require_run_numbers and len(self._run_numbers) == 0:
            raise InvalidRequest("Run number(s) required but not given.")

        # Were additional parameters required?
        params = ([] if require_parameters is None
                  else require_parameters.split(","))
        for p in params:
            if p in requestData:
                self._parameters[p] = requestData[p]
            else:
                raise InvalidRequest(f"Additional parameter '{p}' "
                                     f"required but was not provided.")

        # Was a value map provided / required?
        if "valueMap" in requestData:
            self._value_map = requestData["valueMap"]
            logging.debug(self._value_map)
        elif require_value_map:
            raise InvalidRequest("Value map was required but was not "
                                 "provided.")

    def library_key(self) -> str:
        """Return the library key (same as full URL)"""
        if self._instrument is None:
            return self._source_id
        else:
            return url_join(self._source_id, self._instrument)

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
        """Return the full journal file URL"""
        return url_join(self._journal_root_url,
                        self._journal_filename)

    @property
    def instrument(self) -> str:
        """Return the directory (if given)"""
        return self._instrument

    @property
    def run_data_root_url(self) -> str:
        """Return the root of the data directory (if given)"""
        return self._run_data_root_url

    @property
    def run_numbers(self) -> typing.List[int]:
        """Return the run numbers (if given)"""
        return self._run_numbers

    def parameter(self, name: str) -> str:
        """Return the additional named parameter (if given)"""
        if name in self._parameters:
            return self._parameters[name]

        raise RuntimeError(f"The parameter '{name}' is not in the request data.")

    @property
    def value_map(self) -> {}:
        """Return the value map (if given)"""
        return self._value_map
