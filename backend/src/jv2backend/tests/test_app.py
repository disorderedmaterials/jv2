# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import json
import pytest
from jv2backend.app import create_app

FAKE_SERVER_ADDRESS = "http://fake.url/testing"


@pytest.fixture()
def app():
    app = create_app(FAKE_SERVER_ADDRESS)
    app.config.update({"TESTING": True})
    # other setup can go here
    yield app
    # clean up / reset resources here


@pytest.fixture()
def client(app):
    return app.test_client()


def test_request_cycles_returns_expected_json_for_existing_instrument(
    client, requests_mock, sample_journallist_xml
):
    instrument_name = "ALF"
    requests_mock.get(
        _fake_instrument_journallist_url(instrument_name),
        content=sample_journallist_xml,
    )
    response = client.get(f"/getCycles/{instrument_name}")

    response_payload = json.loads(response.data)
    assert len(response_payload) == 1
    assert response_payload[0] == "journal_21_1.xml"


def test_request_cycles_returns_expected_error_for_nonexistent_instrument(
    client, requests_mock
):
    instrument_name = "FAKE"
    requests_mock.get(
        _fake_instrument_journallist_url(instrument_name),
        content=b"Not Found",
        status_code=404,
    )
    response = client.get(f"/getCycles/{instrument_name}")

    response_payload = json.loads(response.data)
    assert "404" in response_payload


def test_request_journal_returns_expected_json_for_existing_journal(
    client, requests_mock, sample_journal_xml
):
    instrument_name, journal_filename = "ALF", "journal_21_1.xml"
    requests_mock.get(
        _fake_instrument_journal_url(instrument_name, journal_filename),
        content=sample_journal_xml,
    )
    response = client.get("/getJournal/ALF/journal_21_1.xml")

    response_payload = json.loads(response.data)
    assert len(response_payload) == 3
    assert instrument_name == response_payload[0]["instrument_name"]


def test_request_journal_returns_error_for_nonexisting_journal(client, requests_mock):
    instrument_name, journal_filename = "ALF", "bad.xml"
    requests_mock.get(
        _fake_instrument_journal_url(instrument_name, journal_filename),
        content=b"Not Found",
        status_code=404,
    )

    response = client.get(f"/getJournal/{instrument_name}/{journal_filename}")

    response_payload = json.loads(response.data)
    assert "404" in response_payload


def xtest_request_search_over_journals():
    # Example requests
    "/getAllJournals/ALF/experiment_identifier/12345/caseSensitivity=true"
    "/getAllJournals/ALF/title/12345/caseSensitivity=true"
    "/getAllJournals/ALF/user_name/12345/caseSensitivity=true"
    "/getAllJournals/ALF/run_number/12345/caseSensitivity=true"
    "/getAllJournals/ALF/start_date/12345/caseSensitivity=true"


# private functions


def _fake_instrument_journal_url(instrument_name: str, journal_filename: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/{journal_filename}"


def _fake_instrument_journallist_url(instrument_name: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/journal_main.xml"
