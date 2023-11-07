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
import xml.etree.ElementTree as ElementTree
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
                          f"{len(self.collections[c].journalFiles)} "
                          f"journal files:")
            for j in self.collections[c].journalFiles:
                if j.has_run_data():
                    logging.debug(f"     {j.get_file_url()} "
                                  f"({j.get_run_count()} run data)")
                else:
                    logging.debug(f"     {j.get_file_url()} (not yet loaded)")

    # ---------------- File Operations

    @classmethod
    def _get_journal_file_xml(cls, source_type: SourceType,
                              journal_file_url: str) -> ElementTree:
        """Get the content of the file and create an ElementTree"""
        if source_type == SourceType.Network:
            response = requests.get(journal_file_url, timeout=3)
            response.raise_for_status()
            return ElementTree.parse(BytesIO(response.content))
        elif source_type == SourceType.Generated:
            return ElementTree.parse("{}")
        elif source_type == SourceType.File:
            with open(journal_file_url, "rb") as file:
                buffer = BytesIO(file.read())
            return ElementTree.parse(buffer)
        else:
            raise RuntimeError("No source type set.")

    # ---------------- Work Functions

    def get_index(self, request_data: RequestData) -> FlaskResponse:
        """Retrieve an index file containing journal information, creating a
        collection for it in the process, and return the contained journals.
        The collection may already exist, in which case we return it directly.

        :param request_data: RequestData object containing index file details
        :return: A JSON response with the journal list or an error string

        The index xml file contains a simple list of full journal files in the
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
        # If we already have a collection for the specified source, just
        # return it
        if request_data.library_key() in self.collections:
            logging.debug(f"Returning existing journal collection for "
                          f"'{request_data.library_key()}'")
            return make_response(
                self.collections[request_data.library_key()].to_basic_json(),
                200)

        # Retrieve the specified index as ElementTree data
        try:
            index_tree = self._get_journal_file_xml(
                request_data.source_type,
                request_data.journal_file_url()
            )
        except FileNotFoundError:
            return make_response(jsonify("Index File Not Found"), 200)
        except (requests.HTTPError, requests.ConnectionError) as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)
        except etree.XMLSyntaxError as exc:
            return make_response(jsonify({"Error": str(exc)}), 200)

        index_root = index_tree.getroot()

        # Construct list of valid journal files for return
        journals = []
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
                cycle_dir = j.attrib["name"].replace("journal", "cycle")
                cycle_dir.replace(".xml", "")
                data_directory = url_join(
                    request_data.run_data_root_url,
                    request_data.directory,
                    "Instrument",
                    "data",
                    cycle_dir)

            # Append to our list
            journals.append(
                Journal(
                    display_name,
                    request_data.source_type,
                    request_data.library_key(),
                    url_join(request_data.journal_root_url,
                             request_data.directory),
                    j.attrib["name"], data_directory))

        # Store as a new collection in the library
        self.collections[request_data.library_key()] = JournalCollection(journals)

        return make_response(
            self.collections[request_data.library_key()].to_basic_json(), 200
        )

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
