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

def _create_request_dict(updated_keys: {} = {}) -> {}:
    """Create a dictionary containing basic data for initialising RequestData"""
    result = {
        "sourceID": "TestID",
        "sourceType": "Network",
        "journalRootUrl": FAKE_SERVER_ADDRESS,
        "directory": FAKE_INSTRUMENT_NAME,
        "journalFilename": "journal_main.xml"
    }
    result.update(updated_keys)
    return result

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
    index_request = RequestData(_create_request_dict(), require_journal_file=True)

    with app.app_context():
        index = library.get_index(index_request)
        response = json.loads(index.data)

    assert "Error" not in response
    assert "TestID/" + FAKE_INSTRUMENT_NAME in library

    assert len(response) == 3
    assert response[0]["display_name"] == "Cycle 21 1"
    assert response[1]["display_name"] == "Cycle 20 2"
    assert response[2]["display_name"] == "Cycle 11 1"

@pytest.mark.parametrize("journal_file", ["journal_21_1.xml", "journal_20_2.xml"])
def test_parse_isis_journal_file(app, journal_file):
    library = jv2backend.journalLibrary.JournalLibrary({})

    index_request = RequestData(_create_request_dict(), require_journal_file=True)

    run_data_request = RequestData(_create_request_dict({"journalFilename": journal_file}), require_journal_file=True)

    with app.app_context():
        index = library.get_index(index_request)
        index_response = json.loads(index.data)
        assert "Error" not in index_response
        assert "TestID/" + FAKE_INSTRUMENT_NAME in library

        journal = library.get_journal_data(run_data_request)
        journal_response = json.loads(journal.data)
        assert "Error" not in journal_response
        assert len(journal_response) == 3

def test_missing_journal_file(app,):
    library = jv2backend.journalLibrary.JournalLibrary({})

    index_request = RequestData(_create_request_dict(), require_journal_file=True)

    run_data_request = RequestData(_create_request_dict({"journalFilename": "journal_11_1.xml"}), require_journal_file=True)

    with app.app_context():
        index = library.get_index(index_request)
        index_response = json.loads(index.data)
        assert "Error" not in index_response
        assert "TestID/" + FAKE_INSTRUMENT_NAME in library

        journal = library.get_journal_data(run_data_request)
        journal_response = json.loads(journal.data)
        assert "Error" in journal_response
