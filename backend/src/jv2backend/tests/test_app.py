# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2022 E. Devlin, M. Gigg and T. Youngs
import json
import pytest
from unittest.mock import MagicMock

from jv2backend.app import create_app

FAKE_SERVER_ADDRESS = "http://fake.url/testing"
TESTDATA_INSTRUMENT_NAME = "ALF"


@pytest.fixture()
def app(
    requests_mock, sample_journallist_xml, sample_journal_xml, sample_nexus_filepath
):
    run_locator_mock = MagicMock()
    run_locator_mock.locate.return_value = sample_nexus_filepath

    app = create_app(FAKE_SERVER_ADDRESS, run_locator=run_locator_mock)
    app.config.update({"TESTING": True})
    requests_mock.get(
        _fake_instrument_journallist_url(TESTDATA_INSTRUMENT_NAME),
        content=sample_journallist_xml,
        headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
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


# ------------------------ Journal access routes ------------------------


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


def test_pingcycle_returns_empty_str_when_no_change_since_last_request(
    client, requests_mock
):
    requests_mock.head(
        _fake_instrument_journallist_url(TESTDATA_INSTRUMENT_NAME),
        headers={"Last-Modified": "Fri, 04 Nov 2022 10:34:44 GMT"},
    )

    response = client.get(f"/pingCycle/{TESTDATA_INSTRUMENT_NAME}")

    assert response.data == b""


def test_pingcycle_returns_final_journal_name_when_changed_since_last_request(
    client, requests_mock
):
    client.get(f"/getCycles/{TESTDATA_INSTRUMENT_NAME}")
    requests_mock.head(
        _fake_instrument_journallist_url(TESTDATA_INSTRUMENT_NAME),
        headers={"Last-Modified": "Fri, 11 Nov 2022 10:34:44 GMT"},
    )
    response = client.get(f"/pingCycle/{TESTDATA_INSTRUMENT_NAME}")

    assert response.data == b"journal_20_2.xml"


def test_updatejournal_returns_runs_later_than_last_given(client):
    response = client.get(
        f"/updateJournal/{TESTDATA_INSTRUMENT_NAME}/journal_21_1.xml/85422"
    )

    runs = json.loads(response.data)
    assert len(runs) == 2
    assert runs[0]["run_number"] == "85423"
    assert runs[1]["run_number"] == "85424"


# ---------------------- NeXus access routes ----------------------


def test_getNexusFields_returns_all_expected_log_data_fields_paths(client):
    response = client.get(f"/getNexusFields/{TESTDATA_INSTRUMENT_NAME}/21_1;/85423;")

    fields = json.loads(response.data)
    assert len(fields) == 3
    for group in fields:
        assert group[0] in ("selog", "runlog", "framelog")


def test_getNexusData_returns_all_expected_log_data_fields(client):
    fields = f":raw_data_1:selog:Z;:raw_data_1:runlog:dae_beam_current"
    response = client.get(
        f"/getNexusData/{TESTDATA_INSTRUMENT_NAME}/21_1;/85423;/{fields}"
    )

    data = json.loads(response.data)
    assert len(data) == 2
    # first entry is all of the fields
    all_fields = data[0]
    for group in all_fields:
        assert group[0] in ("selog", "runlog", "framelog")

    # second entry describes the run
    run_data = data[1]
    start, end = run_data[0]
    assert "2021" in start
    assert "2021" in end
    selog_values = run_data[1]
    assert selog_values[0][0] == "85423"
    assert selog_values[0][1].endswith("Z")
    assert selog_values[1][0] == pytest.approx(-5779.0)
    runlog_values = run_data[2]
    assert runlog_values[0][0] == "85423"
    assert runlog_values[0][1].endswith("current")
    assert runlog_values[1][0] == pytest.approx(-5775.0)


def test_getSpectrumRange_returns_spectrum_count(client):
    response = client.get(
        f"/getSpectrumRange/{TESTDATA_INSTRUMENT_NAME}/21_1/85423;85423"
    )

    data = json.loads(response.data)
    assert data[0] == 2368
    assert data[1] == 2368


# ---------------------- Private functions ----------------------


def _fake_instrument_journal_url(instrument_name: str, journal_filename: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/{journal_filename}"


def _fake_instrument_journallist_url(instrument_name: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/journal_main.xml"
