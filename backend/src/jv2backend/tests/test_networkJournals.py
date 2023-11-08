# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2023 Team JournalViewer and contributors

import pytest
import logging
from pathlib import Path
from jv2backend.utils import url_join
from jv2backend.app import create_app
from jv2backend.requestData import RequestData
from jv2backend.journal import SourceType
import jv2backend.journalLibrary
import datetime
import requests_mock
import json

FAKE_SERVER_ADDRESS = "http://fake.url/testing"
FAKE_INSTRUMENT_NAME = "FAKE"
FAKE_INDEX_FILE = "journal_main.xml"
FAKE_JOURNAL_FILE_A = "journal_21_1.xml"
FAKE_JOURNAL_FILE_B = "journal_20_2.xml"
FAKE_JOURNAL_FILE_MISSING = "journal_11_1.xml"

# Private

def _fake_index_url() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), FAKE_INDEX_FILE)

def _fake_journal_url_a() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), FAKE_JOURNAL_FILE_A)

def _fake_journal_url_b() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), FAKE_JOURNAL_FILE_B)

def _fake_journal_url_missing() -> str:
    return url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME.lower(), FAKE_JOURNAL_FILE_MISSING)

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

def _get_library_index(library: jv2backend.journalLibrary.JournalLibrary) -> str:
    """Retrieve library index"""
    return json.loads(library.get_index(
        SourceType.Network,
        "TestID/" + FAKE_INSTRUMENT_NAME,
        url_join(FAKE_SERVER_ADDRESS, FAKE_INSTRUMENT_NAME),
        "journal_main.xml",
        "/fake/data/directory"
    ))

@pytest.fixture()
def app(requests_mock):
    app = create_app(activate_cache=False)
    logging.basicConfig(level=logging.DEBUG)
    requests_mock.get(
        _fake_index_url(),
        content=_fake_server_data_file(FAKE_INDEX_FILE),
        headers={"Last-Modified": "Fri, 04 Nov 2022 00:00:00 GMT"},
    )
    requests_mock.get(
        _fake_journal_url_a(),
        content=_fake_server_data_file(FAKE_JOURNAL_FILE_A),
        headers={"Last-Modified": "Fri, 04 Nov 2022 00:00:00 GMT"},
    )
    requests_mock.get(
        _fake_journal_url_b(),
        content=_fake_server_data_file(FAKE_JOURNAL_FILE_B),
        headers={"Last-Modified": "Fri, 04 Nov 2022 00:00:00 GMT"},
    )
    requests_mock.get(
        _fake_journal_url_missing(),
        status_code=400
    )
    requests_mock.head(
        _fake_journal_url_a(),
        headers={"Last-Modified": "Fri, 04 Nov 2022 00:00:00 GMT"},
    )

    yield app


def test_parse_isis_journal_index(app):
    library = jv2backend.journalLibrary.JournalLibrary({})

    with app.app_context():
        response = _get_library_index(library)

    assert "Error" not in response
    assert "TestID/" + FAKE_INSTRUMENT_NAME in library

    assert len(response) == 3
    assert response[0]["display_name"] == "Cycle 21 1"
    assert response[1]["display_name"] == "Cycle 20 2"
    assert response[2]["display_name"] == "Cycle 11 1"


@pytest.mark.parametrize("journal_file", [FAKE_JOURNAL_FILE_A, FAKE_JOURNAL_FILE_B])
def test_parse_isis_journal_file(app, journal_file):
    library = jv2backend.journalLibrary.JournalLibrary({})

    run_data_request = RequestData(_create_request_dict({"journalFilename": journal_file}), require_journal_file=True)

    with app.app_context():
        index_response = _get_library_index(library)
        assert "Error" not in index_response
        assert "TestID/" + FAKE_INSTRUMENT_NAME in library

        journal = library.get_journal_data(run_data_request)
        journal_response = json.loads(journal.data)
        assert "Error" not in journal_response
        assert len(journal_response) == 3

def test_missing_journal_file(app,):
    library = jv2backend.journalLibrary.JournalLibrary({})

    run_data_request = RequestData(_create_request_dict({"journalFilename": FAKE_JOURNAL_FILE_MISSING}), require_journal_file=True)

    with app.app_context():
        index_response = _get_library_index(library)
        assert "Error" not in index_response
        assert "TestID/" + FAKE_INSTRUMENT_NAME in library

        journal = library.get_journal_data(run_data_request)
        journal_response = json.loads(journal.data)
        assert "Error" in journal_response


def test_get_journal_file_updates(app):
    library = jv2backend.journalLibrary.JournalLibrary({})

    run_data_request = RequestData(_create_request_dict({"journalFilename": FAKE_JOURNAL_FILE_A}), require_journal_file=True)

    # Assemble the collection in the library and load in the full journal data
    with app.app_context():
        index_response = _get_library_index(library)
        assert "Error" not in index_response
        assert "TestID/" + FAKE_INSTRUMENT_NAME in library

        journal = library.get_journal_data(run_data_request)
        journal_response = json.loads(journal.data)
        assert "Error" not in journal_response
        assert len(journal_response) == 3

    # Get the target journal
    collection = library["TestID/" + FAKE_INSTRUMENT_NAME]
    assert collection is not None
    journal = collection[FAKE_JOURNAL_FILE_A]
    assert journal is not None

    # Try to update current journal - will be up-to-date, so expect None
    with app.app_context():
        updates = library.get_journal_data_updates(run_data_request)
        updates_response = json.loads(updates.data)
        assert updates_response is None

    # Delete last run data from the journal, and set a new modtime
    last_run_number0 = journal.get_last_run_number()
    del(journal.run_data[last_run_number0])
    journal.run_data = journal.run_data
    last_run_number1 = journal.get_last_run_number()
    del(journal.run_data[last_run_number1])
    journal.run_data = journal.run_data
    journal.last_modified = journal.last_modified - datetime.timedelta(days=1)
    assert journal.get_run_count() == 1

    # Try to update current journal
    with app.app_context():
        updates = library.get_journal_data_updates(run_data_request)
        logging.debug(str(updates.data))
        updates_response = json.loads(updates.data)
        assert len(updates_response) == 2
        assert updates_response[0]["run_number"] == str(last_run_number1)
        assert updates_response[1]["run_number"] == str(last_run_number0)

def test_get_journal_file_updates_for_empty_journal(app):
    library = jv2backend.journalLibrary.JournalLibrary({})

    run_data_request = RequestData(_create_request_dict({"journalFilename": FAKE_JOURNAL_FILE_A}), require_journal_file=True)

    # Assemble the collection in the library
    with app.app_context():
        index_response = _get_library_index(library)
        assert "Error" not in index_response
        assert "TestID/" + FAKE_INSTRUMENT_NAME in library

    # Try to update current journal (which currently has zero data)
    with app.app_context():
        updates = library.get_journal_data_updates(run_data_request)
        updates_response = json.loads(updates.data)
        assert len(updates_response) == 3
