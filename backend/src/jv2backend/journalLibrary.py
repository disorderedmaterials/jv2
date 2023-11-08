# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

from __future__ import annotations
from dataclasses import dataclass

from io import BytesIO
from flask import jsonify, make_response
from flask.wrappers import Response as FlaskResponse
from jv2backend.utils import url_join, lm_to_datetime
from jv2backend.journalCollection import JournalCollection
from jv2backend.journal import Journal, SourceType
from jv2backend.requestData import RequestData, InvalidRequest
import jv2backend.userCache
import requests
import logging
import lxml.etree as etree
import typing

@dataclass
class JournalLibrary:
    """Defines one or more data source rootURL/directory and their associated
    journal collections.
    """
    collections: typing.Dict[str, JournalCollection]

    def __setitem__(self, key, value):
        self.collections[key] = value

    def __getitem__(self, key):
        if key in self.collections:
            return self.collections[key]
        else:
            return None

    def __contains__(self, key):
        return key in self.collections


    def list(self):
        """List contents of library"""
        for c in self.collections:
            logging.debug(f"Collection '{c}' contains "
                          f"{self.collections[c].get_journal_count()} "
                          f"journal files:")
            for j in self.collections[c].journals:
                if j.has_run_data():
                    logging.debug(f"     {j.get_file_url()} "
                                  f"({j.get_run_count()} run data)")
                else:
                    logging.debug(f"     {j.get_file_url()} (not yet loaded)")

    # ---------------- Work Functions

    def get_index(self, request_data: RequestData) -> FlaskResponse:
        """Retrieve an index file containing journal information, creating a
        collection for it in the process, and return the contained journals.
        The collection may already exist, in which case we return it directly.

        :param request_data: RequestData object containing index file details
        :return: A JSON response with the journal list or an error string
        """
        # Check the library for an existing collection
        collection = self[request_data.library_key()]

        # If we already have a collection for the specified source, return it
        # if it is up-to-date
        if collection is not None:
            if collection.is_up_to_date():
                logging.debug(f"Returning already up-to-date collection for "
                              f"'{request_data.library_key()}'")
                return make_response(collection.to_basic_json(), 200)
        else:
            # Create new collection in the library
            logging.debug(f"Creating new collection for "
                          f"'{request_data.library_key()}'")
            self.collections[request_data.library_key()] = JournalCollection(
                request_data.source_type,
                request_data.library_key(),
                url_join(request_data.journal_root_url,
                         request_data.directory),
                request_data.journal_filename,
                url_join(request_data.run_data_root_url,
                         request_data.directory)
            )
            collection = self[request_data.library_key()]

        # Retrieve the journal index data, since we either don't have it or it
        # needs updating
        try:
            collection.get_index()
        except FileNotFoundError:
            return make_response(jsonify("Index File Not Found"), 200)
        except (requests.HTTPError, requests.ConnectionError) as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)
        except etree.XMLSyntaxError as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        return make_response(collection.to_basic_json(), 200)

    def get_journal_data(self, request_data: RequestData) -> FlaskResponse:
        """Retrieve run data contained in a journal file within a collection

        :param request_data: RequestData object containing target source / journal
        :return: A JSON response with the journal list or an error string
        """
        # Check whether the specified collection exists
        if request_data.library_key() not in self.collections:
            return make_response(
                jsonify({"Error": f"No library '{request_data.library_key()}' "
                                  f"currently exists."}), 200
            )

        collection = self.collections[request_data.library_key()]
        return collection.get_journal_data(request_data)

    def get_journal_data_updates(self, request_data: RequestData) -> FlaskResponse:
        """Retrieve new run data contained in a journal file within a collection

        :param request_data: RequestData object containing target source / journal
        :return: A JSON response with the journal list or an error string
        """
        # Check whether the specified collection exists
        if request_data.library_key() not in self.collections:
            return make_response(
                jsonify({"Error": f"No library '{request_data.library_key()}' "
                                  f"currently exists."}), 200
            )

        collection = self.collections[request_data.library_key()]
        return collection.get_updates(request_data)

    def search_collection(self, request_data: RequestData) -> FlaskResponse:
        """Retrieve run data contained in a collection where it matches all of
        the specified search query parameters.

        :param request_data: RequestData object containing target source
        :return: A JSON response with the journal list or an error string
        """
        # Check whether the specified collection exists
        if request_data.library_key() not in self.collections:
            return make_response(
                jsonify({"Error": f"No library '{request_data.library_key()}' "
                                  f"currently exists."}), 200
            )

        # Get the collection and make sure we have all data for all journals
        collection = self.collections[request_data.library_key()]
        collection.get_all_journal_data()

        results = collection.search(request_data.value_map)
        return make_response(Journal.convert_run_data_to_json_array(results), 200)
