# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import json
import pytest
from jv2backend.app import create_app

FAKE_SERVER_ADDRESS = "http://fake.url/testing"
TESTDATA_INSTRUMENT_NAME = "ALF"


@pytest.fixture()
def app(requests_mock, sample_journallist_xml, sample_journal_xml):
    app = create_app(FAKE_SERVER_ADDRESS)
    app.config.update({"TESTING": True})
    requests_mock.get(
        _fake_instrument_journallist_url(TESTDATA_INSTRUMENT_NAME),
        content=sample_journallist_xml,
    )
    for journal_filename in ["journal_21_1.xml", "journal_20_2.xml"]:
        requests_mock.get(
            _fake_instrument_journal_url(TESTDATA_INSTRUMENT_NAME, journal_filename),
            content=sample_journal_xml(journal_filename),
        )

    # other setup can go here
    yield app
    # clean up / reset resources here


@pytest.fixture()
def client(app):
    return app.test_client()


def test_request_cycles_returns_expected_json_for_existing_instrument(client):
    response = client.get(f"/getCycles/{TESTDATA_INSTRUMENT_NAME}")

    response_payload = json.loads(response.data)
    assert len(response_payload) == 2
    assert response_payload[0] == "journal_21_1.xml"
    assert response_payload[1] == "journal_20_2.xml"


def test_request_cycles_returns_expected_error_for_nonexistent_instrument(
    client, requests_mock
):
    requests_mock.get(
        _fake_instrument_journallist_url(TESTDATA_INSTRUMENT_NAME),
        content=b"Not Found",
        status_code=404,
    )
    response = client.get(f"/getCycles/{TESTDATA_INSTRUMENT_NAME}")

    response_payload = json.loads(response.data)
    assert "404" in response_payload


def test_request_journal_returns_expected_json_for_existing_journal(client):
    response = client.get(f"/getJournal/{TESTDATA_INSTRUMENT_NAME}/journal_21_1.xml")

    response_payload = json.loads(response.data)
    assert len(response_payload) == 3
    assert TESTDATA_INSTRUMENT_NAME == response_payload[0]["instrument_name"]


def test_request_journal_returns_error_for_nonexisting_journal(client, requests_mock):
    journal_filename = "bad.xml"
    requests_mock.get(
        _fake_instrument_journal_url(TESTDATA_INSTRUMENT_NAME, journal_filename),
        content=b"Not Found",
        status_code=404,
    )

    response = client.get(f"/getJournal/{TESTDATA_INSTRUMENT_NAME}/{journal_filename}")

    response_payload = json.loads(response.data)
    assert "404" in response_payload


@pytest.mark.parametrize("case_sensitive", [True, False])
def test_search_user_name_respects_case_options(case_sensitive, client):
    search_options = (
        "caseSensitivity=true" if case_sensitive else "caseSensitivity=false"
    )
    response = client.get(
        f"/getAllJournals/{TESTDATA_INSTRUMENT_NAME}/user_name/username/{search_options}"
    )

    response_payload = json.loads(response.data)
    if case_sensitive:
        assert len(response_payload) == 0
    else:
        assert len(response_payload) == 6


def test_search_run_number_returns_expected_json(client):
    response = client.get(
        f"/getAllJournals/{TESTDATA_INSTRUMENT_NAME}/run_number/83898-85424/caseSensitivity=true"
    )

    response_payload = json.loads(response.data)
    assert len(response_payload) == 4


@pytest.mark.parametrize("field", ["start_date", "start_time"])
def test_search_start_time_returns_expected_json(field, client):
    response = client.get(
        f"/getAllJournals/{TESTDATA_INSTRUMENT_NAME}/{field}/2021;05;01-2021;05;02/caseSensitivity=true"
    )

    response_payload = json.loads(response.data)
    assert len(response_payload) == 2


def xtest_request_search_over_journals():
    # Example requests
    "/getAllJournals/ALF/user_name/12345/caseSensitivity=true"
    "/getAllJournals/ALF/title/12345/caseSensitivity=true"
    "/getAllJournals/ALF/experiment_identifier/12345/caseSensitivity=true"
    "/getAllJournals/ALF/run_number/12344-12345/caseSensitivity=true"

    "/getAllJournals/ALF/start_date/12345/caseSensitivity=true"


# private functions


def _fake_instrument_journal_url(instrument_name: str, journal_filename: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/{journal_filename}"


def _fake_instrument_journallist_url(instrument_name: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/journal_main.xml"
