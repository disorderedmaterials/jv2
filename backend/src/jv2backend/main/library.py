# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2024 Team JournalViewer and contributors

from __future__ import annotations

from jv2backend.classes.collection import JournalCollection
from jv2backend.classes.journal import SourceType
import jv2backend.main.userCache
import requests
import logging
import lxml.etree as etree
import json

class JournalLibrary:
    """Defines one or more data source rootURL/directory and their associated
    journal collections.
    """

    def __init__(self, collections: {} = None):
        self._collections = {} if collections is None else collections

    def __setitem__(self, key, value):
        self._collections[key] = value

    def __getitem__(self, key):
        if key in self._collections:
            return self._collections[key]
        else:
            return None

    def __contains__(self, key):
        return key in self._collections


    def list(self):
        """List contents of library"""
        for c in self._collections:
            collection = self._collections[c]
            logging.debug(f"Collection '{c}' contains "
                          f"{collection.get_journal_count()} "
                          f"journal files:")
            for j in self._collections[c].journals:
                if j.has_run_data():
                    logging.debug(f"     {j.get_file_url()} "
                                  f"({j.get_run_count()} run data)")
                else:
                    if jv2backend.main.userCache.has_data(collection.library_key,
                                                          j.filename):
                        logging.debug(f"     {j.get_file_url()} (in cache)")
                    else:
                        logging.debug(f"     {j.get_file_url()} (not yet loaded)")

    # ---------------- Work Functions

    def get_index(self, source_type: SourceType, library_key: str,
                  journal_directory: str, journal_filename: str,
                  run_data_directory: str) -> str:
        """Retrieve an index file containing journal information, creating a
        collection for it in the process, and return the contained journals.
        The collection may already exist, in which case we return it directly.

        :param source_type: The SourceType for the index
        :param library_key: Library key identifying a collection
        :param journal_directory: Location / URL of the journal index file
        :param journal_filename: Filename of journal index file
        :param run_data_directory: Directory containing associated run data
        :return: A JSON response with the journal list or an error string
        """
        # Check the library for an existing collection
        collection = self[library_key]

        # If we already have a collection for the specified source, return it
        # if it is up-to-date
        if collection is not None:
            if collection.is_up_to_date():
                logging.debug(f"Returning already up-to-date collection for "
                              f"'{library_key}'")
                return collection.to_basic_json()
        else:
            # Create new collection in the library
            logging.debug(f"Creating new collection for "
                          f"'{library_key}'")
            self._collections[library_key] = JournalCollection(
                source_type,
                library_key,
                journal_directory,
                journal_filename,
                run_data_directory
            )
            collection = self[library_key]

        # Retrieve the journal index data, since we either don't have it or it
        # needs updating
        try:
            collection.get_index()
        except FileNotFoundError:
            return json.dumps("Index File Not Found")
        except (requests.HTTPError, requests.ConnectionError) as exc:
            return json.dumps({"Error": str(exc)})
        except etree.XMLSyntaxError as exc:
            return json.dumps({"Error": str(exc)})

        return collection.to_basic_json()
