# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import pytest
from flask import Flask
from typing import Callable
import logging
from pathlib import Path
from jv2backend.utils import url_join
from jv2backend.app import create_app
from jv2backend.requestData import RequestData
import jv2backend.journalLibrary
import requests_mock
import json

FAKE_SERVER_ADDRESS = "http://fake.url/testing"
FAKE_INSTRUMENT_NAME = "FAKE"


# Private

def _fake_index_url() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), "journal_main.xml")

def _fake_journal_url_a() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), "journal_21_1.xml")

def _fake_journal_url_b() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), "journal_20_2.xml")

def _fake_journal_url_missing() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), "journal_11_1.xml")

def _fake_server_data_dir() -> Path:
    """Return the path to the test data"""
    return Path(__file__).parent / "data/fake_server"

def _fake_server_data_file(filename: str) -> bytes:
    """Provide a function to return sample journal data.
    The fixture accepts a filename for the journal"""
    with open(_fake_server_data_dir() / filename) as handle:
        return handle.read().encode("utf-8")

@pytest.fixture()
def app(requests_mock):
    app = create_app()
    logging.basicConfig(level=logging.DEBUG)
    requests_mock.get(
        _fake_index_url(),
        content=_fake_server_data_file("journal_main.xml"),
        headers={"Last-Modified": "Fri, 04 Nov 2022 00:00:00 GMT"},
    )
    requests_mock.get(
        _fake_journal_url_a(),
        content=_fake_server_data_file("journal_21_1.xml"),
        headers={"Last-Modified": "Fri, 04 Nov 2022 00:00:00 GMT"},
    )
    requests_mock.get(
        _fake_journal_url_b(),
        content=_fake_server_data_file("journal_20_2.xml"),
        headers={"Last-Modified": "Fri, 04 Nov 2022 00:00:00 GMT"},
    )
    requests_mock.get(
        _fake_journal_url_missing(),
        status_code=400
    )

    yield app


def test_parse_isis_journal_index(app):
    library = jv2backend.journalLibrary.JournalLibrary({})
    data = RequestData(
        {"sourceID": "TestID",
         "sourceType": "Network",
         "journalRootUrl": FAKE_SERVER_ADDRESS,
         "directory": FAKE_INSTRUMENT_NAME,
         "journalFilename": "journal_main.xml"},
        require_journal_file=True)

    with app.app_context():
        try:
            index = library.get_index(data)
            journals = json.loads(index.data)
        except Exception as exc:
            pytest.fail(f"Unexpected exception: {exc}")

    assert "TestID/" + FAKE_INSTRUMENT_NAME in library

    assert len(journals) == 3
    assert journals[0]["display_name"] == "Cycle 21 1"
    assert journals[1]["display_name"] == "Cycle 20 2"
    assert journals[2]["display_name"] == "Cycle 11 1"






#
# def test_journal_filenames_raises_Exception_on_http_error(requests_mock):
#     requests_mock.get(
#         _fake_instrument_journallist_url(FAKE_INSTRUMENT_NAME),
#         content=b"Not Found",
#         status_code=404,
#     )
#     server = ISISJournalServer(FAKE_SERVER_ADDRESS)
#
#     with pytest.raises(Exception, match=".*404.*"):
#         server.journal_filenames(FAKE_INSTRUMENT_NAME)
#
#
# def test_journal_raises_ValueError_if_filename_cyclename_not_provided():
#     server = ISISJournalServer(FAKE_SERVER_ADDRESS)
#     with pytest.raises(ValueError):
#         server.journal("ALF")
#
#
# @pytest.mark.parametrize(
#     "call_args", [{"filename": "journal_21_1.xml"}, {"cyclename": "21_1"}]
# )
# def test_journal_returned_on_successful_response(
#     call_args, requests_mock, sample_journal_xml
# ):
#     instrument_name = "ALF"
#     requests_mock.get(
#         _fake_instrument_journal_url(instrument_name, "journal_21_1.xml"),
#         content=sample_journal_xml(),
#     )
#     server = ISISJournalServer(FAKE_SERVER_ADDRESS)
#
#     journal = server.journal(instrument_name, **call_args)
#
#     assert isinstance(journal, Journal)
#     assert journal.run_count == 3
#
#
# def test_journal_call_raises_Exception_on_http_error(requests_mock):
#     instrument_name, journal_filename = "ALF", "bad.xml"
#     requests_mock.get(
#         _fake_instrument_journal_url(instrument_name, journal_filename),
#         content=b"Not Found",
#         status_code=404,
#     )
#     server = ISISJournalServer(FAKE_SERVER_ADDRESS)
#
#     with pytest.raises(Exception, match=".*404.*"):
#         server.journal(instrument_name, filename=journal_filename)
#
#
# def test_check_for_journal_filenames_update_returns_latest_filename_when_changed_since_last_request(
#     requests_mock, sample_journallist_xml
# ):
#     instrument_name = "ALF"
#     server = ISISJournalServer(FAKE_SERVER_ADDRESS)
#
#     requests_mock.get(
#         _fake_instrument_journallist_url(instrument_name),
#         content=sample_journallist_xml,
#         headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
#     )
#     server.journal_filenames(instrument_name)
#     requests_mock.head(
#         _fake_instrument_journallist_url(instrument_name),
#         headers={"Last-Modified": "Fri, 11 Nov 2022 10:34:44 GMT"},
#     )
#
#     result = server.check_for_journal_filenames_update(instrument_name)
#
#     assert result == "journal_20_2.xml"
#
#
# def test_check_for_journal_filenames_update_returns_Nonewhen_no_change_since_last_request(
#     requests_mock, sample_journallist_xml
# ):
#     instrument_name = "ALF"
#     server = ISISJournalServer(FAKE_SERVER_ADDRESS)
#
#     requests_mock.get(
#         _fake_instrument_journallist_url(instrument_name),
#         content=sample_journallist_xml,
#         headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
#     )
#     server.journal_filenames(instrument_name)
#     requests_mock.head(
#         _fake_instrument_journallist_url(instrument_name),
#         headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
#     )
#
#     result = server.check_for_journal_filenames_update(instrument_name)
#
#     assert result is None
#
#
# def test_search_by_user_name_search_across_all_journals(server_faker):
#     instrument_name = "ALF"
#     server = server_faker(instrument_name)
#     run_field, user_input = "user_name", "Username2"
#     search_results = server.search(instrument_name, run_field, user_input)
#
#     assert search_results.run_count == 3
#
#
# def test_search_by_experiment_identifier_search_across_all_journals(server_faker):
#     instrument_name = "ALF"
#     server = server_faker(instrument_name)
#     run_field, user_input = "experiment_identifier", "1234567"
#     search_results = server.search(instrument_name, run_field, user_input)
#
#     assert search_results.run_count == 2
#
#
# def test_search_by_title_search_across_all_journals(server_faker):
#     instrument_name = "ALF"
#     server = server_faker(instrument_name)
#     run_field, user_input = "title", "MnSi"
#     search_results = server.search(instrument_name, run_field, user_input)
#
#     assert search_results.run_count == 3
#
#
# def test_search_by_run_number_search_across_all_journals(server_faker):
#     instrument_name = "ALF"
#     server = server_faker(instrument_name)
#     run_field, user_input = "run_number", "83898-85424"
#     search_results = server.search(instrument_name, run_field, user_input)
#
#     assert search_results.run_count == 4
#
