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


def test_request_journal_returns_expected_json(
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


# private functions


def _fake_instrument_journal_url(instrument_name: str, journal_filename: str) -> str:
    return FAKE_SERVER_ADDRESS + f"/ndx{instrument_name.lower()}/{journal_filename}"
